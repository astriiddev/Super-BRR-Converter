#ifndef __SBC_WAVEFORM_H
#define __SBC_WAVEFORM_H

#include "sbc_defs.h"

void handleZoom(const bool zoom_in);
void handleScroll(const bool scoll_left);

void resetZoom(void);
void updateWaveform(void);
void drawLoopWindow(void);

bool allocatePoints(const int len);
void freePoints(void);
void freeDrawingSampleBuffer(void);

void drawNewWave(void);

void scrollClicked(const bool clicked);
void setScrollFactor(const int mouse_x);

void repaintWaveform(void);
bool paintWaveform(void);

bool mouseOverScrollBar(const int x, const int y);

int *getWaveStart(void);
void setMouseFocus(const int x);

void setCursor(const int x);
void setSelEnd(const int x);

bool  isZoomed(void);
const Rect_t* getScrollbarBack(void);

bool *waveIsSelecting(void);
void setWaveSelecting(const bool select); 

void handleSelectAll(void);
void handleSampleCopy(void);
bool handleSampleCut(void);
bool handleSampleCrop(void);
bool handleSamplePaste(void);
bool handleSampleDelete(void);

#endif /* __SBC_WAVEFORM_H */
