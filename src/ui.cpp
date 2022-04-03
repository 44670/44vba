#include "gba.h"
#include "globals.h"
#include <stdlib.h>
#include <string.h>
#include <vgafont8.h>

#define COLOR_BLACK (0x0000)
#define COLOR_WHITE (0xFFFF)
#define LCD_W (240)
#define LCD_H (160)

void uiClear() {
    memset(pix, 0, 240 * 160 * 2);
}

static inline void uiSetP(int x, int y, uint16_t color) {
    if (x < 0 || x >= LCD_W || y < 0 || y >= LCD_H) {
        return;
    }
    pix[x + y * LCD_W] = color;
}

void uiShowMenu() {

}