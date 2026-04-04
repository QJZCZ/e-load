/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: MIT License
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-15     luhuadong    the first version
 * 2026-04-04     zcz          adapt for RT-Thread nano version
 */

#include <rtthread.h>
#include <string.h>
#include "ssd1306.h"
#include "i2c.h"
#include "pin.h"
#include <stdlib.h>

#if defined(SSD1306_USE_I2C)

#define SSD1306_CTRL_CMD     0x00
#define SSD1306_CTRL_DATA    0x40

static struct rt_i2c_bus_device *i2c_bus;

void ssd1306_Reset(void)
{
    /* for I2C - do nothing */
}

// Send a byte to the command register
void ssd1306_WriteCommand(uint8_t byte)
{
    uint8_t buf[2] = {SSD1306_CTRL_CMD, byte};
    rt_i2c_master_send(i2c_bus, SSD1306_I2C_ADDR, 0, buf, 2);
}

// Send data
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size)
{
    for (int i=0; i<buff_size; i++)
    {
        uint8_t buf[2] = {SSD1306_CTRL_DATA, buffer[i]};
        rt_i2c_master_send(i2c_bus, SSD1306_I2C_ADDR, 0, buf, 2);
    }
}

#else
#error "You should define SSD1306_USE_SPI or SSD1306_USE_I2C macro!"
#endif

// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];

// Screen object
static SSD1306_t SSD1306;

/* Fills the Screenbuffer with values from a given buffer of a fixed length */
SSD1306_Error_t ssd1306_FillBuffer(uint8_t* buf, uint32_t len)
{
    SSD1306_Error_t ret = SSD1306_ERR;
    if (len <= SSD1306_BUFFER_SIZE) {
        memcpy(SSD1306_Buffer,buf,len);
        ret = SSD1306_OK;
    }
    return ret;
}

// Initialize the oled screen
void ssd1306_Init(void)
{
#if defined(SSD1306_USE_I2C)
    rt_i2c_core_init();

    rt_pin_mode(15, PIN_MODE_OUTPUT);
    rt_pin_mode(23, PIN_MODE_OUTPUT);
    rt_pin_write(15, PIN_HIGH);
    rt_pin_write(23, PIN_HIGH);
    
    i2c_bus = rt_i2c_bus_device_find("i2c1");
    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("SSD1306: can not find %s device\n", SSD1306_I2C_BUS_NAME);
        return;
    }
#endif

    // Reset OLED
    ssd1306_Reset();

    // Wait for the screen to boot
    rt_thread_mdelay(100);

    // Init OLED
    ssd1306_SetDisplayOn(1); //display on

    ssd1306_WriteCommand(0x20); //Set Memory Addressing Mode
    ssd1306_WriteCommand(0x00); // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                // 10b,Page Addressing Mode (RESET); 11b,Invalid

    ssd1306_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7

#ifdef SSD1306_MIRROR_VERT
    ssd1306_WriteCommand(0xC0); // Mirror vertically
#else
    ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction
#endif

    ssd1306_WriteCommand(0x00); //---set low column address
    ssd1306_WriteCommand(0x10); //---set high column address

    ssd1306_WriteCommand(0x40); //--set start line address - CHECK

    ssd1306_SetContrast(0xFF);

#ifdef SSD1306_MIRROR_HORIZ
    ssd1306_WriteCommand(0xA0); // Mirror horizontally
#else
    ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
    ssd1306_WriteCommand(0xA7); //--set inverse color
#else
    ssd1306_WriteCommand(0xA6); //--set normal color
#endif

// Set multiplex ratio.
#if (SSD1306_HEIGHT == 128)
    // Found in the Luma Python lib for SH1106.
    ssd1306_WriteCommand(0xFF);
#else
    ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64) - CHECK
#endif

#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x1F); //
#elif (SSD1306_HEIGHT == 64)
    ssd1306_WriteCommand(0x3F); //
#elif (SSD1306_HEIGHT == 128)
    ssd1306_WriteCommand(0x3F); // Seems to work for 128px high displays too.
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    ssd1306_WriteCommand(0xD3); //-set display offset - CHECK
    ssd1306_WriteCommand(0x00); //-not offset

    ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); //--Set divide ratio

    ssd1306_WriteCommand(0xD9); //--set pre-charge period
    ssd1306_WriteCommand(0x22); //

    ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration - CHECK
#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x02);
#else
    ssd1306_WriteCommand(0x12);
#endif

    ssd1306_WriteCommand(0xDB); //--set vcomh
    ssd1306_WriteCommand(0x20); //0x20,0.77xVcc;0x30,0.83xVcc

    ssd1306_WriteCommand(0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(0x14); //

    ssd1306_WriteCommand(0xAF); //--turn on SSD1306 panel

    // Clear screen
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    SSD1306.Initialized = 1;
}

// Fill the whole screen with the given color
void ssd1306_Fill(SSD1306_COLOR color)
{
    uint32_t i;

    for(i = 0; i < sizeof(SSD1306_Buffer); i++)
    {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

// Update the display
void ssd1306_UpdateScreen(void)
{
    ssd1306_WriteData(SSD1306_Buffer, sizeof(SSD1306_Buffer));
}

//    Draw one pixel on the screen
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
    {
        return;
    }

    // Check if pixel should be inverted
    if(SSD1306.Inverted)
    {
        color = (color == Black) ? White : Black;
    }

    // Set color
    if(color == White)
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    }
    else
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

// Draw 1 char to the screen buffer
char ssd1306_WriteChar(char ch, FontDef_t Font, SSD1306_COLOR color)
{
    uint32_t i, b, j;

    // Check remaining space on current line
    if (SSD1306_WIDTH < (SSD1306.CurrentX + Font.FontWidth) ||
        SSD1306_HEIGHT < (SSD1306.CurrentY + Font.FontHeight))
    {
        return 0;
    }

    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++)
    {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for(j = 0; j < Font.FontWidth; j++)
        {
            if((b << j) & 0x8000)
            {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            }
            else
            {
                if(color == White)
                {
                    ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
                }
            }
        }
    }

    // The current space is now taken
    SSD1306.CurrentX += Font.FontWidth;

    // Return written char for validation
    return ch;
}

// Write full string to screen
char ssd1306_WriteString(char* str, FontDef_t Font, SSD1306_COLOR color)
{
    // Write until null-byte
    while(*str)
    {
        if (ssd1306_WriteChar(*str, Font, color) != *str)
        {
            // Char could not be written
            return *str;
        }

        // Next char
        str++;
    }

    // Everything ok
    return *str;
}

// Position the cursor
void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

// Draw line
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color)
{
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX = ((x1 < x2) ? 1 : -1);
    int32_t signY = ((y1 < y2) ? 1 : -1);
    int32_t error = deltaX - deltaY;
    int32_t error2;

    while(x1 != x2 || y1 != y2)
    {
        ssd1306_DrawPixel(x1, y1, color);
        error2 = 2 * error;
        if(error2 > -deltaY)
        {
            error -= deltaY;
            x1 += signX;
        }
        if(error2 < deltaX)
        {
            error += deltaX;
            y1 += signY;
        }
    }
}

// Draw arc
void ssd1306_DrawArc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color)
{
    // TODO: Implement arc drawing
}

// Draw circle
void ssd1306_DrawCircle(uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1306_COLOR color)
{
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if (par_x >= SSD1306_WIDTH || par_y >= SSD1306_HEIGHT)
    {
        return;
    }

    do
    {
        ssd1306_DrawPixel(par_x - x, par_y + y, color);
        ssd1306_DrawPixel(par_x + x, par_y + y, color);
        ssd1306_DrawPixel(par_x - x, par_y - y, color);
        ssd1306_DrawPixel(par_x + x, par_y - y, color);
        e2 = err;
        if (e2 <= y)
        {
            y += 1;
            err += y * 2 + 1;
            if(-x == y && e2 <= x)
            {
                e2 = x;
                x += 1;
                err += x * 2 + 1;
            }
        }
        if(e2 > x)
        {
            x += 1;
            err += x * 2 + 1;
        }
    } while(x < 0);
}

// Draw polyline
void ssd1306_Polyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size, SSD1306_COLOR color)
{
    uint16_t i;
    for(i = 1; i < par_size; i++)
    {
        ssd1306_Line(par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y, color);
    }
}

// Draw rectangle
void ssd1306_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color)
{
    ssd1306_Line(x1, y1, x2, y1, color);
    ssd1306_Line(x2, y1, x2, y2, color);
    ssd1306_Line(x2, y2, x1, y2, color);
    ssd1306_Line(x1, y2, x1, y1, color);
}

// Set contrast
void ssd1306_SetContrast(const uint8_t value)
{
    ssd1306_WriteCommand(0x81);
    ssd1306_WriteCommand(value);
}

// Set display on/off
void ssd1306_SetDisplayOn(const uint8_t on)
{
    SSD1306.DisplayOn = !!on;
    if(on)
    {
        ssd1306_WriteCommand(0xAF); // Display on
    }
    else
    {
        ssd1306_WriteCommand(0xAE); // Display off
    }
}

// Get display on/off state
uint8_t ssd1306_GetDisplayOn(void)
{
    return SSD1306.DisplayOn;
}
