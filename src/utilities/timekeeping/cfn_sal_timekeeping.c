/**
 * @file cfn_sal_timekeeping.c
 * @brief High-level timekeeping and epoch management service implementation.
 */

#include "utilities/cfn_sal_timekeeping.h"
#include "cfn_hal_timer.h"
#include "cfn_hal_rtc.h"
#include <time.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>
#endif

/* VMT Implementations ----------------------------------------------*/

static cfn_hal_error_code_t timekeeping_set_time(cfn_sal_timekeeping_t *driver, time_t timestamp)
{
    if (driver->phy && driver->phy->type == CFN_HAL_PERIPHERAL_TYPE_RTC)
    {
        cfn_hal_rtc_t *rtc     = (cfn_hal_rtc_t *) driver->phy->handle;
        struct tm     *tm_info = gmtime(&timestamp);
        return cfn_hal_rtc_set_time(rtc, tm_info);
    }
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t timekeeping_get_time(cfn_sal_timekeeping_t *driver, time_t *timestamp_out)
{
    if (!timestamp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (driver->phy && driver->phy->type == CFN_HAL_PERIPHERAL_TYPE_RTC)
    {
        cfn_hal_rtc_t       *rtc = (cfn_hal_rtc_t *) driver->phy->handle;
        struct tm            tm_info;
        cfn_hal_error_code_t err = cfn_hal_rtc_get_time(rtc, &tm_info);
        if (err == CFN_HAL_ERROR_OK)
        {
            *timestamp_out = mktime(&tm_info);
        }
        return err;
    }

#if defined(__unix__) || defined(__APPLE__)
    *timestamp_out = time(NULL);
    return CFN_HAL_ERROR_OK;
#else
    return CFN_HAL_ERROR_NOT_SUPPORTED;
#endif
}

static cfn_hal_error_code_t timekeeping_get_ms(cfn_sal_timekeeping_t *driver, uint64_t *ms_out)
{
    if (!ms_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (driver->phy && driver->phy->type == CFN_HAL_PERIPHERAL_TYPE_TIMER)
    {
        cfn_hal_timer_t *tim = (cfn_hal_timer_t *) driver->phy->handle;
        /* Channel 0 used by convention for system uptime if not specified */
        return cfn_hal_timer_get_ticks_u64(tim, 0, ms_out);
    }

#if defined(__unix__) || defined(__APPLE__)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *ms_out = (uint64_t) tv.tv_sec * 1000ULL + (uint64_t) tv.tv_usec / 1000ULL;
    return CFN_HAL_ERROR_OK;
#else
    return CFN_HAL_ERROR_NOT_SUPPORTED;
#endif
}

static cfn_hal_error_code_t timekeeping_sync_now(cfn_sal_timekeeping_t *driver)
{
    CFN_HAL_UNUSED(driver);
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t timekeeping_is_synchronized(cfn_sal_timekeeping_t *driver, bool *is_sync_out)
{
    CFN_HAL_UNUSED(driver);
    if (is_sync_out)
    {
        *is_sync_out = false;
    }
    return CFN_HAL_ERROR_OK;
}

static const cfn_sal_timekeeping_api_t TIMEKEEPING_API = {
    .base = {
        .init = NULL,
        .deinit = NULL,
    },
    .set_time = timekeeping_set_time,
    .get_time = timekeeping_get_time,
    .get_ms   = timekeeping_get_ms,
    .sync_now = timekeeping_sync_now,
    .is_synchronized = timekeeping_is_synchronized,
};

/* Public Functions -------------------------------------------------*/

cfn_hal_error_code_t cfn_sal_timekeeping_construct(cfn_sal_timekeeping_t              *driver,
                                                   const cfn_sal_timekeeping_config_t *config,
                                                   const cfn_sal_phy_t                *phy,
                                                   void                               *dependency,
                                                   cfn_sal_timekeeping_callback_t      callback,
                                                   void                               *user_arg)
{
    if (!driver)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    cfn_sal_timekeeping_populate(driver, 0, dependency, &TIMEKEEPING_API, phy, config, callback, user_arg);
    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_timekeeping_destruct(cfn_sal_timekeeping_t *driver)
{
    if (!driver)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_timekeeping_populate(driver, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    driver->base.status = CFN_HAL_DRIVER_STATUS_UNKNOWN;
    return CFN_HAL_ERROR_OK;
}
