/**
 * @file io_callback.h
 * @author Kevin McKenzie
 * @brief  Generic callback that both loader.c and file.c invoke to stream output/input in chunks via the user specified
 * function pointer. The first pointer arg is the data required for the function to execute, e.g. Some IO vector
 * like a socket. The second argument is the buffer data will be written to/read from, and the third is the amount to
 * read/send.
 */
#ifndef IO_CALLBACK_H
#define IO_CALLBACK_H

#include <stdint.h>
#include <sys/types.h>

typedef struct
{
    int (*func)(void *, uint8_t *, ssize_t); /**< User defined callback per description above.*/
    void *data;                              /**< Pointer to data to pass into the callback on invocation. */
} io_callback_t;

#endif /* IO_CALLBACK_H */

/*** END OF FILE ***/
