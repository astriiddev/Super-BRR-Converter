#ifndef __SBC_UTILS_H
#define __SBC_UTILS_H

#include <string.h>
#include <errno.h>

#include "sbc_defs.h"

int *getWindowScale(void);
bool exporting16bit(void);
void set16bitExport(const bool bit16);

int fixedMap(const int64_t x, const int64_t in_min, const int64_t in_max,
			 const int64_t out_min, const int64_t out_max);

void setSampScale(const int64_t samp_len);
void setWaveScale(const int64_t line_len);
int scr2samp(const int64_t x);
int samp2scr(const int64_t x);

int hitbox(const Rect_t*, const int, const int);
void showErrorMsgBox(const char *title, const char *msg, const char *error);

int addColors(const int c1, const int c2);
int subColors(const int c1, const int c2);

bool loadSample(const char* file_path);
void openFileDialog(void);
void saveFileDialog(void);

const char* getFileNameWithoutExt(const char *file);

void setWorkingDir(char *dir);
char* getWorkingDir(void);

void initCursors(void);
void freeCursors(void);
void setHandCursor(void);
void setTextCursor(void);
void setDragCursor(void);
void setStandardCursor(void);

bool getRepaintTimer(void);

int getRelativeBrrSampBlock(const int samp, const int ref_pos);

bool isStringEmpty(const char *s);

char *_strcasestr(const char *haystack, const char *needle);
char *_strndup(char *str, size_t chars);

void print_args(const char *format, const int size, ...);

#define SBC_FREE(buffer) \
	if(buffer != NULL) \
	{ \
		free((void*) buffer); \
		buffer = NULL; \
	} \
	assert(buffer == NULL)

#define SBC_MALLOC(size, nmemb, buffer)  \
	errno = 0; \
	if((buffer = malloc(size * sizeof *buffer)) == NULL) \
	{ \
		showErrorMsgBox("Memory Allocation Error!", "Error while allocation memory! ", strerror(errno)); \
		exit(1); \
	} \
	assert(buffer != NULL)

#define SBC_CALLOC(size, nmemb, buffer) \
	errno = 0; \
	if((buffer = calloc(size, nmemb)) == NULL) \
	{ \
		showErrorMsgBox("Memory Allocation Error!", "Error while allocation memory! ", strerror(errno)); \
		exit(1); \
	} \
	assert(buffer != NULL)

#ifdef DEBUG

#define SBC_LOG(lbl, fmt,  ...)	\
{ \
	printf("\033[0;32m  ____SBC_LOG____: \033[0;37m %s  =  ", #lbl); \
	print_args(#fmt, 1, __VA_ARGS__); \
}  do {;} while(0)

#define SBC_ERR(lbl, err) fprintf(stderr, "\033[0;31m  ____SBC_ERR____: \033[0;37m %s: %s\n", lbl, err)

#else
#define SBC_LOG(lbl, fmt, ...) do {;} while(0)
#define SBC_ERR(lbl, err)  	   do {;} while(0)
#endif

#endif /* __SBC_UTILS_H */
