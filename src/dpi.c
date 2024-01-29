#include "dpi.h"

#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <shtypes.h>

#define DPI_PRINT_ERRORS         1
#define DPI_SCALE_FACTOR_TIMEOUT 4000

typedef enum
{
    DISPLAYCONFIG_DEVICE_INFO_GET_RELATIVE_SCALE_FACTOR_INDEX = (-3),
    DISPLAYCONFIG_DEVICE_INFO_SET_RELATIVE_SCALE_FACTOR_INDEX = (-4),
} DISPLAYCONFIG_DEVICE_INFO_TYPE_RELATIVE_SCALE_FACTOR_INDEX;

typedef struct DISPLAYCONFIG_GET_RELATIVE_SCALE_FACTOR_INDEX
{
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    INT32                            indexMin;
    INT32                            index;
    INT32                            indexMax;
} DISPLAYCONFIG_GET_RELATIVE_SCALE_FACTOR_INDEX;

typedef struct DISPLAYCONFIG_SET_RELATIVE_SCALE_FACTOR_INDEX
{
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    INT32                            index;
} DISPLAYCONFIG_SET_RELATIVE_SCALE_FACTOR_INDEX;

typedef struct display
{
    LUID   adapter_id;
    UINT32 source_id;
} display_t;

static const uint32_t g_dpi_scale[] = {
    SCALE_100_PERCENT,
    SCALE_125_PERCENT,
    SCALE_150_PERCENT,
    SCALE_175_PERCENT,
    SCALE_200_PERCENT,
    SCALE_225_PERCENT,
    SCALE_250_PERCENT,
    SCALE_300_PERCENT,
    SCALE_350_PERCENT,
    SCALE_400_PERCENT,
    SCALE_450_PERCENT,
    SCALE_500_PERCENT,
};

#define countof(a) (sizeof(a) / sizeof(*(a)))

static __thread char g_scale_factor_list[countof(g_dpi_scale) * 8];

#if !DPI_PRINT_ERRORS
#define printf(...)
#endif

static bool _dpi_scale_to_factor(const DISPLAYCONFIG_GET_RELATIVE_SCALE_FACTOR_INDEX* relative,
                                 dpi_scale_factor_t*                                  dpi_scale_factor)
{
    int32_t idx_def = abs(relative->indexMin);

    if (idx_def >= (int32_t)countof(g_dpi_scale))
    {
        return false;
    }

    int32_t idx_min = idx_def + relative->indexMin;

    if ((idx_min < 0) || (idx_min >= (int32_t)countof(g_dpi_scale)))
    {
        return false;
    }

    int32_t idx = idx_def + relative->index;

    if ((idx < 0) || (idx >= (int32_t)countof(g_dpi_scale)))
    {
        return false;
    }

    int32_t idx_max = idx_def + relative->indexMax;

    if ((idx_max < 0) || (idx_max >= (int32_t)countof(g_dpi_scale)))
    {
        return false;
    }

    dpi_scale_factor->scale_def = g_dpi_scale[idx_def];
    dpi_scale_factor->scale_min = g_dpi_scale[idx_min];
    dpi_scale_factor->scale     = g_dpi_scale[idx];
    dpi_scale_factor->scale_max = g_dpi_scale[idx_max];

    return true;
}

static int32_t _dpi_scale_to_relative(const dpi_scale_factor_t* dpi_scale_factor, uint32_t scale)
{
    int32_t idx     = countof(g_dpi_scale);
    int32_t idx_def = countof(g_dpi_scale);

    for (int32_t i = 0; i < (int32_t)countof(g_dpi_scale); i++)
    {
        uint32_t dpi_scale = g_dpi_scale[i];

        if (dpi_scale == scale)
        {
            idx = i;
        }

        if (dpi_scale == dpi_scale_factor->scale_def)
        {
            idx_def = i;
        }
    }

    if ((idx == (int32_t)countof(g_dpi_scale)) || (idx_def == (int32_t)countof(g_dpi_scale)))
    {
        return 0;
    }

    return idx - idx_def;
}

static bool _dpi_display_find(const wchar_t* gdi_device, display_t* display)
{
    for (;;)
    {
        LONG   ret;
        UINT32 paths_num;
        UINT32 modes_num;

        ret = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &paths_num, &modes_num);

        if (ret != ERROR_SUCCESS)
        {
            printf("GetDisplayConfigBufferSizes() error %ld\n", ret);
            return false;
        }

        DISPLAYCONFIG_PATH_INFO paths[paths_num];
        DISPLAYCONFIG_MODE_INFO modes[modes_num];

        ret = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &paths_num, paths, &modes_num, modes, NULL);

        if (ret != ERROR_SUCCESS)
        {
            if (ret == ERROR_INSUFFICIENT_BUFFER)
            {
                continue;
            }

            printf("QueryDisplayConfig() error %ld\n", ret);
            return false;
        }

        for (size_t idx = 0; idx < paths_num; idx++)
        {
            DISPLAYCONFIG_SOURCE_DEVICE_NAME device_name = {
                .header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,
                .header.size      = sizeof(device_name),
                .header.adapterId = paths[idx].targetInfo.adapterId,
                .header.id        = paths[idx].sourceInfo.id,
            };

            ret = DisplayConfigGetDeviceInfo(&device_name.header);

            if (ret != ERROR_SUCCESS)
            {
                printf("DisplayConfigGetDeviceInfo(device_name) error %ld\n", ret);
                return false;
            }

            if (wcsncmp(gdi_device, device_name.viewGdiDeviceName, countof(device_name.viewGdiDeviceName)) != 0)
            {
                continue;
            }

            display->adapter_id = paths[idx].targetInfo.adapterId;
            display->source_id  = paths[idx].sourceInfo.id;

            return true;
        }

        return false;
    }
}

static bool _dpi_scale_factor_get(const display_t* display, dpi_scale_factor_t* dpi_scale_factor)
{
    DISPLAYCONFIG_GET_RELATIVE_SCALE_FACTOR_INDEX relative_scale_factor = {
        .header.type      = (DISPLAYCONFIG_DEVICE_INFO_TYPE)DISPLAYCONFIG_DEVICE_INFO_GET_RELATIVE_SCALE_FACTOR_INDEX,
        .header.size      = sizeof(relative_scale_factor),
        .header.adapterId = display->adapter_id,
        .header.id        = display->source_id,
    };

    LONG ret = DisplayConfigGetDeviceInfo(&relative_scale_factor.header);

    if (ret != ERROR_SUCCESS)
    {
        printf("DisplayConfigGetDeviceInfo(relative_scale_factor) error %ld\n", ret);
        return false;
    }

    return _dpi_scale_to_factor(&relative_scale_factor, dpi_scale_factor);
}

bool dpi_scale_factor_get(const wchar_t* gdi_device, dpi_scale_factor_t* dpi_scale_factor)
{
    if (!gdi_device || !dpi_scale_factor)
    {
        return false;
    }

    display_t display;

    if (!_dpi_display_find(gdi_device, &display))
    {
        return false;
    }

    return _dpi_scale_factor_get(&display, dpi_scale_factor);
}

bool dpi_scale_factor_set(const wchar_t* gdi_device, uint32_t scale)
{
    if (!gdi_device)
    {
        return false;
    }

    display_t display;

    if (!_dpi_display_find(gdi_device, &display))
    {
        return false;
    }

    dpi_scale_factor_t dpi_scale_factor;

    if (!_dpi_scale_factor_get(&display, &dpi_scale_factor))
    {
        return false;
    }

    if (scale > dpi_scale_factor.scale)
    {
        printf("Applying workaround for windows explorer taskbar blur issue, temporary switching to the max scale "
               "factor %u%%...",
               (unsigned)dpi_scale_factor.scale_max);

        DISPLAYCONFIG_SET_RELATIVE_SCALE_FACTOR_INDEX relative_scale_factor = {
            .header.type = (DISPLAYCONFIG_DEVICE_INFO_TYPE)DISPLAYCONFIG_DEVICE_INFO_SET_RELATIVE_SCALE_FACTOR_INDEX,
            .header.size = sizeof(relative_scale_factor),
            .header.adapterId = display.adapter_id,
            .header.id        = display.source_id,
            .index            = _dpi_scale_to_relative(&dpi_scale_factor, dpi_scale_factor.scale_max),
        };

        LONG ret = DisplayConfigSetDeviceInfo(&relative_scale_factor.header);

        if (ret != ERROR_SUCCESS)
        {
            printf("[failed, DisplayConfigSetDeviceInfo(relative_scale_factor) error %ld]\n", ret);
            return false;
        }

        printf("[done]\n");

        printf("Waiting for %u milliseconds...", DPI_SCALE_FACTOR_TIMEOUT);
        Sleep(DPI_SCALE_FACTOR_TIMEOUT);
        printf("[done]\n");
    }

    printf("Applying scale factor %u%% to device %ls...", (unsigned)scale, gdi_device);

    DISPLAYCONFIG_SET_RELATIVE_SCALE_FACTOR_INDEX relative_scale_factor = {
        .header.type      = (DISPLAYCONFIG_DEVICE_INFO_TYPE)DISPLAYCONFIG_DEVICE_INFO_SET_RELATIVE_SCALE_FACTOR_INDEX,
        .header.size      = sizeof(relative_scale_factor),
        .header.adapterId = display.adapter_id,
        .header.id        = display.source_id,
        .index            = _dpi_scale_to_relative(&dpi_scale_factor, scale),
    };

    LONG ret = DisplayConfigSetDeviceInfo(&relative_scale_factor.header);

    if (ret != ERROR_SUCCESS)
    {
        printf("[failed, DisplayConfigSetDeviceInfo(relative_scale_factor) error %ld]\n", ret);
        return false;
    }

    printf("[done]\n");
    return true;
}

bool dpi_scale_factor_is_valid(const dpi_scale_factor_t* dpi_scale_factor, uint32_t scale)
{
    if (!dpi_scale_factor || ((scale < dpi_scale_factor->scale_min) || (scale > dpi_scale_factor->scale_max)))
    {
        return false;
    }

    for (size_t i = 0; i < countof(g_dpi_scale); i++)
    {
        uint32_t dpi_scale = g_dpi_scale[i];

        if ((dpi_scale < dpi_scale_factor->scale_min) || (dpi_scale > dpi_scale_factor->scale_max))
        {
            continue;
        }

        if (scale == dpi_scale)
        {
            return true;
        }
    }

    return false;
}

const char* dpi_scale_factor_list_get(const dpi_scale_factor_t* dpi_scale_factor)
{
    if (!dpi_scale_factor)
    {
        return "Error";
    }

    g_scale_factor_list[0] = 0;

    enum
    {
        ACTION_SEARCH,
        ACTION_APPEND,
        ACTION_APPEND_SELECTED,
    } action = ACTION_SEARCH;

    for (size_t i = 0; i < countof(g_dpi_scale); i++)
    {
        if ((action == ACTION_SEARCH) && (g_dpi_scale[i] == dpi_scale_factor->scale_min))
        {
            action = ACTION_APPEND;
        }

        if ((action == ACTION_APPEND) && (g_dpi_scale[i] == dpi_scale_factor->scale))
        {
            action = ACTION_APPEND_SELECTED;
        }

        if ((action == ACTION_APPEND_SELECTED) && (g_dpi_scale[i] != dpi_scale_factor->scale))
        {
            action = ACTION_APPEND;
        }

        const char* fmt;
        bool        is_last_item = (i == (countof(g_dpi_scale) - 1)) || (g_dpi_scale[i] == dpi_scale_factor->scale_max);

        switch (action)
        {
            case ACTION_SEARCH: continue;
            case ACTION_APPEND: fmt = is_last_item ? "%u%%" : "%u%%, "; break;
            case ACTION_APPEND_SELECTED: fmt = is_last_item ? "[%u%%]" : "[%u%%], "; break;
        }

        char s[32];
        sprintf_s(s, sizeof(s), fmt, (unsigned)g_dpi_scale[i]);
        strcat_s(g_scale_factor_list, sizeof(g_scale_factor_list), s);

        if (is_last_item)
        {
            break;
        }
    }

    return g_scale_factor_list;
}
