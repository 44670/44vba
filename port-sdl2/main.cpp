// Using SDL2

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "gba.h"
#include "globals.h"
#include "memory.h"
#include "sound.h"
#include "ui.h"
#ifdef __SWITCH__
#include <switch.h>
#endif
#ifdef __VITA__
#include <psp2/kernel/processmgr.h>
#endif
#ifdef __3DS__
#include <3ds.h>
#endif

int showAudioDebug = 0;

int isQuitting = 0;
int autoSaveEnabled = 0;
int frameDrawn = 0;
int audioSent = 0;
uint32_t frameCount = 0;
int emuFPS = 0;
char savFilePath[512];
char osdText[64];
int osdShowCount = 0;

#ifndef SDL1
SDL_Texture *texture;
SDL_Renderer *renderer;
SDL_AudioDeviceID audioDevice;
#else
SDL_Surface *screen;
#endif
#define AUDIO_LOCK SDL_LockAudio()
#define AUDIO_UNLOCK SDL_UnlockAudio()
int isAudioEnabled = 0;

uint32_t lastTime = SDL_GetTicks();
uint8_t lastSaveBuf[LIBRETRO_SAVE_BUF_LEN];
int prevSaveChanged = 0;

#define AUDIO_FIFO_CAP (8192)
volatile int16_t audioFifo[AUDIO_FIFO_CAP];
volatile int audioFifoHead = 0;
volatile int audioFifoLen = 0;
volatile int turboMode = 0;
// "a", "b", "select", "start", "right", "left", "up", "down", "r", "l",
// "turbo", "menu"
volatile int emuKeyState[12][2];
int emuKeyboardMap[12] = {SDLK_x,     SDLK_z,    SDLK_SPACE, SDLK_RETURN,
                          SDLK_RIGHT, SDLK_LEFT, SDLK_UP,    SDLK_DOWN,
                          SDLK_s,     SDLK_a,    SDLK_TAB,   SDLK_ESCAPE};
#if defined(__VITA__)
/*
static const unsigned int button_map[] = {
    SCE_CTRL_TRIANGLE, SCE_CTRL_CIRCLE, SCE_CTRL_CROSS, SCE_CTRL_SQUARE,
    SCE_CTRL_LTRIGGER, SCE_CTRL_RTRIGGER,
    SCE_CTRL_DOWN, SCE_CTRL_LEFT, SCE_CTRL_UP, SCE_CTRL_RIGHT,
    SCE_CTRL_SELECT, SCE_CTRL_START};
*/
int emuJoystickMap[12] = {1, 2, 10, 11, 9, 7, 8, 6, 5, 4, 0, 3};
int emuJoystickDeadzone = 10000;
#elif defined(__3DS__)
int emuJoystickMap[12] = {1, 2, 7, 8, -1, -1, -1, -1, 6, 5, 9, 8};
int emuJoystickDeadzone = 10000;
#else
int emuJoystickMap[12] = {0, 1, 11, 10, 14, 12, 13, 15, 7, 6, 9, 5};
int emuJoystickDeadzone = 10000;
#endif
void emuRunAudio() {
  audioSent = 0;
  while (!audioSent) {
    CPULoop();
  }
}

void emuRunFrame() {
  frameDrawn = 0;
  while (!frameDrawn) {
    CPULoop();
  }
}

void emuShowOsd(int cnt, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(osdText, sizeof(osdText), fmt, args);
  va_end(args);
  osdShowCount = cnt;
}

int emuUpdateSaveFile() {
  if (strlen(savFilePath) == 0) {
    return 0;
  }
  FILE *savFile = fopen(savFilePath, "wb");
  if (!savFile) {
    printf("Failed to open save file: %s\n", savFilePath);
    emuShowOsd(10, "Save failed!");
    return -1;
  }
  fwrite(libretro_save_buf, 1, LIBRETRO_SAVE_BUF_LEN, savFile);
  fclose(savFile);
  printf("Saved save file: %s\n", savFilePath);
  emuShowOsd(3, "Auto saved.");
  osdShowCount = 3;
  return 0;
}

void emuCheckSave() {
  if (!autoSaveEnabled) {
    return;
  }
  int changed = memcmp(lastSaveBuf, libretro_save_buf, LIBRETRO_SAVE_BUF_LEN);
  if (changed) {
    memcpy(lastSaveBuf, libretro_save_buf, LIBRETRO_SAVE_BUF_LEN);
  } else {
    if (prevSaveChanged) {
      // Changed in previous check, and not changed now
      emuUpdateSaveFile();
    }
  }
  prevSaveChanged = changed;
}

void emuUpdateFB() {
#ifndef SDL1
  SDL_UpdateTexture(texture, NULL, pix, 256 * 2);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
#else
  memcpy(screen->pixels, pix, 256 * 160 * 2);
  SDL_UpdateRect(screen, 0, 0, 0, 0);
#endif
}

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
  frameCount++;
  if (frameCount % 120 == 0) {
    uint32_t currentTime = SDL_GetTicks();
    uint32_t delta = currentTime - lastTime;
    if (delta <= 0) {
      delta = 1;
    }
    emuFPS = 120 * 1000 / delta;
    printf("FPS: %d\n", emuFPS);
    lastTime = currentTime;
  }
  if (frameCount % 60 == 0) {
    if (!turboMode) {
      emuCheckSave();
      if (osdShowCount > 0) {
        osdShowCount--;
      }
    }
  }
  if (turboMode) {
#ifdef __SWITCH__
    const int turboModeSkipFactor = 6;
#else
    const int turboModeSkipFactor = 60;
#endif
    if (frameCount % turboModeSkipFactor != 0) {
      return;
    }
  }
  char buf[64];
  if (osdShowCount > 0) {
    // Draw osd
    uiDrawBoxDim(0, 0, 240, 10);
    uiDrawText(0, 0, osdText, COLOR_WHITE);
  } else if (showAudioDebug) {
    uiDrawBoxDim(0, 0, 240, 10);
    snprintf(buf, sizeof(buf), "FPS: %d, fifo: %d", emuFPS, audioFifoLen);
    uiDrawText(0, 0, buf, COLOR_WHITE);
  } else {
    if (turboMode) {
      // Show FPS
      uiDrawBoxDim(0, 0, 240, 10);
      snprintf(buf, sizeof(buf), "FPS: %d", emuFPS);
      uiDrawText(0, 0, buf, COLOR_WHITE);
    }
  }
  emuUpdateFB();
}

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length) {
  audioSent = 1;
  if (turboMode) {
    // Do not send audio in turbo mode
    return;
  }
  AUDIO_LOCK;

  int wpos = (audioFifoHead + audioFifoLen) % AUDIO_FIFO_CAP;
  if (audioFifoLen + length >= AUDIO_FIFO_CAP) {
    // printf("audio fifo overflow: %d\n", audioFifoLen);
    goto bed;
  }
  length = (length / 2) * 2;
  for (int i = 0; i < length; i++) {
    if (audioFifoLen >= AUDIO_FIFO_CAP) {
      break;
    }
    audioFifo[wpos] = finalWave[i];
    wpos = (wpos + 1) % AUDIO_FIFO_CAP;
    audioFifoLen++;
  }
bed:
  AUDIO_UNLOCK;
}

void audioCallback(void *userdata, Uint8 *stream, int len) {
  memset(stream, 0, len);
  if (turboMode) {
    return;
  }
  uint16_t *wptr = (uint16_t *)stream;
  int samples = len / 2;
  if (audioFifoLen < samples) {
    // printf("audio underrun: %d < %d\n", audioFifoLen, samples);
  } else {
    for (int i = 0; i < samples; i++) {
      int16_t sample = audioFifo[audioFifoHead];
      audioFifoHead = (audioFifoHead + 1) % AUDIO_FIFO_CAP;
      audioFifoLen--;
      *wptr = sample;
      wptr++;
    }
  }
}

int emuLoadROM(const char *path) {
  memset(rom, 0, 32 * 1024 * 1024);
  memset(libretro_save_buf, 0, LIBRETRO_SAVE_BUF_LEN);
  FILE *f = fopen(path, "rb");
  if (!f) {
    uiDispError("Open ROM Failed.");
    return -1;
  }
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (size > 32 * 1024 * 1024) {
    uiDispError("ROM too large.");
    return -1;
  }
  int bytesRead = fread(rom, 1, size, f);
  fclose(f);

  cpuSaveType = 0;
  flashSize = 0x10000;
  enableRtc = false;
  mirroringEnable = false;
  CPUSetupBuffers();
  CPUInit(NULL, false);
  // gba_init();
  void load_image_preferences(void);
  load_image_preferences();
  CPUReset();
  soundSetSampleRate(47782);
  soundReset();
  rtcEnable(true);
  flashSetSize(flashSize);
  // Load Save File
  snprintf(savFilePath, sizeof(savFilePath), "%s.4gs", path);
  FILE *savFile = fopen(savFilePath, "rb");
  if (savFile) {
    printf("Loading save file: %s\n", savFilePath);
    fread(libretro_save_buf, 1, LIBRETRO_SAVE_BUF_LEN, savFile);
    fclose(savFile);
  }
  memcpy(lastSaveBuf, libretro_save_buf, LIBRETRO_SAVE_BUF_LEN);
  prevSaveChanged = 0;
#ifdef USE_FRAME_SKIP
#warning "Frame skip enabled"
  SetFrameskip(0x1);
#endif
  return 0;
}

void emuHandleKey(int key, int down) {
  for (int i = 0; i < 12; i++) {
    if (emuKeyboardMap[i] == key) {
      emuKeyState[i][0] = down;
      return;
    }
  }
}

int main(int argc, char *argv[]) {
#ifdef __3DS__
  osSetSpeedupEnable(true);
#endif
  int windowWidth = 240 * 4;
  int windowHeight = 160 * 4;
#ifndef SDL1
#ifdef _WIN32
  printf("We are on windows! Using opengl...\n");
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#endif
  // Init SDL2
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
#ifdef __SWITCH__
  windowWidth = 1280;
  windowHeight = 720;
  if (appletGetOperationMode() == AppletOperationMode_Console) {
    windowWidth = 1920;
    windowHeight = 1080;
  }
#endif
#ifdef __VITA__
  windowWidth = 960;
  windowHeight = 544;
#warning "Vita: Using 960x544 window"
#endif
#ifndef SDL1
  // Init video to RGB565
  SDL_Window *window = SDL_CreateWindow(
      "GBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth,
      windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                              SDL_TEXTUREACCESS_STREAMING, 240, 160);
#else
  screen = SDL_SetVideoMode(256, 160, 16, SDL_SWSURFACE);
  SDL_ShowCursor(SDL_DISABLE);
#endif
#ifdef __3DS__
  consoleInit(GFX_BOTTOM, NULL);
#endif
  SDL_Joystick *joystick = SDL_JoystickOpen(0);
  if (!joystick) {
    printf("Failed to open joystick\n");
  } else {
    printf("Opened joystick\n");
  }
  SDL_JoystickEventState(SDL_ENABLE);
  const char *romPath = uiChooseFileMenu();
  if (romPath == NULL) {
    printf("No ROM selected\n");
    return 0;
  }
  uiShowText("Loading...");
  if (emuLoadROM(romPath) != 0) {
    return 0;
  }
  uiShowText("Loaded!");
  // Init audio
  SDL_AudioSpec desiredSpec = {
      .freq = 48000,
      .format = AUDIO_S16SYS,
      .channels = 2,
      .samples = 1024,
      .callback = audioCallback,
  };
  int ret = SDL_OpenAudio(&desiredSpec, NULL);
  if (ret != 0) {
    printf("Could not open audio device\n");
  } else {
    printf("Audio device opened\n");
    isAudioEnabled = 1;
  }

  // Play audio
  SDL_PauseAudio(0);
#ifdef __SWITCH__
  appletLockExit();
#endif

  while (1) {
    if (isQuitting) {
      goto bed;
    }
    frameDrawn = 0;
    emuRunFrame();
    // Handle event when frame is drawn
    if (frameDrawn) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
          isQuitting = 1;
          goto bed;
        }
#ifdef USE_KEYBOARD
        if (event.type == SDL_KEYDOWN) {
          int keyCode = event.key.keysym.sym;
          emuHandleKey(keyCode, 1);
        }
        if (event.type == SDL_KEYUP) {
          int keyCode = event.key.keysym.sym;
          emuHandleKey(keyCode, 0);
        }
#endif
      }
      // Poll joysticks
      if (joystick) {
        // SDL_JoystickUpdate(); // Already called in SDL_PollEvent
        for (int i = 0; i < 12; i++) {
          emuKeyState[i][1] = 0;
          if (emuJoystickMap[i] == -1) {
            continue;
          }
          int value = SDL_JoystickGetButton(joystick, emuJoystickMap[i]);
          emuKeyState[i][1] = value;
        }
        int xaxis = SDL_JoystickGetAxis(joystick, 0);
        int yaxis = SDL_JoystickGetAxis(joystick, 1);
        emuKeyState[4][1] |= xaxis > emuJoystickDeadzone;
        emuKeyState[5][1] |= xaxis < -emuJoystickDeadzone;
        emuKeyState[6][1] |= yaxis < -emuJoystickDeadzone;
        emuKeyState[7][1] |= yaxis > emuJoystickDeadzone;
#ifdef __3DS__
        Uint8 hat = SDL_JoystickGetHat(joystick, 0);
        // printf("hat: %d\n", hat);
        emuKeyState[4][1] |= hat & SDL_HAT_RIGHT;
        emuKeyState[5][1] |= hat & SDL_HAT_LEFT;
        emuKeyState[6][1] |= hat & SDL_HAT_UP;
        emuKeyState[7][1] |= hat & SDL_HAT_DOWN;
#endif
      }
      joy = 0;
      for (int i = 0; i < 10; i++) {
        if (emuKeyState[i][0] || emuKeyState[i][1]) {
          joy |= (1 << i);
        }
      }
      turboMode = emuKeyState[10][0] || emuKeyState[10][1];
      UpdateJoypad();
    }
  }
bed:
  emuUpdateSaveFile();
#ifdef __SWITCH__
  appletUnlockExit();
#endif
#ifndef __3DS__
  // Do not call SDL_Quit on 3ds
  SDL_Quit();
#endif
#ifdef __VITA__
  sceKernelExitProcess(0);
#endif
  return 0;
}
