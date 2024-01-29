#pragma once

#include <stdint.h>
#include <stdbool.h>

#define VIDEO_ADAPTER_GDI_DEVICE_LEN (32)

void video_adapter_info_print_all(void);
bool video_adapter_resolution_set(uint32_t adapter_idx, uint32_t width, uint32_t height);
bool video_adapter_index_to_device_name(uint32_t adapter_idx, wchar_t gdi_device[VIDEO_ADAPTER_GDI_DEVICE_LEN]);
