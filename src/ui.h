#pragma once

#include <stdint.h>
void uiDrawBoxDim(int x, int y, int w, int h);
void uiDrawText(int x, int y, const char *text, uint16_t color);
void uiDrawBox(int x, int y, int w, int h, uint16_t color);
void uiShowText(const char *text);
void uiDispError(const char *text);
#define COLOR_BLACK (0x0000)
#define COLOR_WHITE (0xFFFF)
#define LCD_W (256)
#define LCD_H (160)

const char *uiChooseFileMenu();