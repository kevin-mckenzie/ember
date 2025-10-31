
/**
 * @file utils.c
 * @author Kevin McKenzie
 * @brief `utils` keeps a global store of all includes required to build the
 * project, as well as defining utility functions/syscall wrappers used across the project.
 */
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

/**
 * @brief Validate whether a string contains standard ASCII scale printable characters (0x20-0x7E).
 * @param buf Buffer of bytes to be checked
 * @param buf_len Length of expected string not including NULL byte
 * @return true buf is a valid string
 * @return false buf is not a valid string
 */
bool utils_is_valid_str_buf(const uint8_t *buf, uint32_t buf_len)
{
    bool b_is_valid = false;

    if (NULL != buf) // NOLINT (misc-include-cleaner) (false positive)
    {
        b_is_valid = true;

        // May only contain printable characters
        for (uint32_t idx = 0; idx < buf_len; idx++)
        {
            if (('\0' != buf[idx]) && ((' ' > buf[idx]) || ('~' < buf[idx])))
            {
                b_is_valid = false;
                break;
            }
        }
    }

    return b_is_valid;
}

ssize_t utils_writeall(int write_fd, void *src, size_t len)
{
    ssize_t total_written = 0;

    while ((size_t)total_written < len)
    {
        ssize_t written = write(write_fd, (uint8_t *)src + total_written, len - (size_t)total_written);
        if (0 < written)
        {
            total_written += written;
        }
        else
        {
            if (-1 == written)
            {
                total_written = -1;
            }
            break;
        }
    }

    return total_written;
}

ssize_t utils_readall(int read_fd, void *dest, size_t read_size)
{
    ssize_t total_read = 0;
    while ((size_t)total_read < read_size)
    {
        ssize_t this_read = read(read_fd, (uint8_t *)dest + total_read, read_size - (size_t)total_read);
        if (0 < this_read)
        {
            total_read += this_read;
        }
        else
        {
            if (-1 == this_read)
            {
                total_read = -1;
            }
            break;
        }
    }

    return total_read;
}

ssize_t utils_recvall(int sock, void *dest, size_t num_bytes, int flags)
{
    ssize_t total_recvd = 0;
    while ((size_t)total_recvd < num_bytes)
    {
        DEBUG_PRINT("%d %lu", sock, num_bytes - (size_t)total_recvd);
        ssize_t recvd = recv(sock, (uint8_t *)dest + total_recvd, num_bytes - (size_t)total_recvd, flags);
        if (0 < recvd)
        {
            total_recvd += recvd;
        }
        else
        {
            DEBUG_PERROR("recv");
            total_recvd = -1;
            break;
        }
    }

    return total_recvd;
}

ssize_t utils_sendall(int sock, void *src, size_t len, int flags)
{
    ssize_t total_sent = 0;

    while ((size_t)total_sent < len)
    {
        ssize_t sent = send(sock, (uint8_t *)src + total_sent, len - (size_t)total_sent, flags);

        if (0 < sent)
        {
            total_sent += sent;
        }
        else
        {
            DEBUG_PERROR("send");
            total_sent = -1;
            break;
        }
    }

    return total_sent;
}

uint64_t utils_ntohll(uint64_t net_long)
{
    uint64_t result = 0;

#if (defined(__BYTE_ORDER) && (__BYTE_ORDER == __ORDER_LITTLE_ENDIAN__) || (_BYTE_ORDER == __ORDER_LITTLE_ENDIAN__))
    const uint32_t uint32_bits = 32;
    const uint32_t uint32_mask = 0xFFFFFFFF;
    uint32_t hi_uint32 = ntohl((uint32_t)(net_long >> uint32_bits));
    uint32_t lo_uint32 = ntohl((uint32_t)(net_long & uint32_mask));
    result = (((uint64_t)lo_uint32) << uint32_bits) | ((uint64_t)hi_uint32);
#else
    result = net_long;
#endif

    return result;
}

uint64_t utils_htonll(uint64_t host_long)
{
    uint64_t result = 0;

#if (defined(__BYTE_ORDER) && (__BYTE_ORDER == __ORDER_LITTLE_ENDIAN__) || (_BYTE_ORDER == __ORDER_LITTLE_ENDIAN__))
    const uint32_t uint32_bits = 32;
    const uint32_t uint32_mask = 0xFFFFFFFF;
    uint32_t hi_uint32 = htonl((uint32_t)(host_long >> uint32_bits));
    uint32_t lo_uint32 = htonl((uint32_t)(host_long & uint32_mask));
    result = (((uint64_t)lo_uint32) << uint32_bits) | ((uint64_t)hi_uint32);
#else
    result = host_long;
#endif

    return result;
}

/*** END OF FILE ***/
