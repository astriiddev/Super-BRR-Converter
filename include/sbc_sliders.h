#ifndef __SBC_SLIDERS_H
#define __SBC_SLIDERS_H

#include "sbc_defs.h"

void setSlider(const SliderType_t type, const SliderType_t ref, const int x);
void setLoopFromSlider(const SliderType_t type);

int sliderHitBox(const SliderType_t type, const int x, const int y);
void clickSlider(const SliderType_t type, const bool click);
bool wasSliderClicked(const SliderType_t type);

bool mouseOverSlider(const int x, const int y);

void paintSliderValues(void);
void paintSliders(void);
void resetSliders(void);
void updateSliders(void);

#endif /* __SBC_SLIDERS_H */
