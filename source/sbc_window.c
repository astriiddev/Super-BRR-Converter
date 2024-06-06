#ifdef _WIN32
#include <dwmapi.h>
#include <SDL2/SDL_syswm.h>
#endif

#include "sbc_utils.h"
#include "sbc_window.h"
#include "sbc_screen.h"

static struct Program_Info_s
{
	SDL_Window		*window;
	SDL_Renderer	*renderer;
	SDL_Texture 	*texture;

	bool quit;
} *sbcProgramInfo;

static Pixel_Buffer_t *sbcPixelBuffer;

bool initSbcWindow(void)
{
	bool success = true;

	SDL_DisplayMode DM;

	sbcProgramInfo = calloc(1, sizeof *sbcProgramInfo);

	sbcProgramInfo->window = SDL_CreateWindow("Super BRR Converter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
				        					SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
	if (sbcProgramInfo->window == NULL)
	{
		showErrorMsgBox("SDL Window Error!", "Window could not be created!", SDL_GetError());
		success = false;
	}
	else
	{
		initCursors();

#ifdef _WIN32
		SDL_SysWMinfo 	wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(sbcProgramInfo->window, &wmInfo);

		HWND hwnd = wmInfo.info.win.window;

		COLORREF TITLE_COLOR = 0xAE434F;

		HRESULT SET_CAPTION_COLOR =
		DwmSetWindowAttribute(
			hwnd, DWMWA_CAPTION_COLOR,
			&TITLE_COLOR, sizeof(TITLE_COLOR
		));

		if (FAILED(SET_CAPTION_COLOR))
		{
			showErrorMsgBox("Initialize Error", "Failed to initialize title bar!", NULL);
		}
#endif
	
		SDL_GetCurrentDisplayMode(0, &DM);

		sbcPixelBuffer = calloc(1, sizeof *sbcPixelBuffer);

		sbcPixelBuffer->width  = SCREEN_WIDTH;
		sbcPixelBuffer->height = SCREEN_HEIGHT;
		sbcPixelBuffer->update = false;

		if (DM.w > 3840) sbcPixelBuffer->scale = 3;
		else if (DM.w > 1920) sbcPixelBuffer->scale = 2;
		else if (DM.w > 1280) sbcPixelBuffer->scale = 1;
		else sbcPixelBuffer->scale = 0;

		sbcProgramInfo->renderer = SDL_CreateRenderer(sbcProgramInfo->window, -1, SDL_RENDERER_PRESENTVSYNC);
		if (sbcProgramInfo->renderer == NULL)
		{
			showErrorMsgBox("SDL Render Error!", "Renderer could not be created!", SDL_GetError());
			success = false;
		}
		else
		{
			int w, h;

			SDL_RenderSetLogicalSize(sbcProgramInfo->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
			SDL_SetRenderDrawBlendMode(sbcProgramInfo->renderer, SDL_BLENDMODE_NONE);

			SDL_SetWindowSize(sbcProgramInfo->window, SCREEN_WIDTH  << sbcPixelBuffer->scale, 
										 SCREEN_HEIGHT << sbcPixelBuffer->scale);

			SDL_SetWindowPosition(sbcProgramInfo->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

			SDL_GetWindowSize(sbcProgramInfo->window, &w, &h);
			printf("SBC window size: %dx%d\n", w, h);

			if(sbcPixelBuffer->scale > 0)
			{
				SDL_WarpMouseInWindow(sbcProgramInfo->window, SCREEN_WIDTH  << (sbcPixelBuffer->scale - 1), 
									  SCREEN_HEIGHT << (sbcPixelBuffer->scale - 1));
			}
			else 
			{
				SDL_WarpMouseInWindow(sbcProgramInfo->window, SCREEN_WIDTH  >> (sbcPixelBuffer->scale + 1), 
									  SCREEN_HEIGHT >> (sbcPixelBuffer->scale + 1));
			}

			sbcProgramInfo->texture = SDL_CreateTexture(sbcProgramInfo->renderer, SDL_PIXELFORMAT_ARGB8888, 
									SDL_TEXTUREACCESS_STREAMING, 
									SCREEN_WIDTH, SCREEN_HEIGHT);
			if (sbcProgramInfo->texture == NULL)
			{
				showErrorMsgBox("SDL Texture Error!", 
						"Texture could not be created!", SDL_GetError());
				success = false;
			}
			
			SDL_SetTextureBlendMode(sbcProgramInfo->texture, SDL_BLENDMODE_NONE);

			if (!allocateScreen(sbcPixelBuffer))
			{
				showErrorMsgBox("Video Buffer Error", "Not enough memory!", NULL);
				success = false;
			}

			setScreenBuffer(sbcPixelBuffer);
			clearScreenArea(0, SBCDGREY, sbcPixelBuffer->width * sbcPixelBuffer->height);

			SDL_SetRenderDrawColor(sbcProgramInfo->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		}
	}

	return success;
}

void cleanUpAndDestroyWindow(void)
{
	freeCursors();
	
	SBC_FREE(sbcPixelBuffer->buffer);
	SBC_FREE(sbcPixelBuffer);

	if(sbcProgramInfo->texture != NULL)
	{
		SDL_DestroyTexture(sbcProgramInfo->texture);
		sbcProgramInfo->texture = NULL;
	}

	if(sbcProgramInfo->renderer != NULL)
	{
		SDL_DestroyRenderer(sbcProgramInfo->renderer);
		sbcProgramInfo->renderer = NULL;
	}

	if(sbcProgramInfo->window != NULL)
	{
		SDL_DestroyWindow(sbcProgramInfo->window);
		sbcProgramInfo->window = NULL;
	}

	SBC_FREE(sbcProgramInfo);
}

SDL_Window   **getSbcWindow(void)     { return &sbcProgramInfo->window; }
SDL_Renderer **getSbcRenderer(void)   { return &sbcProgramInfo->renderer; }
SDL_Texture  **getSbcTexture(void)	  { return &sbcProgramInfo->texture; }

Pixel_Buffer_t *getSbcPixelBuffer(void) { return sbcPixelBuffer; }

bool screenShouldUpdate(void) { return sbcPixelBuffer->update; }
void screenHasUpdated(void) { sbcPixelBuffer->update = false; }
void repaintScreen(void) { sbcPixelBuffer->update = true; }

bool programShouldQuit(void) { return sbcProgramInfo->quit; }
void quitProgram(void) { sbcProgramInfo->quit = true; }
