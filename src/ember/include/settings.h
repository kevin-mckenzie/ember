#ifndef SETTINGS_H
#define SETTINGS_H

#include <netinet/in.h>
#include <stdint.h>
#include <time.h>

enum modes
{
    BEACON = 0,
    TRIGGER
};

enum settings_flags
{
    INTERVAL = 1,
    WINDOW = 2,
    CALLBACK = 4,
    MODE = 16,
    SEED = 32,
};

typedef struct
{
    uint32_t guid[4];
    struct timespec next_callback;
    struct timespec interval;
    struct timespec window;
    struct sockaddr_in callback_location;
    uint8_t mode;
    uint32_t seed;
} settings_t;

int8_t settings_update(uint16_t flags, settings_t *p_src, settings_t *p_dest);

#endif
