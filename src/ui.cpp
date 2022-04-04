#include "gba.h"
#include "globals.h"
#include <stdlib.h>
#include <string.h>
#include <vgafont8.h>
#include "ui.h"



void uiClear() {
    memset(pix, 0, 240 * 160 * 2);
}

static inline void uiSetP(int x, int y, uint16_t color) {
    if (x < 0 || x >= LCD_W || y < 0 || y >= LCD_H) {
        return;
    }
    pix[x + y * LCD_W] = color;
}

static inline uint16_t uiGetP(int x, int y) {
    if (x < 0 || x >= LCD_W || y < 0 || y >= LCD_H) {
        return 0;
    }
    return pix[x + y * LCD_W];
}

void uiDrawBox(int x, int y, int w, int h, uint16_t color) {
    for (int _x = x; _x < x + w; _x++) {
        for (int _y = y; _y < y + h; _y++) {
            uiSetP(_x, _y, color);
        }
    }
}

void uiDrawBoxDim(int x, int y, int w, int h) {
    for (int _x = x; _x < x + w; _x++) {
        for (int _y = y; _y < y + h; _y++) {
            uint16_t p = uiGetP(_x, _y);
            // Set highest bit of each color to 0
            p &= 0x7BEF;
            uiSetP(_x, _y, p);
        }
    }
}

void uiDrawText(int x, int y, const char *text, uint16_t color) {
    while (*text) {
        uint8_t c = *text++;
        for (int i = 0; i < 8; i++) {
            uint8_t p = vgafont8[c * 8 + i];
            for (int j = 0; j < 8; j++) {
                if (p & (1 << (7 - j))) {
                    uiSetP(x + j, y + i, color);
                }
            }
        }
        x += 8;
    }
}

void uiShowMenu() {

}