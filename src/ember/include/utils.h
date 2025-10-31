/**
 * @file utils.h
 * @author Kevin McKenzie
 * @brief `utils` keeps a global store of all includes required to build the
 * project, as well as defining utility functions/syscall wrappers used across the project.
 */
#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
    DECIMAL = 10,
    ONE_HUNDRED = 100,
    ONE_MILLION = 1000000,
    MAX_PERMS = 0777,
    RECV_TIMEOUT = 5,
};

#ifndef NDEBUG
#define DEBUG_MSG(msg) (void)fprintf(stderr, "DEBUG: %s:%d:%s(): " msg "\n", __FILE__, __LINE__, __func__)
#define DEBUG_PRINT(fmt, ...)                                                                                          \
    (void)fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG_PERROR(msg)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        const char *err_str = strerror(errno); /* NOLINT */                                                            \
        (void)fprintf(stderr, "DEBUG: %s:%d:%s(): %s: %s\n", __FILE__, __LINE__, __func__, err_str, msg);              \
    } while (0)
#else
#define DEBUG_MSG(msg)
#define DEBUG_PRINT(fmt, ...)
#define DEBUG_PERROR(msg)
#endif

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/** Pointers should never be freed without NULL assignment. */
#define utils_free(ptr)                                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        free(ptr);                                                                                                     \
        (ptr) = NULL;                                                                                                  \
    } while (0)

/**
 * @brief Validate whether a string contains standard ASCII scale printable
 * non whitespace characters (0x21-0x7E).
 * @param buf Buffer of bytes to be checked
 * @param buf_len Length of expected string not including NULL byte
 * @return true buf is a valid string
 * @return false buf is not a valid string
 */
bool utils_is_valid_str_buf(const uint8_t *buf, uint32_t buf_len);

uint64_t utils_htonll(uint64_t host_long);

uint64_t utils_ntohll(uint64_t net_long);

ssize_t utils_writeall(int write_fd, void *src, size_t len);

ssize_t utils_readall(int read_fd, void *dest, size_t read_size);

ssize_t utils_recvall(int sock, void *dest, size_t num_bytes, int flags);

ssize_t utils_sendall(int sock, void *src, size_t len, int flags);

#endif /* UTILS_H */

/*** END OF FILE ***/
