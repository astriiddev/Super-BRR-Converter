#include "sbc_utils.h"

#include "sbc_optmenu.h"
#include "sbc_window.h"
#include "sbc_gui.h"

#include "sbc_samp_edit.h"
#include "sbc_waveform.h"

#include "sbc_buttons.h"
#include "sbc_sliders.h"

#include "sbc_fileload.h"
#include "sbc_filesave.h"
#include "sbc_filedialog.h"

enum Cursor_Type
{
	DFLT_CURSOR = 0,
	HAND_CURSOR = 1,
	TEXT_CURSOR = 2,
	DRAG_CURSOR = 3
};

static SDL_Cursor *sbc_cursor[4];
static char *work_dir = NULL;

static bool export_16bit = true; 

static uint64_t repaint_timer = 0;
static int64_t samp2wave_scale = 0, wave2samp_scale = 0;

bool exporting16bit(void) { return export_16bit; }
void set16bitExport(const bool bit16) { export_16bit = bit16; }

int *getWindowScale(void) { return &getSbcPixelBuffer()->scale; }

int fixedMap(const int64_t x, const int64_t in_min, const int64_t in_max, const int64_t out_min, const int64_t out_max)
{
	return  (in_max - in_min) == 0 ?  0 :
			(int)((((x - in_min) * ((out_max - out_min) << 32) / (in_max - in_min) + ROUND32) >> 32) + out_min);
}

void setSampScale(const int64_t samp_len)	{ samp2wave_scale = (samp_len << 32) / SCREEN_WIDTH; }
void setWaveScale(const int64_t line_len)	{ wave2samp_scale = ((int64_t)SCREEN_WIDTH << 32) / line_len; }

int scr2samp(const int64_t x) { return (((x * samp2wave_scale) + ROUND32) >> 32) + *getWaveStart(); }
int samp2scr(const int64_t x) { return (((x - *getWaveStart()) * wave2samp_scale) + ROUND32) >> 32; }

int hitbox(const Rect_t *r, const int x, const int y)
{
    if(r == NULL) return 0;
    
	if(x < r->x) return 0;
	if(x > r->x + r->w) return 0;
	if(y < r->y) return 0;
	if(y > r->y + r->h) return 0;

	return 1;
}

int addColors(const int c1, const int c2)
{
	const int A = 0xFF000000,
			  R = 0x00FF0000,
			  G = 0x0000FF00,			  
			  B = 0x000000FF;
	
	int r = c1 & R,
		g = c1 & G,
		b = c1 & B;

	r = r > (R - (R & c2)) ? R : r + (R & c2);
	g = g > (G - (G & c2)) ? G : g + (G & c2);
	b = b > (B - (B & c2)) ? B : b + (B & c2);

	return (A | r | g | b);
}

int subColors(const int c1, const int c2)
{
	const int A = 0xFF000000,
			  R = 0x00FF0000,
			  G = 0x0000FF00,			  
			  B = 0x000000FF;
	
	int r = c1 & R,
		g = c1 & G,
		b = c1 & B;

	r = r < (R & c2) ? 0 : r - (R & c2);
	g = g < (G & c2) ? 0 : g - (G & c2);
	b = b < (B & c2) ? 0 : b - (B & c2);

	return (A | r | g | b);
}

bool loadSample(const char* file_path)
{
	char *file_name = NULL;

	if((file_path) == NULL) return false;

	SBC_LOG(LOADING FILE, %s, file_path);

	if(!loadFile(file_path)) return false;

	file_name = (char *) getFileNameWithoutExt(file_path);
	handleSampleNameText(file_name);
	handleSampleRateText(*getSampleEditSampleRate());

	if(*isSampEditLoopEnabled()) click_button(getLoopButton(), true);
	else click_button(getLoopButton(), false);

	drawNewWave();	
	resetZoom();
	resetSliders();

	if(!optionsIsShowing()) repaintWaveform();
	
	repaintGUI();

	// hacky way to automatically regain keyboard focus, doesn't work on mac
#ifndef __APPLE__
	SDL_MaximizeWindow(*getSbcWindow());
	SDL_SetWindowInputFocus(*getSbcWindow());
#endif

	SBC_FREE(file_name);

	return true;
}

void openFileDialog(void)
{
	const char  *dir_path  = getLastDir(),
				*file_path = openDialog(dir_path);

	if(file_path == NULL) return;
	
	loadSample(file_path);

	SBC_FREE(file_path);
}

void saveFileDialog(void)
{
	const char *dir_path = getLastDir(), *file_path = NULL;
	
	if (*getSampleEditBuffer() == NULL || *getSampleEditLength() <= 1) return;

	file_path =  saveDialog(dir_path, getSampEditName());
	
	if(file_path == NULL) return;

	SBC_LOG(SAVING FILE, %s, file_path);
	
	saveFile(file_path);

	SBC_FREE(file_path);
}

void setWorkingDir(char *dir)
{
	if(isStringEmpty(dir)) return;

	SBC_FREE(work_dir);
	work_dir = _strndup(dir, strlen(dir));

	SBC_LOG(WORKING DIR, %s, work_dir);
}

char *getWorkingDir(void)
{
	char* home_dir = isStringEmpty(work_dir) ? getHomeDir() : NULL;

	if(home_dir != NULL)
	{
		setWorkingDir(home_dir);
		SBC_FREE(home_dir);
	}
	
	return work_dir;
}

int getRelativeBrrSampBlock(const int samp, const int ref_pos)
{
    int new_samp = samp;

	if(!wasButtonClicked(getBrrButton())) return samp;

    if(samp < ref_pos) new_samp -= (samp - ref_pos) % 16;
    else new_samp += (ref_pos - samp) % 16;

    return new_samp + 1 < *getSampleEditLength() ? new_samp : *getSampleEditLength();
}

bool isStringEmpty(const char *s)
{
	if(s == NULL) return true;
	if(s[0] == '\0') return true;
    return false;
}

void initCursors(void)
{
	work_dir = _strndup("", 1);

	sbc_cursor[HAND_CURSOR] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	sbc_cursor[DRAG_CURSOR] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	sbc_cursor[TEXT_CURSOR] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	sbc_cursor[DFLT_CURSOR] = SDL_GetDefaultCursor();

	SDL_SetCursor(sbc_cursor[DFLT_CURSOR]);
}

void freeCursors(void)
{
	for(int i = 0; i < 3; i++)
		SDL_FreeCursor(sbc_cursor[i]);
	
	setLastDir(NULL);
	SBC_FREE(work_dir);
}

static bool set_mouse_cursor(const enum Cursor_Type cursor)
{
	if(SDL_GetCursor() == sbc_cursor[cursor]) return false;
	if(sbc_cursor[cursor] == NULL) return false;

	SDL_SetCursor(sbc_cursor[cursor]);
	return true;
}

void setHandCursor(void) { if(set_mouse_cursor(HAND_CURSOR)) SBC_LOG(MOUSE CURSOR, %s, "HAND CURSOR"); }
void setDragCursor(void) { if(set_mouse_cursor(DRAG_CURSOR)) SBC_LOG(MOUSE CURSOR, %s, "DRAG CURSOR"); }
void setTextCursor(void) { if(set_mouse_cursor(TEXT_CURSOR)) SBC_LOG(MOUSE CURSOR, %s, "TEXT CURSOR"); }
void setStandardCursor(void) { if(set_mouse_cursor(DFLT_CURSOR)) SBC_LOG(MOUSE CURSOR, %s, "STANDARD CURSOR"); }

bool getRepaintTimer(void)
{
	if(SDL_GetTicks64() - repaint_timer < 20) return false;
	repaint_timer = SDL_GetTicks64();
	return true;
}

void showErrorMsgBox(const char *title, const char *msg, const char *error)
{
	char errormsg[256];
	soundErrorBell();

	memset(errormsg, '\0', 256);
	if (error != NULL) snprintf(errormsg, 255, "%s Error: %s", msg, error);
	else snprintf(errormsg, 255, "%s", msg);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title, errormsg, *getSbcWindow());

	SBC_ERR(title, errormsg);
}

char *_strndup(char *str, size_t chars)
{
#if defined (_WIN32)		// MinGW doesn't have strndup but likes to think it does

    char *buffer;
    size_t n;

    buffer = (char *) malloc(chars +1);
    if (buffer)
    {
        for (n = 0; ((n < chars) && (str[n] != 0)) ; n++) buffer[n] = str[n];
        buffer[n] = 0;
    }

    return buffer;

#else
	return strndup(str, chars);
#endif
}

/*
* 	Cross-platform strcasestr with respect to early SDL2 versions. 
*/
char *_strcasestr(const char *haystack, const char *needle)
{

#ifdef HAVE_STRCASESTR
        return strcasestr(haystack, needle);
#elif(SDL_VERSION_ATLEAST(2,26,0))
        return SDL_strcasestr(haystack, needle);
#else
        size_t length = SDL_strlen(needle);
        while (*haystack) {
                if (SDL_strncasecmp(haystack, needle, length) == 0) {
                return (char *) haystack;
                }
                ++haystack;
        }
        return NULL;
#endif

}

void print_args(const char *format, const int size, ...)
{
	va_list args; 
	va_start(args, size); 
	vprintf(format, args); 
	va_end(args);
	printf("\n");
}
