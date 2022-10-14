// Using SDL2
#include <emscripten.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gba.h"
#include "globals.h"
#include "memory.h"
#include "sound.h"
#include "ui.h"

uint8_t emuGBuf[0x1000];

void load_image_preferences(void);

int showAudioDebug = 0;

int isQuitting = 0;
int frameDrawn = 0;
int audioSent = 0;
uint32_t frameCount = 0;
int emuFPS = 0;
char savFilePath[512];
char osdText[64];
int osdShowCount = 0;
uint32_t FB[240 * 160];

uint8_t lastSaveBuf[LIBRETRO_SAVE_BUF_LEN];

void systemMessage(const char *fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  printf("GBA: %s\n", buf);
}

void systemDrawScreen(void) {
  frameDrawn = 1;
  uint8_t *ptr = (uint8_t *)FB;
  uint16_t *srcPtr = (uint16_t *)pix;
  unsigned padding = 256 - 240;

  for (unsigned y = 0; y < 160; y++) {
    for (unsigned x = 0; x < 240; x++) {
      uint16_t c = *srcPtr;
      uint8_t r = ((c & 0xF800) >> 11) << 3;
      uint8_t g = ((c & 0x7E0) >> 5) << 2;
      uint8_t b = ((c & 0x1F)) << 3;
      ptr[0] = r;
      ptr[1] = g;
      ptr[2] = b;
      ptr[3] = 0xFF;
      ptr += 4;
      srcPtr++;
    }
    srcPtr += padding;
  }
}

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length) {
  EM_ASM_({ writeAudio($0, $1); }, finalWave, length / 2);
}

extern "C" {

void emuRunAudio() {
  audioSent = 0;
  while (!audioSent) {
    CPULoop();
  }
}

void emuRunFrame(int key) {
  joy = key;
  UpdateJoypad();
  frameDrawn = 0;
  while (!frameDrawn) {
    CPULoop();
  }
}

int emuUpdateSavChangeFlag() {
  int changed =
      (memcmp(lastSaveBuf, libretro_save_buf, LIBRETRO_SAVE_BUF_LEN) != 0);
  if (changed) {
    memcpy(lastSaveBuf, libretro_save_buf, LIBRETRO_SAVE_BUF_LEN);
  }
  return changed;
}
void emuSetSampleRate(int sampleRate) { soundSetSampleRate(sampleRate); }

int emuLoadROM(int romSize) {
  cpuSaveType = 0;
  flashSize = 0x10000;
  enableRtc = false;
  mirroringEnable = false;
  CPUSetupBuffers();
  CPUInit(NULL, false);
  // gba_init();

  load_image_preferences();
  CPUReset();
  soundSetSampleRate(47782);
  soundReset();
  rtcEnable(true);
  flashSetSize(flashSize);
  memcpy(lastSaveBuf, libretro_save_buf, LIBRETRO_SAVE_BUF_LEN);
#ifdef USE_FRAME_SKIP
#warning "Frame skip enabled"
  SetFrameskip(0x1);
#endif
  return 0;
}

void emuResetCpu() { CPUReset(); }

void *emuGetSymbol(int id) {
  if (id == 1) {
    return rom;
  }
  if (id == 2) {
    return libretro_save_buf;
  }
  if (id == 3) {
    return FB;
  }
  if (id == 4) {
    return emuGBuf;
  }
  return 0;
}

int emuAddCheat(const char *codeLine) {
  int len = strlen(codeLine);
  if (len == 13) {
    cheatsAddCBACode(codeLine, codeLine);
  } else if (len == 16) {
    cheatsAddGSACode(codeLine, codeLine, true);
  } else {
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  memset(rom, 0, 32 * 1024 * 1024);
  EM_ASM(wasmReady(););
  return 0;
}
}