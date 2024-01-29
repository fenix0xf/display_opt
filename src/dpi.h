#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct dpi_scale_factor
{
    uint32_t scale_def;
    uint32_t scale_min;
    uint32_t scale;
    uint32_t scale_max;
} dpi_scale_factor_t;

bool        dpi_scale_factor_get(const wchar_t* gdi_device, dpi_scale_factor_t* dpi_scale_factor);
bool        dpi_scale_factor_set(const wchar_t* gdi_device, uint32_t scale);
bool        dpi_scale_factor_is_valid(const dpi_scale_factor_t* dpi_scale_factor, uint32_t scale);
const char* dpi_scale_factor_list_get(const dpi_scale_factor_t* dpi_scale_factor);
