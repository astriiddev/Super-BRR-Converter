#ifndef __SBC_WINDOW_H
#define __SBC_WINDOW_H

#include <SDL2/SDL.h>

#include "sbc_defs.h"

bool initSbcWindow(void);

SDL_Window **getSbcWindow(void);
SDL_Renderer **getSbcRenderer(void);
SDL_Texture  **getSbcTexture(void);

Pixel_Buffer_t *getSbcPixelBuffer(void);

bool screenShouldUpdate(void);
void screenHasUpdated(void);
void repaintScreen(void);

bool programShouldQuit(void);
void quitProgram(void);

void cleanUpAndDestroyWindow(void);

#endif /* __SBC_WINDOW_H */
