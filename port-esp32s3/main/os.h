#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
void lcdInit();
void lcdSetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcdWriteFB(uint8_t *buf, int len);
void delayMS(int ms);
void osInit();
uint32_t osReadKey();

#define LCD_W (240)
#define LCD_H (240)

#define PIN_LCD_DC 10
#define PIN_LCD_CS 11
#define PIN_SPI0_MOSI 13
#define PIN_SPI0_MISO 14
#define PIN_SPI0_SCLK 12
#define PIN_SYS_RSTN 21

#define PIN_KEY_UP (0)
#define PIN_KEY_DOWN (39)
#define PIN_KEY_LEFT (38)
#define PIN_KEY_RIGHT (45)
#define PIN_KEY_A (18)
#define PIN_KEY_B (8)
#define PIN_KEY_SELECT (48)
#define PIN_KEY_START (46)

#ifdef __cplusplus
}
#endif