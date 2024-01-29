#include "video_adapter.h"
#include "dpi.h"

#include <stdio.h>
#include <windows.h>

#define VIDEO_ADAPTER_PRINTF_WIDTH (-14)

#define countof(a)                 (sizeof(a) / sizeof(*(a)))

static bool _video_adapter_params_print(const wchar_t* gdi_device, int printf_width)
{
    printf("%*s: ", printf_width, "Parameters");

    DEVMODEW dev_mode = {.dmSize = sizeof(dev_mode)};

    if (!EnumDisplaySettingsW(gdi_device, ENUM_CURRENT_SETTINGS, &dev_mode))
    {
        printf("EnumDisplaySettingsW() error\n");
        return false;
    }

    if ((dev_mode.dmFields & DM_PELSWIDTH) && (dev_mode.dmFields & DM_PELSHEIGHT))
    {
        printf("%ux%u", (unsigned)dev_mode.dmPelsWidth, (unsigned)dev_mode.dmPelsHeight);
    }

    if (dev_mode.dmFields & DM_DISPLAYFREQUENCY)
    {
        printf(", %u Hz", (unsigned)dev_mode.dmDisplayFrequency);
    }

    if (dev_mode.dmFields & DM_BITSPERPEL)
    {
        printf(", %u bpp", (unsigned)dev_mode.dmBitsPerPel);
    }

    if (dev_mode.dmFields & DM_DISPLAYORIENTATION)
    {
        printf(", orientation %u deg", (unsigned)dev_mode.dmDisplayOrientation * 90);
    }

    printf("\n");
    return true;
}

void video_adapter_info_print_all(void)
{
    for (DWORD idx = 0;; idx++)
    {
        DISPLAY_DEVICEW adapter = {.cb = sizeof(adapter)};

        if (!EnumDisplayDevicesW(NULL, idx, &adapter, 0))
        {
            break;
        }

        if ((adapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
        {
            continue;
        }

        printf("%*s: %lu, %s, %ls (%ls)\n",
               VIDEO_ADAPTER_PRINTF_WIDTH,
               "Video adapter",
               idx,
               (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ? "Primary" : "Non-Primary",
               adapter.DeviceString,
               adapter.DeviceName);

        _video_adapter_params_print(adapter.DeviceName, VIDEO_ADAPTER_PRINTF_WIDTH);

        dpi_scale_factor_t dpi_scale_factor;

        if (dpi_scale_factor_get(adapter.DeviceName, &dpi_scale_factor))
        {
            printf("%*s: %u%% (%s)\n",
                   VIDEO_ADAPTER_PRINTF_WIDTH,
                   "Scale factor",
                   (unsigned)dpi_scale_factor.scale,
                   dpi_scale_factor_list_get(&dpi_scale_factor));
        }

        for (DWORD display_idx = 0;; display_idx++)
        {
            DISPLAY_DEVICEW display = {.cb = sizeof(display)};

            if (!EnumDisplayDevicesW(adapter.DeviceName, display_idx, &display, 0))
            {
                break;
            }

            printf("%*s: %ls (%ls)\n", VIDEO_ADAPTER_PRINTF_WIDTH, "Monitor", display.DeviceString, display.DeviceName);
        }

        printf("\n");
    }
}

bool video_adapter_resolution_set(uint32_t adapter_idx, uint32_t width, uint32_t height)
{
    for (DWORD idx = 0;; idx++)
    {
        DISPLAY_DEVICEW adapter = {.cb = sizeof(adapter)};

        if (!EnumDisplayDevicesW(NULL, idx, &adapter, 0))
        {
            break;
        }

        if ((idx != adapter_idx) || ((adapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0))
        {
            continue;
        }

        printf("Applying resolution %ux%u to device %ls...", (unsigned)width, (unsigned)height, adapter.DeviceName);

        DEVMODEW dev_mode = {
            .dmSize       = sizeof(dev_mode),
            .dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT,
            .dmPelsWidth  = (DWORD)width,
            .dmPelsHeight = (DWORD)height,
        };

        LONG ret = ChangeDisplaySettingsExW(adapter.DeviceName, &dev_mode, NULL, CDS_UPDATEREGISTRY, NULL);

        if (ret != DISP_CHANGE_SUCCESSFUL)
        {
            printf("[failed, ChangeDisplaySettingsExW() error %ld]\n", ret);
            return false;
        }

        printf("[done]\n");
        return true;
    }

    return false;
}

bool video_adapter_index_to_device_name(uint32_t adapter_idx, wchar_t gdi_device[VIDEO_ADAPTER_GDI_DEVICE_LEN])
{
    for (DWORD idx = 0;; idx++)
    {
        DISPLAY_DEVICEW adapter = {.cb = sizeof(adapter)};

        if (!EnumDisplayDevicesW(NULL, idx, &adapter, 0))
        {
            break;
        }

        if ((idx != adapter_idx) || ((adapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0))
        {
            continue;
        }

        wcscpy_s(gdi_device, VIDEO_ADAPTER_GDI_DEVICE_LEN, adapter.DeviceName);
        return true;
    }

    return false;
}
