#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#include "exec.h"
#include "file.h"
#include "settings.h"

#define TASK_HDR_LEN 18

enum op_codes
{
    SETTINGS = 1,
    EXEC,
    DOWNLOAD,
    UPLOAD,
    DISCONNECT,
    EXIT
};

typedef struct task_header_t
{
    uint8_t op_code;
    uint8_t pad_len;
    uint16_t flags;
    uint16_t perms;
    uint32_t data_len;
    uint64_t file_len;
} task_header_t;

typedef struct task_t
{
    task_header_t hdr;
    uint8_t *raw_data;
    settings_t settings;
    exec_t exec;
    file_t file;
    int8_t response_code;
} task_t;

int task_receive_and_execute(int sock, settings_t *p_settings);

#endif
