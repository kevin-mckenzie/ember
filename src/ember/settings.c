#include <stdbool.h>
#include <stdint.h>

#include "codes.h"
#include "settings.h"
#include "utils.h"

bool is_valid_settings_update(uint16_t flags, settings_t *p_settings)
{
    bool b_is_valid = true;

    if ((uint16_t)INTERVAL & flags)
    {
        if (INT32_MAX < p_settings->interval.tv_sec)
        {
            b_is_valid = false;
        }
    }

    if ((uint16_t)WINDOW & flags)
    {
        if (INT32_MAX < p_settings->window.tv_sec)
        {
            b_is_valid = false;
        }
    }

    if ((uint16_t)MODE & flags)
    {
        if ((BEACON != p_settings->mode) && (TRIGGER != p_settings->mode))
        {
            b_is_valid = false;
        }
    }

    return b_is_valid;
}

int8_t settings_update(uint16_t flags, settings_t *p_src, settings_t *p_dest)
{
    int8_t ret = SUCCESS;

    if (is_valid_settings_update(flags, p_src))
    {
        if ((uint16_t)INTERVAL & flags)
        {
            DEBUG_PRINT("new beacon interval: %ld", p_src->interval.tv_sec);
            p_dest->interval.tv_sec = p_src->interval.tv_sec;

            // When beacon interval gets changed, reset start time for callbacks.
            (void)clock_gettime(CLOCK_MONOTONIC, &p_dest->next_callback);
        }

        if ((uint16_t)WINDOW & flags)
        {
            p_dest->window.tv_sec = p_src->window.tv_sec;
        }

        if ((uint16_t)CALLBACK & flags)
        {
            p_dest->callback_location.sin_addr.s_addr = p_src->callback_location.sin_addr.s_addr;
            p_dest->callback_location.sin_port = p_src->callback_location.sin_port;
        }

        if ((uint16_t)MODE & flags)
        {
            p_dest->mode = p_src->mode;
        }

        if ((uint16_t)SEED & flags)
        {
            p_dest->seed = p_src->seed;
        }
    }
    else
    {
        ret = -INVALID_CONFIG;
    }

    return ret;
}
