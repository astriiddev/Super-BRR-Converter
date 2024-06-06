#ifndef __SBC_DEFS_H
#define __SBC_DEFS_H

#include <stdio.h>

#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#define ROUND32 0x80000000

#define SCREEN_WIDTH	540
#define SCREEN_HEIGHT	320
#define SAMPLE_HEIGHT 	160
#define SAMPLE_Y_CENTRE 80

extern enum
{
	SBCMPURPLE  = (int) 0xFF4F43AE,
	SBCDPURPLE  = (int) 0xFF2F238E,
	SBCLPURPLE  = (int) 0xFFC8C0CF,
	SBCDGREY    = (int) 0xFFAAAAAA,
	SBCLGREY	= (int) 0xFFC0C0C0,
	SCROLLBACK	= (int) 0xFF3B3283,
	SCROLLPURP	= (int) 0xFFB5B6E4,
	SCROLLPINK	= (int) 0xFFC885DB,
	TRACKGREY	= (int) 0xFF9E9E9E
} SBCPALETTE;

typedef struct Pixel_Buffer_s
{
    uint32_t *buffer;

    int width;
    int height;

    int scale;

    bool update;
    
} Pixel_Buffer_t;

typedef struct Rect_s
{
	int x,
	    y,
	    w,
	    h;

	int c;
} Rect_t;

typedef enum
{
    END_LOOP_SLIDER   = 4,
    START_SAMP_SLIDER = 1,
    START_LOOP_SLIDER = 2
} SliderType_t;

#endif /* __SBC_DEFS_H */
