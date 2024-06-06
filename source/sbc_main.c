#ifdef __WIN_DEBUG__
#include <crtdbg.h>
#endif

#include "sbc_utils.h"
#include "sbc_conf.h"
#include "sbc_logo.h"

#include "sbc_window.h"
#include "sbc_screen.h"
#include "sbc_gui.h"
#include "sbc_optmenu.h"
#include "sbc_textbox.h" 

#include "sbc_events.h"
#include "sbc_audio.h"
#include "sbc_samp_edit.h"
#include "sbc_waveform.h"

#include "sbc_buttons.h"
#include "sbc_sliders.h"

#include "sbc_fileload.h"
#include "sbc_filedialog.h"

static SDL_Texture* logo = NULL;

static bool init(void);
static bool loadSampleArg(char *file_arg);

int main(int argc, char* argv[])
{
#ifdef __WIN_DEBUG__
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR | _CRT_WARN, _CRTDBG_MODE_FILE  );
	_CrtSetReportFile( _CRT_ERROR | _CRT_WARN , _CRTDBG_FILE_STDERR );
#endif

	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

	if (!init())
		showErrorMsgBox("Init Error", "Failed to initialize SBC700!", NULL);
	else
	{
		bool sample_loaded = false;

		SDL_Event e;
		SDL_Rect logo_rect = { 5, SAMPLE_HEIGHT + 22, 130, 64 };

		initFileDialog(&argc, &argv);

		initSampleBuffers();

		if(!initAudio()) showErrorMsgBox("SDL Audio Error!", "Could not initialize audio!", SDL_GetError());
		
		loadConfig();

		initButtons();
		initTextboxes();

		if(*isSampEditLoopEnabled()) click_button(getLoopButton(), true);
		else click_button(getLoopButton(), false);

		SDL_StartTextInput();

		initOptMenu();

		if(argc > 1) sample_loaded = loadSampleArg(argv[1]);

		if(!sample_loaded) 
		{
			clearSampleEdit();
			resetSliders();
			drawNewWave();
		}

		while (!programShouldQuit())
		{
			handleFileDialogEvents();

			while (SDL_PollEvent(&e) > 0) handleEvents(&e);

			SDL_RenderClear(*getSbcRenderer());

			if(paintSBC()) repaintScreen();

			if (screenShouldUpdate())
			{
				SDL_UpdateTexture(*getSbcTexture(), NULL, *getScreenBuffer(), 
						          SCREEN_WIDTH * sizeof * *getScreenBuffer()); 
				
                screenHasUpdated();
			}
			
		    SDL_RenderCopy(*getSbcRenderer(), *getSbcTexture(), NULL, NULL);
			SDL_RenderCopy(*getSbcRenderer(), logo, NULL, &logo_rect);

			if(!optionsIsShowing()) paintSliders();

			SDL_RenderPresent(*getSbcRenderer());

			if (*audioQueued()) 
			{
				playAudio();
				repaintGUI();
			}
			else audioPaused();
		}

		printf("Freeing buffers...\n");
		freeDrawingSampleBuffer();
	}

	saveConfig();
	freeAudio();

	freeOptMenu();
	freeTextboxes();
	freeButtons();

	cleanUpAndFreeSampleEdit();

	SDL_DestroyTexture(logo);
    logo = NULL;
        
    cleanUpAndDestroyWindow();

	printf("Closing SDL2...\n");
	SDL_Quit();

#ifdef __WIN_DEBUG__
	_CrtDumpMemoryLeaks();
#endif

	printf("Closing out of SBC700...\n");
	printf("\033[1;34mThank \033[1;35myou \033[1;37mfor \033[1;35musing \033[1;34mSBC700!:)\033[0;37m\n\n");

	return 0;
}

static bool init(void)
{
	bool success = true;

	printf("\n\033[38;5;55mSBC700 v0.3 by _astriid_\033[0;37m\n\n");
	printf("Initializing SDL2...\n");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
	{
		showErrorMsgBox("SDL Init Error!", "SDL could not initialize!", SDL_GetError());
		success = false;
	}
	else
	{
		SDL_version sdl_ver;

		SDL_VERSION(&sdl_ver);
		printf("SDL compiled version: %u.%u.%u\n", sdl_ver.major, sdl_ver.minor, sdl_ver.patch);

		SDL_GetVersion(&sdl_ver);
		printf("SDL linked version: %u.%u.%u\n", sdl_ver.major, sdl_ver.minor, sdl_ver.patch);

		SDL_SetHint("SDL_RENDER_SCALE_QUALITY", "nearest");
		SDL_SetHint("SDL_WINDOWS_DPI_SCALING", "1");

		if (!initSbcWindow())
        {
            success = false;
            showErrorMsgBox("Window init error!", "Could not initialize window!", NULL);
        }
        else
        {
            SDL_RWops *rw = NULL;
            SDL_Surface* loadedSurface = NULL;

            if((rw = SDL_RWFromConstMem(logo_array, LOGO_LENGTH)) == NULL)
            {
                showErrorMsgBox("SDL Memory Read Error!", 
                        		"Logo could not be read from memory", SDL_GetError());
                success = false;
            }

            loadedSurface = SDL_LoadBMP_RW(rw, 0);

            SDL_FreeRW(rw);
            rw = NULL;

            if (loadedSurface == NULL)
            {
                showErrorMsgBox("SDL Surface Error", 
                        		"Logo could not be copied to surface!", SDL_GetError());
                success = false;
            }
            else
            {
                logo = SDL_CreateTextureFromSurface(*getSbcRenderer(), loadedSurface);
                if (logo == NULL)
                {
                    showErrorMsgBox("SDL Texture Error", 
                            		"Logo could not be copied to texture!", SDL_GetError());
                    success = false;
                }

                SDL_FreeSurface(loadedSurface);
            }
        }
    }

	return success;
}

static bool loadSampleArg(char *file_arg)
{
	char *sample_arg = NULL, *tilde_dir;
	const size_t file_len = strlen(file_arg);

	bool success = false;

	if((tilde_dir = strchr(file_arg, '~')) != NULL)
	{
		if(file_len <= 2) return success;

		if(*tilde_dir != *file_arg)
		{
			sample_arg = _strndup(file_arg, (int) file_len);
		}
		else 
		{	
            size_t path_len;
#if defined (_WIN32)
			char* home_dir = getenv("USERPROFILE");
            if(home_dir == NULL) return false;
            
			path_len = strlen(home_dir) + (file_len - 2);

			sample_arg = calloc(path_len, sizeof *sample_arg);
			snprintf(sample_arg, path_len + 2, "%s\\%s", home_dir, file_arg + 2);
#else
			char* home_dir = getenv("HOME");
            if(home_dir == NULL) return false;
            
			path_len = strlen(home_dir) + (file_len - 2);

			sample_arg = calloc(path_len, sizeof *sample_arg);
			snprintf(sample_arg, path_len + 2, "%s/%s", home_dir, file_arg + 2);
#endif
		}
	}
	else
	{
		sample_arg = _strndup(file_arg, file_len);
	}

	success = loadSample(sample_arg);

	SBC_FREE(sample_arg);

	return success;
}
