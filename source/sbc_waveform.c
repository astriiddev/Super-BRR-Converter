#include <math.h>

#include "sbc_utils.h"
#include "sbc_waveform.h"
#include "sbc_screen.h"
#include "sbc_sliders.h"
#include "sbc_samp_edit.h"

/*
*	TODO: reduce number of static stack variables....
*/

static const Rect_t scroll_bar_back  = 
{ 
	5, SAMPLE_HEIGHT + 3, 
	SCREEN_WIDTH - 10, 16, 
	SCROLLBACK 
};

static Rect_t select_wave = 
{ 
	0, 0, 0, 
	SAMPLE_HEIGHT, 
	SCROLLBACK 
};

static Rect_t scroll_bar  = 
{ 
	7, SAMPLE_HEIGHT + 5, 
	SCREEN_WIDTH - 14, 12, 
	SCROLLPURP 
};

static struct WaveArea_t {
	int start, end, width;
} wave_area  = { 0, 0, 0}, select_area = { 0, 0, /* unused */ 0};

static int *point_x = NULL, *point_y = NULL;
static int16_t *sample_buffer = NULL;

static int samp_length = 0, mouse_focus = 0, scroll_factor = 0;
static double zoom_divider = 0.0;

static bool wave_changed = true, repaint_wave = true, wave_can_select = false;

bool allocatePoints(const int len)
{
	bool success = true;

	freePoints();

	SBC_MALLOC(len, sizeof *point_x, point_x);
	SBC_MALLOC(len, sizeof *point_y, point_y);

	if (point_x == NULL) success = false;
	else if (point_y == NULL) success = false;
	
	return success;
}

void freePoints(void)
{
	SBC_FREE(point_x);
	SBC_FREE(point_y);
}

void freeDrawingSampleBuffer(void)
{
	SBC_FREE(sample_buffer);
	SBC_FREE(point_x);
	SBC_FREE(point_y);
}

static void set_point_x(const int index, const int x)
{
	point_x[index] = x;

	if(point_x[index] >= SCREEN_WIDTH)
		point_x[index] = SCREEN_WIDTH;
}

static void set_point_y(const int index, const int y)
{
	point_y[index] = y;

	if(point_y[index] >= SAMPLE_HEIGHT)
		point_y[index] = SAMPLE_HEIGHT;
}

static int vert_map(const int x) { return SAMPLE_Y_CENTRE - ((x * SAMPLE_HEIGHT) >> 16); }

void drawNewWave(void)
{
	if(*getSampleEditBuffer() == NULL)  return;

	samp_length = *getSampleEditLength();
	
	setSampScale(samp_length);
	setWaveScale(samp_length);

	SBC_FREE(sample_buffer);
	SBC_MALLOC(samp_length, sizeof *sample_buffer, sample_buffer);

	memcpy(sample_buffer, *getSampleEditBuffer(), samp_length * sizeof *sample_buffer);
	allocatePoints(samp_length + 1);

	for (int i = 0; i < samp_length; i++)
		set_point_y(i, vert_map(sample_buffer[i]));

	set_point_y(samp_length, point_y[samp_length - 1]);

	wave_area.start = 0;
	wave_area.end = samp_length;

	if(select_area.start < 0) select_area.start = 0;
	else if(select_area.start > samp_length) select_area.start = samp_length;

	select_area.end = select_area.start;

	wave_changed = true;
}

void handleZoom(const bool zoom_in)
{
	if (samp_length <= 1) return;

	if (zoom_in)
	{
		if(wave_area.width <= 2) return; 

		zoom_divider += (0.5 - zoom_divider) / 2;
	}
	else
	{
		if(wave_area.width >= samp_length || zoom_divider < 0.0) 
		{
			zoom_divider = scroll_factor = 0;
			return;
		}

		zoom_divider -= (0.5 - zoom_divider) / 2;
		mouse_focus = (wave_area.end + wave_area.start) >> 1;
	}

	if (mouse_focus > 0) scroll_factor = mouse_focus - (samp_length >> 1);

	SBC_LOG(LINE LENGTH, %d, wave_area.width);
	SBC_LOG(ZOOM DIVIDER, %f, zoom_divider);

	updateSliders();
	repaint_wave = wave_changed = true;
}

void handleScroll(const bool scroll_left)
{
	if (scroll_left)
	{
		if (wave_area.start <= 0) return;

		if (wave_area.width < 16) scroll_factor--;
		else scroll_factor -= (wave_area.width / 8);

		if (scroll_factor <= -(samp_length / 2)) scroll_factor = -(samp_length / 2) + 1;
	}
	else
	{
		if (wave_area.end >= samp_length) return;

		if (wave_area.width < 16) scroll_factor++;
		else scroll_factor += (wave_area.width / 8);

		if (scroll_factor >= samp_length / 2) scroll_factor = samp_length / 2 - 1;
	}

	SBC_LOG(SCROLL FACTOR, %d, scroll_factor);

	updateSliders();
	wave_changed = true;
}

static int scroll_bar_x(const Rect_t *sb)
{
	const int scroll_min = 7, 
			  scroll_max = SCREEN_WIDTH - sb->w - scroll_min,
			  offset = sb->w <= 5 ? scroll_min : 0;

	int scroll_x = 0;

	if(wave_area.start <= 0) return scroll_min;
	if(wave_area.end   >= samp_length) return scroll_max;
	
	// once upon a time, only God and myself knew why this worked. Now, only God knows...
	scroll_x = (-point_x[0] * wave_area.width / samp_length) + (scroll_min - (14 * wave_area.start / (samp_length - offset)));

	return scroll_x < scroll_min ? scroll_min : scroll_x > scroll_max ? scroll_max : scroll_x;
}

static void redraw_wave(void)
{
	int wave_adjust = 0;

    if(point_x == NULL) return;
	
	wave_adjust = (int) floor((double) samp_length * zoom_divider);

	if (wave_area.end < 0) wave_area.end = samp_length;
	if (mouse_focus == 0) mouse_focus = (wave_area.end + wave_area.start) >> 1;

	if (scroll_factor < -wave_adjust) scroll_factor = -wave_adjust;
	else if (scroll_factor > wave_adjust) scroll_factor = wave_adjust;

	wave_area.start = wave_adjust + scroll_factor;
	wave_area.end = (samp_length - wave_adjust + scroll_factor);

	if (wave_area.end > samp_length) wave_area.end = samp_length;
	if (wave_area.start < 0) wave_area.start = 0;
        
	if (wave_area.end <= wave_area.start) 
	{
		wave_area.end = wave_area.start + 1;
		wave_area.start -= 1;
	}

	wave_area.width = wave_area.end - wave_area.start;

	setSampScale(wave_area.width);
	setWaveScale(wave_area.width);

	for (int i = 0; i < samp_length; i++)   
		set_point_x(i, samp2scr(i));

	if(wave_area.end >= samp_length) set_point_x(samp_length, SCREEN_WIDTH);

	scroll_bar.w = (SCREEN_WIDTH - 14) * wave_area.width / samp_length;

	if (scroll_bar.w <= 5) scroll_bar.w = 5;

	scroll_bar.x = scroll_bar_x(&scroll_bar);

	wave_changed = false;
}

void resetZoom(void)
{
	scroll_factor = 0;
	zoom_divider = 0;

	wave_area.start = 0;
	wave_area.end = samp_length;

	select_area.start = select_area.end = 0;
	select_wave.x = select_wave.w = 0;

	redraw_wave();
	updateWaveform();
}

static void draw_wave_lines(void)
{
	for(int i = 0; i <= SCREEN_WIDTH; i++)
	{
		int x1, y1, x2, y2;

		int curr_samp = scr2samp(i + 0);
		int next_samp = scr2samp(i + 1);

		if(curr_samp >= samp_length) curr_samp = samp_length - 1;
		if(next_samp >= samp_length) next_samp = samp_length;

		x1 = point_x[curr_samp];
		y1 = point_y[curr_samp];
		x2 = point_x[next_samp];
		y2 = point_y[next_samp];

		draw_line(x1, y1, x2, y2);
	}
}

static void get_min_max(const int start, const int end, int *ymin, int *ymax)
{
	int samp_min = INT16_MAX, samp_max = INT16_MIN,
		curr_end = end >= samp_length ? samp_length - 1 : end;

	for (int i = start; i <= curr_end; i++)
	{
		int curr_samp;
		
		curr_samp = sample_buffer[i];

		if (curr_samp < samp_min) 
		{
			samp_min = curr_samp;
			*ymax = point_y[i];
		}

		if (curr_samp > samp_max) 
		{
			samp_max = curr_samp;
			*ymin = point_y[i];
		}
	}
}

static void draw_wave_polygons(void)
{
	int last_ymin = point_y[0], last_ymax = point_y[0];

	for(int i = 0; i <= SCREEN_WIDTH; i++)
	{
		int curr_samp = scr2samp(i + 0);
		int next_samp = scr2samp(i + 1);

		if(curr_samp >= samp_length) curr_samp = samp_length - 1;
		if(next_samp >= samp_length) next_samp = samp_length;

		int ymin = 0, ymax = 0;
		get_min_max(curr_samp, next_samp, &ymin, &ymax);

		if(i > 0)
		{
			if(ymin >= last_ymax) draw_line(i - 1, ymin, i, last_ymax);
			if(ymax <= last_ymin) draw_line(i - 1, ymax, i, last_ymin);
		}

		draw_line(i, ymin, i, ymax);

		last_ymin = ymin;
		last_ymax = ymax;
	}
}

static void draw_waveform(void)
{
	assert(point_x != NULL && point_y != NULL && *getSampleEditBuffer() != NULL);
	if (wave_changed) redraw_wave();
	
	if(*getSampleEditLength() < 2) return;
        
	select_wave.x = point_x[select_area.start];
	select_wave.w = point_x[select_area.end] - point_x[select_area.start];

	if (select_wave.w != 0) fill_rect(select_wave);

	if(wave_area.width > SCREEN_WIDTH) draw_wave_polygons();
	else draw_wave_lines();
}

void updateWaveform(void)
{
	if(!repaint_wave) return;

	clearScreenArea(0, SBCLGREY, SCREEN_WIDTH * SAMPLE_HEIGHT);

	fill_rect(scroll_bar_back);

	draw_waveform();

	draw_Vline(select_wave.x, 0, SAMPLE_HEIGHT, SBCDPURPLE);

	draw_Hline(0, SAMPLE_Y_CENTRE, SCREEN_WIDTH, SBCDPURPLE);

	if (select_wave.w != 0 && select_area.start != select_area.end)
	{
		const int line_color = wave_area.width > SCREEN_WIDTH ? 0 : SBCLPURPLE;

		draw_Hline(select_wave.x, SAMPLE_Y_CENTRE, select_wave.w, line_color);
	}

	fill_rect(scroll_bar);

	paint_bevel((Rect_t) {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0}, 0xFF2E2E2E, 0xFFBEBEBE);
	draw_rect((Rect_t) {0, 0, SCREEN_WIDTH, SAMPLE_HEIGHT + 1, SBCDPURPLE });

    repaint_wave = false;
}

void drawLoopWindow(void)
{
	const int win_width = 96, win_height = 75;
	const int loop_end = *getLoopEnd(), loop_start = *getLoopStart();

	Rect_t loopend_win   = { 160, SAMPLE_HEIGHT + 58, win_width, win_height, SBCLPURPLE };
	Rect_t loopstart_win = { 256, SAMPLE_HEIGHT + 58, win_width + 1, win_height, SBCLPURPLE };

	const int loopwin_ycent = loopstart_win.y + (loopstart_win.h / 2);

	int end_i = win_width, start_i = 0;

	fill_rect(loopend_win);
	fill_rect(loopstart_win);

	for(int i = 0; i < win_width; i++)
	{
		int x1 = loopend_win.x + i, x2 = loopend_win.x + i + 1, y1 = loopwin_ycent, y2 = y1;

		if(sample_buffer == NULL || !*isSampEditLoopEnabled()) break;

		if(loop_end - end_i >= 0)
		{
			y1 = fixedMap(sample_buffer[loop_end - end_i - 1], INT16_MAX, INT16_MIN, loopend_win.y + 1, loopend_win.y + loopend_win.h - 1);
			y2 = fixedMap(sample_buffer[loop_end - end_i - 0], INT16_MAX, INT16_MIN, loopend_win.y + 1, loopend_win.y + loopend_win.h - 1);
		}

		draw_line(x1, y1, x2, y2);

		x1 = loopstart_win.x + i, x2 = loopstart_win.x + i + 1, y1 = loopwin_ycent, y2 = y1;

		if(loop_start + start_i + 1 < *getSampleEditLength() - 1)
		{
			y1 = fixedMap(sample_buffer[loop_start + start_i - 1], INT16_MAX, INT16_MIN, loopstart_win.y + 1, loopstart_win.y + loopstart_win.h - 1);
			y2 = fixedMap(sample_buffer[loop_start + start_i - 0], INT16_MAX, INT16_MIN, loopstart_win.y + 1, loopstart_win.y + loopstart_win.h - 1);
		}

		draw_line(x1, y1, x2, y2);

		start_i++;
		end_i--;
	}

	fill_rect((Rect_t) {loopend_win.x + loopend_win.w - 5, loopend_win.y + loopend_win.h - 6, 5, 5, SCROLLPINK});
	fill_rect((Rect_t) {loopstart_win.x + 1, loopend_win.y + 1, 5, 5, SCROLLPINK});

	draw_Hline(loopend_win.x, loopwin_ycent, win_width * 2 + 1, SBCDPURPLE);
	draw_Vline(loopend_win.x + win_width, loopstart_win.y, loopstart_win.h, SBCDPURPLE);

	paint_bevel((Rect_t) {loopend_win.x, loopend_win.y, win_width * 2 + 1, win_height, 0}, 
				loopstart_win.c + 0x00101010, loopstart_win.c - 0x00505050);

	print_string("END", loopend_win.x + 3, loopend_win.y + loopend_win.h - 10, SBCDPURPLE, 1);
	print_string("START", loopstart_win.x + loopstart_win.w - 43, loopstart_win.y + loopstart_win.h - 10, SBCDPURPLE, 1);
}

void repaintWaveform(void) { repaint_wave = true; }

bool paintWaveform(void)
{
	const bool repaint = repaint_wave;
	if(!repaint) return false;

	updateWaveform();

	return repaint;
}

void scrollClicked(const bool click) { scroll_bar.c = click ? SCROLLPINK : SCROLLPURP; }

bool mouseOverScrollBar(const int x, const int y)
{
	if(sample_buffer == NULL) return false;
	if(wave_area.width == *getSampleEditLength()) return false;
	
	return hitbox(&scroll_bar_back, x, y);
}

void setScrollFactor(const int mouse_x)
{
	const int scroll_length = fixedMap(mouse_x, 0, SCREEN_WIDTH, 0, samp_length);
	scroll_factor = scroll_length - (samp_length >> 1);

	SBC_LOG(SCROLL FACTOR, %d, scroll_factor);
	updateSliders();

	wave_changed = true;
}

int *getWaveStart(void) { return &wave_area.start; }
void setMouseFocus(const int x)	{ mouse_focus = x;}

void setCursor(const int x)     
{ 
	select_area.start = select_area.end = getRelativeBrrSampBlock(x, *getSampStart());  
	SBC_LOG(MOUSE CURSOR, %d, select_area.start ); 
}

void setSelEnd(const int x)		
{ 
	select_area.end = getRelativeBrrSampBlock(x, *getSampStart());

	if (select_area.end < wave_area.start) select_area.end = wave_area.start;
	else if (select_area.end > wave_area.end) select_area.end = wave_area.end;

	SBC_LOG(SELECT END \t, %d, select_area.end);
}

void handleSelectAll(void)
{
	select_area.start = 0;
	select_area.end = samp_length;
	
	SBC_LOG(SELECT START, %d, select_area.start);
	SBC_LOG(SELECT END \t, %d, select_area.end);
}

void handleSampleCopy(void)
{
	const int start = select_area.start < select_area.end ? select_area.start : select_area.end,
			  end   = select_area.start < select_area.end ? select_area.end : select_area.start;

	copySampleRange(start, end);	
}

bool handleSampleCut(void)
{
	const int start = select_area.start < select_area.end ? select_area.start : select_area.end,
			  end   = select_area.start < select_area.end ? select_area.end : select_area.start;

	if(start == end) return cutAtCursor(start);
	else return cutSampleRange(start, end);
}

bool handleSampleCrop(void)
{
	const int start = select_area.start < select_area.end ? select_area.start : select_area.end,
			  end   = select_area.start < select_area.end ? select_area.end : select_area.start;

	if(start == end) return false;
	else return cropSampleRange(start, end);
}

bool handleSamplePaste(void)
{
	const int start = select_area.start < select_area.end ? select_area.start : select_area.end,
			  end   = select_area.start < select_area.end ? select_area.end : select_area.start;

	if(start == end)  return pasteAtCursor(start, true);
	return pasteOverRange(start, end);
}

bool handleSampleDelete(void)
{
	bool update = false;

	const int start = select_area.start < select_area.end ? select_area.start : select_area.end,
			  end   = select_area.start < select_area.end ? select_area.end : select_area.start;

	int new_len = 0;

	if(start == end) 
	{
		update = deleteSingleSample(start);
	}
	else 
	{
		update = deleteRangeSample(start, end, true);
		select_area.end = select_area.start = start;
	}
	
	new_len = *getSampleEditLength();

	if(new_len <= 0) select_area.end = select_area.start = 0;
	else if(select_area.end >= new_len) select_area.end = select_area.start = new_len - 1;
	
	SBC_LOG(SELECT START, %d, select_area.start);
	SBC_LOG(SELECT END \t, %d, select_area.end);

	return update;
}

bool isZoomed(void) { return (wave_area.width != samp_length); }
const Rect_t *getScrollbarBack(void) { return &scroll_bar_back; }

bool *waveIsSelecting(void) { return &wave_can_select; }
void setWaveSelecting(const bool select) { wave_can_select = select; }
