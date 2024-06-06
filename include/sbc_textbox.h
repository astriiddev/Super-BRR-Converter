#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "sbc_defs.h"

void paintTextBoxes(void);
bool isTextboxEditing(void);

bool updateTextboxCursor(void);

bool mouseDownTextbox(const int mouse_x, const int mouse_y);
int mouseDragTextbox(const int mouse_x, const int mouse_y, const bool mouse_down);
bool textboxInputText(const char c);
bool textboxKeyInput(const uint8_t* keystate);

void initTextboxes(void);
void freeTextboxes(void);

void setSampleNameBoxText(const char* text);
void setSampleRateBoxText(const double rate);
void setResampleRateBoxText(const double rate);
 
#endif /* TEXTBOX_H */
