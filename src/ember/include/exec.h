#ifndef EXEC_H
#define EXEC_H

#include <linux/limits.h>
#include <stddef.h>
#include <stdint.h>

#include "io_callback.h"

enum exec_flags
{
    IN_MEM = 1,
    BACKGROUND = 4,
    STDIN = 8,
    PATH = 16,
    ARGV = 32,
    ENVP = 64,
    TIMEOUT = 128,
};

typedef struct
{
    char path[PATH_MAX];
    int mem_fd;

    uint8_t *stdin_data;
    size_t stdin_len;

    char *envp[UINT8_MAX + 1];
    char *argv[UINT8_MAX + 1];
} exec_t;

int exec_receive_payload(io_callback_t *p_receiver, exec_t *p_exec, uint64_t file_len, int8_t *p_res);

int exec_run(io_callback_t *p_sender, uint16_t flags, exec_t *p_exec, int8_t *p_res);

#endif
