#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>

typedef struct task_header_t task_header_t;
typedef struct task_t task_t;

int deserialize_task(task_t *p_dest);

void deserialize_task_header(task_header_t *p_hdr, const uint8_t *recv_buf);

#endif
