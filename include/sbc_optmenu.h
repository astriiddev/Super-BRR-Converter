#ifndef __SBC_OPTMENU_H
#define __SBC_OPTMENU_H

#include "sbc_defs.h"
#include "sbc_widgets.h"

void initOptMenu(void);
void freeOptMenu(void);

bool optMenuMouseDown(const int x, const int y);
bool optMenuMouseMove(const int x, const int y, const int mouse_is_down);

void paintOptions(void);
void hideOptions(void);
void showOptions(void);
void repaintOptions(void);

bool optionsIsShowing(void);
Button_t* getBrrButton(void);

#endif /* __SBC_OPTMENU_H */
