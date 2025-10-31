#include <assert.h>
#include <linux/limits.h>
#include <settings.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "codes.h"
#include "errors.h"
#include "exec.h"
#include "file.h"
#include "io_callback.h"
#include "serialization.h"
#include "task.h"
#include "utils.h"

enum
{
    MAX_DATA_LEN = 9 * 1024 * 1024, // 9 megabytes
    MAX_ENV_NUM = 128,
    PAD_BUF_LEN = 255
};

static int send_response_io_callback_wrapper(void *p_data, uint8_t *send_buffer, ssize_t len);
static int network_recv_all_io_callback_wrapper(void *p_data, uint8_t *recv_buffer, ssize_t len);
static int network_send_all_io_callback_wrapper(void *p_data, uint8_t *send_buffer, ssize_t len);
static int receive_task(int sock, task_t *p_task);
static int handle_exec_do(int sock, task_t *p_task);
static int handle_file_download(int sock, task_t *p_task);
static int handle_file_upload(int sock, task_t *p_task);
static int do_task(int sock, task_t *p_task, settings_t *p_settings);

static int send_response(int sock, int8_t op_code, void *data, size_t len)
{
    int err = EMBER_SUCCESS;

    size_t send_len = sizeof(int8_t) + sizeof(uint64_t) + len;
    uint8_t *send_buf = (uint8_t *)malloc(send_len);

    uint64_t net_len = len;
    net_len = utils_htonll(net_len);

    size_t bytes_copied = 0;
    memcpy(send_buf, &op_code, sizeof(uint8_t));
    bytes_copied += sizeof(uint8_t);

    memcpy(send_buf + bytes_copied, &net_len, sizeof(uint64_t));
    bytes_copied += sizeof(uint64_t);

    memcpy(send_buf + bytes_copied, data, len);

    if ((ssize_t)send_len != utils_sendall(sock, send_buf, send_len, MSG_NOSIGNAL))
    {
        err = -EMBER_ERROR;
    }

    DEBUG_PRINT("exit %d", err);
    return err;
}

int task_receive_and_execute(int sock, settings_t *p_settings)
{
    if (NULL == p_settings)
    {
        return -EMBER_ERROR;
    }

    int err = EMBER_SUCCESS;

    while (EMBER_SUCCESS == err)
    {
        task_t task = {0};

        err = receive_task(sock, &task);

        if (EMBER_SUCCESS == err)
        {
            err = do_task(sock, &task, p_settings);
        }

        if (EMBER_SUCCESS == err)
        {
            DEBUG_PERROR("no response");
            err = send_response(sock, task.response_code, NULL, 0);
        }
        DEBUG_PERROR("post response");

        if (DISCONNECT == task.response_code)
        {
            break;
        }
    }
    DEBUG_PRINT("exit %d", err);
    return err;
}

static int send_response_io_callback_wrapper(void *p_data, uint8_t *send_buffer, ssize_t len)
{
    assert((p_data != NULL) && (send_buffer != NULL) && (0 < len));
    return send_response(*(int *)p_data, OUTPUT, send_buffer, (size_t)len);
}

static int network_recv_all_io_callback_wrapper(void *p_data, uint8_t *recv_buffer, ssize_t len)
{
    assert((p_data != NULL) && (recv_buffer != NULL) && (0 < len));
    return (int)utils_recvall(*((int *)p_data), recv_buffer, (size_t)len, 0);
}

static int network_send_all_io_callback_wrapper(void *p_data, uint8_t *send_buffer, ssize_t len)
{
    assert((p_data != NULL) && (send_buffer != NULL) && (0 < len));
    return (int)utils_sendall(*((int *)p_data), send_buffer, (size_t)len, MSG_NOSIGNAL);
}

static int receive_task(int sock, task_t *p_task)
{
    int err = EMBER_SUCCESS;

    DEBUG_PERROR("enter");

    // replace with header-specific receiving function
    uint8_t hdr_buf[TASK_HDR_LEN] = {0};
    if (TASK_HDR_LEN != utils_recvall(sock, hdr_buf, TASK_HDR_LEN, 0))
    {
        DEBUG_MSG("hdr recv");
        err = -EMBER_ERROR;
    }

    // Validate task, check lengths and such
    if (EMBER_SUCCESS == err)
    {
        deserialize_task_header(&p_task->hdr, hdr_buf);
        p_task->raw_data = (uint8_t *)malloc(p_task->hdr.data_len);
        if (NULL == p_task->raw_data)
        {
            DEBUG_PERROR("malloc");
            err = -EMBER_ERROR;
        }
    }

    if (EMBER_SUCCESS == err)
    {
        uint8_t pad_buf[PAD_BUF_LEN] = {0};
        if (((ssize_t)p_task->hdr.data_len != utils_recvall(sock, p_task->raw_data, (size_t)p_task->hdr.data_len, 0)) ||
            ((ssize_t)p_task->hdr.pad_len != utils_recvall(sock, pad_buf, (size_t)p_task->hdr.pad_len, 0)))
        {
            err = -EMBER_ERROR;
        }
    }

    if (EMBER_SUCCESS == err)
    {
        err = deserialize_task(p_task);
    }

    if ((EMBER_SUCCESS != err) && (NULL != p_task->raw_data))
    {
        utils_free(p_task->raw_data);
    }

    DEBUG_PERROR("exit");

    return err;
}

static int handle_exec_do(int sock, task_t *p_task)
{
    int err = EMBER_SUCCESS;

    if ((uint16_t)IN_MEM & p_task->hdr.flags)
    {
        io_callback_t receiver = {.func = network_recv_all_io_callback_wrapper, .data = &sock};
        err = exec_receive_payload(&receiver, &p_task->exec, p_task->hdr.file_len, &p_task->response_code);
    }

    if (EMBER_SUCCESS == err)
    {
        io_callback_t sender = {.func = send_response_io_callback_wrapper, .data = &sock};
        err = exec_run(&sender, p_task->hdr.flags, &p_task->exec, &p_task->response_code);
    }

    return err;
}

static int handle_file_download(int sock, task_t *p_task)
{
    char resolved_path[PATH_MAX] = {0};
    int8_t *p_res = &p_task->response_code;

    int err = file_resolve_path(p_task->file.path, resolved_path, false, p_res);

    int download_fd = -1;
    struct stat download_stat = {0};
    if ((EMBER_SUCCESS == err) && (SUCCESS == *p_res))
    {
        download_fd = file_open_for_reading(resolved_path, &download_stat, p_task->hdr.data_len, p_res, &err);
    }

    if (EMBER_SUCCESS == err)
    {
        int64_t file_size = download_stat.st_size;
        err = send_response(sock, *p_res, &file_size, (size_t)sizeof(uint64_t));
    }

    if ((EMBER_SUCCESS == err) && (SUCCESS == *p_res))
    {
        io_callback_t reader = {.func = network_send_all_io_callback_wrapper, .data = &sock};
        err = file_read_in_chunks(&reader, (size_t)download_stat.st_size, download_fd, p_res);
    }

    if ((-1 != download_fd) && (-1 == close(download_fd)))
    {
        DEBUG_PERROR("close");
        err = -EMBER_ERROR;
    }

    return err;
}

static int handle_file_upload(int sock, task_t *p_task)
{
    char resolved_path[PATH_MAX] = {0};
    int8_t *p_res = &p_task->response_code;

    int err = file_resolve_path(p_task->file.path, resolved_path, true, p_res);

    int upload_fd = -1;
    if ((EMBER_SUCCESS == err) && (SUCCESS == *p_res))
    {
        upload_fd = file_open_for_writing(resolved_path, p_task->hdr.perms, (uint16_t)OVERWRITE & p_task->hdr.flags,
                                          p_res, &err);
    }

    if (EMBER_SUCCESS == err)
    {
        err = send_response(sock, *p_res, NULL, 0);
    }

    if ((EMBER_SUCCESS == err) && (SUCCESS == *p_res))
    {
        io_callback_t reader = {.func = network_recv_all_io_callback_wrapper, .data = &sock};
        err = file_write_in_chunks(&reader, p_task->hdr.file_len, upload_fd, p_res);
    }

    // TODO(user): unlink on error here based on what error happened
    if ((-1 != upload_fd) && (-1 == close(upload_fd)))
    {
        DEBUG_PERROR("close");
        err = -EMBER_ERROR;
    }

    return err;
}

static int do_task(int sock, task_t *p_task, settings_t *p_settings)
{
    int err = EMBER_SUCCESS;

    switch (p_task->hdr.op_code)
    {
    case SETTINGS:
        DEBUG_PRINT("Settings update %ld %ld", p_task->settings.interval.tv_sec, p_settings->interval.tv_sec);
        p_task->response_code = settings_update(p_task->hdr.flags, &p_task->settings, p_settings);
        break;
    case EXEC:
        err = handle_exec_do(sock, p_task);
        break;
    case DOWNLOAD:
        err = handle_file_download(sock, p_task);
        break;
    case UPLOAD:
        err = handle_file_upload(sock, p_task);
        break;
    case DISCONNECT: // NOLINT (bugprone-branch-clone)
        break;
    case EXIT: // NOLINT (bugprone-branch-clone)
        break;
    default:
        DEBUG_MSG("Invalid op_code");
        err = -EMBER_ERROR;
        break;
    }

    return err;
}
