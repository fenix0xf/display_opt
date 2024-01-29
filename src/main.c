#include "video_adapter.h"
#include "dpi.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, const char** argv)
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc < 5)
    {
        video_adapter_info_print_all();

        printf("Usage: display_opt adapter_index resolution_width resolution_height scale_factor_pct\n");
        return 0;
    }

    uint32_t adapter_index     = strtoul(argv[1], NULL, 10);
    uint32_t resolution_width  = strtoul(argv[2], NULL, 10);
    uint32_t resolution_height = strtoul(argv[3], NULL, 10);
    uint32_t scale_factor_pct  = strtoul(argv[4], NULL, 10);

    wchar_t  gdi_device[VIDEO_ADAPTER_GDI_DEVICE_LEN];

    if (!video_adapter_index_to_device_name(adapter_index, gdi_device))
    {
        printf("video_adapter_index_to_device_name(adapter_index %u) error\n");
        return 1;
    }

    if (!video_adapter_resolution_set(adapter_index, resolution_width, resolution_height))
    {
        printf("video_adapter_index_to_device_name(adapter_index %u) error\n");
        return 2;
    }

    dpi_scale_factor_t dpi_scale_factor;

    if (!dpi_scale_factor_get(gdi_device, &dpi_scale_factor))
    {
        printf("dpi_scale_factor_get(\"%ls\") error\n", gdi_device);
        return 3;
    }

    if (!dpi_scale_factor_is_valid(&dpi_scale_factor, scale_factor_pct))
    {
        printf("Scale factor %u%% is invalid for GDI device \"%ls\"\n", (unsigned)scale_factor_pct, gdi_device);
        return 4;
    }

    if (!dpi_scale_factor_set(gdi_device, scale_factor_pct))
    {
        printf("dpi_scale_factor_set(%ls, %u%%) error\n", gdi_device, (unsigned)scale_factor_pct);
        return 5;
    }

    return 0;
}
