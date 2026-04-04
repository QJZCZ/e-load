/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: MIT License
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-15     luhuadong    the first version
 */

#ifndef __SSD1306_FONTS_H__
#define __SSD1306_FONTS_H__

#include <stdint.h>

// Enumeration for font sizes
typedef enum {
    FONT_6x8,
    FONT_7x10,
    FONT_11x18,
    FONT_16x26
} FontSize;

// Font structure definition
typedef struct {
    const uint8_t FontWidth;    /*!< Font width in pixels */
    const uint8_t FontHeight;   /*!< Font height in pixels */
    const uint16_t *data;       /*!< Pointer to font data array */
} FontDef_t;

// Export font structures
extern FontDef_t Font_6x8;
extern FontDef_t Font_7x10;
extern FontDef_t Font_11x18;
extern FontDef_t Font_16x26;

#endif /* __SSD1306_FONTS_H__ */