
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "codes.h"
#include "errors.h"
#include "file.h"
#include "io_callback.h"

int file_resolve_path(const char path[PATH_MAX], char resolved_path[PATH_MAX], bool b_file_is_new, int8_t *p_res)
{
    (void)path;
    (void)(resolved_path);
    (void)(b_file_is_new);
    (void)(p_res);
    memcpy(resolved_path, path, strnlen(path, PATH_MAX));

    *p_res = SUCCESS;
    return EMBER_SUCCESS;
}

int file_open_for_reading(const char resolved_path[PATH_MAX], struct stat *p_read_stat, uint64_t max_size,
                          int8_t *p_res, int *p_err)
{
    (void)(resolved_path);
    (void)(p_read_stat);
    (void)(max_size);
    (void)(p_res);
    (void)(p_err);

    *p_res = SUCCESS;
    *p_err = EMBER_SUCCESS;
    return EMBER_SUCCESS;
}

int file_open_for_writing(const char resolved_path[PATH_MAX], uint16_t perms, bool b_overwrite, int8_t *p_res,
                          int *p_err)
{
    (void)(resolved_path);
    (void)(perms);
    (void)(b_overwrite);
    (void)(p_res);
    (void)(p_err);
    *p_res = SUCCESS;
    *p_err = EMBER_SUCCESS;
    return EMBER_SUCCESS;
}

int file_read_in_chunks(const io_callback_t *p_reader, uint64_t num_bytes, int read_fd, int8_t *p_res)
{
    (void)(p_reader);
    (void)(num_bytes);
    (void)(read_fd);
    (void)(p_res);

    *p_res = SUCCESS;
    return EMBER_SUCCESS;
}

int file_write_in_chunks(const io_callback_t *p_writer, uint64_t num_bytes, int write_fd, int8_t *p_res)
{
    (void)(p_writer);
    (void)(num_bytes);
    (void)(write_fd);
    (void)(p_res);

    *p_res = SUCCESS;
    return EMBER_SUCCESS;
}
