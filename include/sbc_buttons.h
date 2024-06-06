#ifndef __SBC_BUTTONS_H
#define __SBC_BUTTONS_H

#include "sbc_widgets.h"

void initButtons(void);
void freeButtons(void);

void paintButtons(void);

bool mouseDownOnButton(const int x, const int y);
bool mouseUpOnButton(const int x, const int y);
bool mouseOverButton(const int x, const int y);

Button_t* getPlayButton(void);
Button_t* getLoopButton(void);

Button_t* getOptButton(void);

#endif /* __SBC_BUTTONS_H */
