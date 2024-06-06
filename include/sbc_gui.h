#ifndef __SBC_GUI_H
#define __SBC_GUI_H

#include "sbc_defs.h"

bool paintSBC(void);

void paintGUI(void);
void repaintGUI(void);
void repaintOptions(void);

bool mousedn(const int, const int);
bool mouseup(const int, const int);
bool mousemotion(const int x, const int y);

bool isMouseOnSlider(void);

void handleSampleNameText(const char* samp_name);
void handleSampleRateText(const double rate);
void handleResampleRateText(const double rate);

#endif /* __SBC_GUI_H */
