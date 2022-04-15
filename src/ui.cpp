#include "ui.h"

#include <SDL.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vgafont8.h>

#include <string>
#include <vector>

#include "gba.h"
#include "globals.h"

#define COLOR_FG (0xffff)

void emuUpdateFB();
void uiClear() { memset(pix, 0, 240 * 160 * 2); }

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

extern int isQuitting;
extern int emuJoystickMap[12];
int uiWaitKey() {
  SDL_Event event;
  while (1) {
    SDL_WaitEvent(&event);
#if !defined(__SWITCH__) && !defined(__3DS__)
    if (event.type == SDL_KEYDOWN) {
      return event.key.keysym.sym;
    }
#endif
    if (event.type == SDL_QUIT) {
      isQuitting = 1;
      return SDLK_ESCAPE;
    }
    if (event.type == SDL_JOYBUTTONDOWN) {
      int button = event.jbutton.button;
      if (emuJoystickMap[0] == button) {
        return SDLK_RETURN;
      }
      if (emuJoystickMap[6] == button) {
        return SDLK_UP;
      }
      if (emuJoystickMap[7] == button) {
        return SDLK_DOWN;
      }
    }
#ifdef __3DS__
    if (event.type == SDL_JOYHATMOTION) {
      Uint8 hat = event.jhat.hat;
      Uint8 value = event.jhat.value;
      if (value == SDL_HAT_UP) {
        return SDLK_UP;
      } else if (value == SDL_HAT_DOWN) {
        return SDLK_DOWN;
      } else if (value == SDL_HAT_LEFT) {
        return SDLK_LEFT;
      } else if (value == SDL_HAT_RIGHT) {
        return SDLK_RIGHT;
      }
    }
#endif
  }
}

void uiShowText(const char *text) {
  uiClear();
  uiDrawText(0, 10, text, COLOR_FG);
  emuUpdateFB();
}

void uiMsgBox(const char *text) {
  uiShowText(text);
  uiWaitKey();
}

void uiDispError(const char *text) {
  uiClear();
  uiDrawText(0, 0, "Error:", COLOR_FG);
  uiDrawText(0, 12, text, COLOR_FG);
  emuUpdateFB();
  while (!isQuitting) {
    uiWaitKey();
  }
}

using namespace std;
vector<string> uiGbaFileList;
string uiGbaFilePath;
#ifdef __VITA__
#define BASE_DIR "ux0:gba"
#else
#define BASE_DIR "gba"
#endif

const char *uiChooseFileMenu() {
  uiGbaFileList.clear();
  DIR *dir = opendir(BASE_DIR);
  if (!dir) {
#ifdef _WIN32
    mkdir(BASE_DIR);
#else
    mkdir(BASE_DIR, 0777);
#endif

    dir = opendir(BASE_DIR);
  }
  if (!dir) {
    uiDispError("Could not open gba dir.");
    return NULL;
  }
  struct dirent *ent;
  while ((ent = readdir(dir))) {
    if (ent->d_name[0] == '.') {
      continue;
    }
    // Skip non-GBA files
    if (strlen(ent->d_name) < 4 ||
        strcmp(ent->d_name + strlen(ent->d_name) - 4, ".gba")) {
      continue;
    }
    uiGbaFileList.push_back(ent->d_name);
  }
  closedir(dir);
  if (uiGbaFileList.size() == 0) {
    uiDispError("No gba files found in gba dir.");
    return NULL;
  }
  int currentItem = 0;
  const int itemsPerPage = 13;
  while (1) {
    uiClear();
    int page = currentItem / itemsPerPage;
    int pageStart = page * itemsPerPage;
    int y = 0;
    for (int i = pageStart; i < pageStart + itemsPerPage; i++) {
      if (i >= (int)uiGbaFileList.size()) {
        break;
      }
      uiDrawText(24, y, uiGbaFileList[i].c_str(), COLOR_FG);
      if (i == currentItem) {
        uiDrawText(8, y, ">", COLOR_FG);
      }
      y += 12;
    }
    emuUpdateFB();
    int k = uiWaitKey();
    if (k == SDLK_ESCAPE) {
      return NULL;
    }
    if (k == SDLK_UP) {
      currentItem--;
      if (currentItem < 0) {
        currentItem = (int)uiGbaFileList.size() - 1;
      }
    }
    if (k == SDLK_DOWN) {
      currentItem++;
      if (currentItem >= (int)uiGbaFileList.size()) {
        currentItem = 0;
      }
    }
    if (k == SDLK_RETURN) {
      uiGbaFilePath = string(BASE_DIR) + "/" + uiGbaFileList[currentItem];
      return uiGbaFilePath.c_str();
    }
  }
}