#define _POSIX_C_SOURCE 200809L // NOLINT

#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#include "ember.h"
#include "errors.h"
#include "settings.h"
#include "utils.h"

static settings_t g_initial_settings = {0};

enum
{
    PORT = 31337
};

int main(void)
{
    // Temporary until stamping is added
    g_initial_settings.guid[0] = 12345;
    g_initial_settings.mode = BEACON;
    g_initial_settings.interval.tv_sec = 1;
    g_initial_settings.interval.tv_nsec = 0;
    g_initial_settings.callback_location.sin_addr.s_addr = inet_addr("127.0.0.1");
    g_initial_settings.callback_location.sin_port = htons(PORT);
    g_initial_settings.callback_location.sin_family = AF_INET;

    int ret = EMBER_SUCCESS;

    // TODO: check other reliable syscalls: getrandom, etc.
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &(struct timespec){0}))
    {
        DEBUG_PERROR("clock_monotonic() not supported");
        ret = EMBER_ERROR;
    }

    if (EMBER_SUCCESS == ret)
    {
        ret = ember_run(g_initial_settings);
    }

    if (EMBER_SUCCESS != ret)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
