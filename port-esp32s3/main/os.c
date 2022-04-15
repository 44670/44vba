
#include "os.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

void delayMS(int ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }

#define USE_HORIZONTAL 0
spi_device_handle_t spiDev0;

static void lcdWrite(uint8_t *buf, int len, int isCmd) {
  if (len <= 0) {
    return;
  }
  gpio_set_level(PIN_LCD_CS, 0);
  gpio_set_level(PIN_LCD_DC, isCmd ? 0 : 1);
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8 * len;
  t.tx_buffer = buf;
  ESP_ERROR_CHECK(spi_device_transmit(spiDev0, &t));
  gpio_set_level(PIN_LCD_DC, 1);
  gpio_set_level(PIN_LCD_CS, 1);
}

static void lcdCmd8(uint8_t cmd) { lcdWrite(&cmd, 1, 1); }
static void lcdDat8(uint8_t dat) { lcdWrite(&dat, 1, 0); }
static void lcdDat16(uint16_t dat) {
  uint8_t buf[2] = {dat >> 8, dat & 0xff};
  lcdWrite(buf, 2, 0);
}

void lcdSetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  if (USE_HORIZONTAL == 0) {
    lcdCmd8(0x2a);  //列地址设置
    lcdDat16(x1);
    lcdDat16(x2);
    lcdCmd8(0x2b);  //行地址设置
    lcdDat16(y1);
    lcdDat16(y2);
    lcdCmd8(0x2c);  //储存器写
  } else if (USE_HORIZONTAL == 1) {
    lcdCmd8(0x2a);  //列地址设置
    lcdDat16(x1);
    lcdDat16(x2);
    lcdCmd8(0x2b);  //行地址设置
    lcdDat16(y1 + 80);
    lcdDat16(y2 + 80);
    lcdCmd8(0x2c);  //储存器写
  } else if (USE_HORIZONTAL == 2) {
    lcdCmd8(0x2a);  //列地址设置
    lcdDat16(x1);
    lcdDat16(x2);
    lcdCmd8(0x2b);  //行地址设置
    lcdDat16(y1);
    lcdDat16(y2);
    lcdCmd8(0x2c);  //储存器写
  } else {
    lcdCmd8(0x2a);  //列地址设置
    lcdDat16(x1 + 80);
    lcdDat16(x2 + 80);
    lcdCmd8(0x2b);  //行地址设置
    lcdDat16(y1);
    lcdDat16(y2);
    lcdCmd8(0x2c);  //储存器写
  }
}

void lcdWriteFB(uint8_t *buf, int len) {
  if (len <= 0) {
    return;
  }
  const int maxLen = 240 * 60*2;
  while(len > 0) {
    int writeLen = len > maxLen ? maxLen : len;
    lcdWrite((uint8_t *)buf, writeLen, 0);
    buf += writeLen;
    len -= writeLen;
  }
  
}

void lcdInit() {
  gpio_set_direction(PIN_SYS_RSTN, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LCD_CS, 1);
  gpio_set_level(PIN_SYS_RSTN, 0);
  delayMS(500);
  gpio_set_level(PIN_SYS_RSTN, 1);
  gpio_set_direction(PIN_LCD_CS, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_LCD_DC, 1);
  gpio_set_direction(PIN_LCD_DC, GPIO_MODE_OUTPUT);
  static const spi_bus_config_t buscfg = {
      .miso_io_num = PIN_SPI0_MISO,
      .mosi_io_num = PIN_SPI0_MOSI,
      .sclk_io_num = PIN_SPI0_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 120000,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
  ESP_LOGI("HAL", "SPI2 initialized");
  static const spi_device_interface_config_t devcfg = {.clock_speed_hz = 60000000,
                                          .mode = 0,
                                          .spics_io_num = -1,
                                          .queue_size = 7};
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spiDev0));

  ESP_LOGI("HAL", "Device Added");

  //************* Start Initial Sequence **********//
  lcdCmd8(0x11);  // Sleep out
  delayMS(120);   // Delay 120ms
  //************* Start Initial Sequence **********//
  lcdCmd8(0x36);
  if (USE_HORIZONTAL == 0)
    lcdDat8(0x00);
  else if (USE_HORIZONTAL == 1)
    lcdDat8(0xC0);
  else if (USE_HORIZONTAL == 2)
    lcdDat8(0x70);
  else
    lcdDat8(0xA0);

  lcdCmd8(0x3A);
  lcdDat8(0x05);

  lcdCmd8(0xB2);
  lcdDat8(0x0C);
  lcdDat8(0x0C);
  lcdDat8(0x00);
  lcdDat8(0x33);
  lcdDat8(0x33);

  lcdCmd8(0xB7);
  lcdDat8(0x35);

  lcdCmd8(0xBB);
  lcdDat8(0x19);

  lcdCmd8(0xC0);
  lcdDat8(0x2C);

  lcdCmd8(0xC2);
  lcdDat8(0x01);

  lcdCmd8(0xC3);
  lcdDat8(0x12);

  lcdCmd8(0xC4);
  lcdDat8(0x20);

  lcdCmd8(0xC6);
  lcdDat8(0x0F);

  lcdCmd8(0xD0);
  lcdDat8(0xA4);
  lcdDat8(0xA1);

  lcdCmd8(0xE0);
  lcdDat8(0xD0);
  lcdDat8(0x04);
  lcdDat8(0x0D);
  lcdDat8(0x11);
  lcdDat8(0x13);
  lcdDat8(0x2B);
  lcdDat8(0x3F);
  lcdDat8(0x54);
  lcdDat8(0x4C);
  lcdDat8(0x18);
  lcdDat8(0x0D);
  lcdDat8(0x0B);
  lcdDat8(0x1F);
  lcdDat8(0x23);

  lcdCmd8(0xE1);
  lcdDat8(0xD0);
  lcdDat8(0x04);
  lcdDat8(0x0C);
  lcdDat8(0x11);
  lcdDat8(0x13);
  lcdDat8(0x2C);
  lcdDat8(0x3F);
  lcdDat8(0x44);
  lcdDat8(0x51);
  lcdDat8(0x2F);
  lcdDat8(0x1F);
  lcdDat8(0x1F);
  lcdDat8(0x20);
  lcdDat8(0x23);
  lcdCmd8(0x21);

  lcdCmd8(0x29);
}



// "a", "b", "select", "start", "right", "left", "up", "down", "r", "l", "turbo", "menu"
int osKeyMap[12] = {PIN_KEY_A, PIN_KEY_B, PIN_KEY_SELECT, PIN_KEY_START, PIN_KEY_RIGHT, PIN_KEY_LEFT, PIN_KEY_UP, PIN_KEY_DOWN, -1, -1, -1, -1};
uint32_t osReadKey() {
  uint32_t ret = 0;
  for (int i = 0; i < 12; i++) {
    if (osKeyMap[i] != -1) {
      if (gpio_get_level(osKeyMap[i]) == 0) {
        ret |= 1 << i;
      }
    }
  }
  return ret;
}



void osInit() {
  lcdInit();
  for (int i = 0; i < 12; i++) {
    int pin = osKeyMap[i];
    if (pin != -1) {
      // Set the GPIO as a input, pullup
      gpio_set_direction(pin, GPIO_MODE_INPUT);
      gpio_pullup_en(pin);
      // disable pulldown
      gpio_pulldown_dis(pin);
    }
  }
}