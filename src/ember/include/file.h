#ifndef FILE_H
#define FILE_H

#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "io_callback.h"

enum file_flags
{
    OVERWRITE = 1,
};

typedef struct
{
    char path[PATH_MAX];
} file_t;

int file_resolve_path(const char path[PATH_MAX], char resolved_path[PATH_MAX], bool b_file_is_new, int8_t *p_res);

int file_open_for_reading(const char resolved_path[PATH_MAX], struct stat *p_read_stat, uint64_t max_size,
                          int8_t *p_res, int *p_err);

int file_open_for_writing(const char resolved_path[PATH_MAX], uint16_t perms, bool b_overwrite, int8_t *p_res,
                          int *p_err);

int file_read_in_chunks(const io_callback_t *p_reader, uint64_t num_bytes, int read_fd, int8_t *p_res);

int file_write_in_chunks(const io_callback_t *p_writer, uint64_t num_bytes, int write_fd, int8_t *p_res);

#endif
