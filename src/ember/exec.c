
#include "exec.h"
#include "codes.h"
#include "errors.h"
#include "io_callback.h"
#include "stdint.h"

int exec_receive_payload(io_callback_t *p_receiver, exec_t *p_exec, uint64_t file_len, int8_t *p_res)
{
    (void)p_receiver;
    (void)p_exec;
    (void)file_len;
    (void)p_res;

    *p_res = SUCCESS;
    return EMBER_SUCCESS;
}

int exec_run(io_callback_t *p_sender, uint16_t flags, exec_t *p_exec, int8_t *p_res)
{
    (void)p_sender;
    (void)flags;
    (void)p_exec;
    (void)p_res;

    *p_res = SUCCESS;
    return EMBER_SUCCESS;
}
