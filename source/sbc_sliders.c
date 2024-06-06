#include "sbc_utils.h"
#include "sbc_sliders.h"
#include "sbc_buttons.h"

#include "sbc_screen.h"
#include "sbc_audio.h"
#include "sbc_samp_edit.h"
#include "sbc_waveform.h"

Slider_t sampStartSlider = { 0, 0, START_SAMP_SLIDER, false };

Slider_t loopStartSlider = { 0, 0, START_LOOP_SLIDER, false },
         loopEndSlider   = { 0, 0, END_LOOP_SLIDER,   false };


static const uint8_t STRT_SLIDER[4] = { 0x4F, 0x43, 0xAE, 0xFF };
static const uint8_t LOOP_SLIDER[4] = { 0x9C, 0X53, 0XC1, 0XFF };
static const uint8_t STRT_CLICKD[4] = { 0xC0, 0xC0, 0xFF, 0xFF };
static const uint8_t LOOP_CLICKD[4] = { 0xC8, 0x85, 0xDB, 0xFF };

static bool update_slider = true;

static Slider_t *getSlider(const SliderType_t type)
{
    return type == sampStartSlider.type ? &sampStartSlider :
           type == loopStartSlider.type ? &loopStartSlider :
           type == loopEndSlider.type   ? &loopEndSlider   : 
           NULL;
}

int getSliderXPos(const SliderType_t type)
{
    const Slider_t *s = getSlider(type);
    return s->x_pos;
}

int getSliderSample(const SliderType_t type)
{
    const Slider_t *s = getSlider(type);
    return s->sample;
}

int sliderHitBox(const SliderType_t type, const int x, const int y)
{
    const Slider_t *s = getSlider(type);

	const Rect_t flag = { s->type != END_LOOP_SLIDER ? s->x_pos : s->x_pos - 8, 
                          s->type != END_LOOP_SLIDER ? 1 : SAMPLE_HEIGHT - 8, 8, 8, 0 };
    
	const Rect_t line = { s->x_pos - 1, 0, 2, SAMPLE_HEIGHT, 0 };

	if(hitbox(&flag, x, y)) return true;
	if(hitbox(&line, x, y)) return true;

	return false;
}

void clickSlider(const SliderType_t type, const bool click)
{
    Slider_t *s = getSlider(type);
    s->clicked = click;
}

bool wasSliderClicked(const SliderType_t type)
{
    const Slider_t *s = getSlider(type);
    return s->clicked;
}

void setLoopFromSlider(const SliderType_t type)
{
    Slider_t *s = getSlider(type);

    switch (type)
    {
    case START_SAMP_SLIDER:
        if (s->sample == *getSampStart()) return;

        setSampStart(s->sample);
        SBC_LOG(START SAMP SLIDER, %d, s->sample);

        if (s->sample > *getLoopStart())
        {
            setLoopStart(s->sample);
            SBC_LOG(START LOOP SLIDER,  %d, s->sample);
        }

        if (s->sample > *getLoopEnd())
        {
            setLoopEnd(s->sample);
            SBC_LOG(END LOOP SLIDER, %d, s->sample);
        }
        
        break;
    
    case START_LOOP_SLIDER:
        if (s->sample == *getLoopStart()) return;

        setLoopStart(s->sample);
        SBC_LOG(START LOOP SLIDER,  %d, s->sample);

        if (s->sample < *getSampStart())
        {
            setSampStart(s->sample);
            SBC_LOG(END LOOP SLIDER, %d, s->sample);
        }
        
        if (s->sample > *getLoopEnd())
        {
            setLoopEnd(s->sample);
            SBC_LOG(END LOOP SLIDER, %d, s->sample);
        }
        
        break;
    
    case END_LOOP_SLIDER:
        if (s->sample == *getLoopEnd()) return;

        setLoopEnd(s->sample);
        SBC_LOG(END LOOP SLIDER, %d, s->sample);

        if (s->sample < *getSampStart())
        {
            setSampStart(s->sample);
            SBC_LOG(END LOOP SLIDER, %d, s->sample);
        }
        
        if (s->sample < *getLoopStart())
        {
            setLoopStart(s->sample);
            SBC_LOG(START LOOP SLIDER,  %d, s->sample);
        }

        break;
    }
}

void setSliderPos(const SliderType_t type, const int samp)
{
    Slider_t *s = getSlider(type);

    assert(s != NULL);

    s->sample = samp < 0 ? 0 : samp > *getSampleEditLength() ? *getSampleEditLength() : samp;
    s->x_pos  = samp2scr(samp);    // yes we could base this on mouse x pos but more accurate to base on pos of sample
    if (s->x_pos == SCREEN_WIDTH) s->x_pos = SCREEN_WIDTH - 1;
}

void setBRRSlider(const SliderType_t type, const SliderType_t ref, const int samp)
{
    int new_samp = getRelativeBrrSampBlock(samp, getSliderSample(ref));
    setSliderPos(type, new_samp);
}

void setSlider(const SliderType_t type, const SliderType_t ref, const int x)
{
    const int samp = scr2samp(x);
    setBRRSlider(type, ref, samp);
}

void resetSliders(void)
{
    setSliderPos(START_SAMP_SLIDER, *getSampStart());
    setSliderPos(START_LOOP_SLIDER, *getLoopStart());
    setSliderPos(END_LOOP_SLIDER,   *getLoopEnd());
}

static void paintSliderVal(const int s, const int y, const char text[10])
{
    char val_text[20];

    const Rect_t text_area = { SCREEN_WIDTH - 64, y - 2, 62, 10, SBCDGREY };
    fill_rect(text_area);
    snprintf(val_text, 20, "%s: %08X", text, s);
    print_string_shadow(val_text, SCREEN_WIDTH - 160, y, (int[]) {1, 0}, (int[]) {SBCDPURPLE, SBCLGREY}, 1);
}

void paintSliderValues(void)
{
    const int smpl_lgth = *getSampleEditLength() > 1 ? *getSampleEditLength() - *getSampStart() : 0;

    paintSliderVal(*getSampStart(),                    SAMPLE_HEIGHT + 50, "SMPL STRT");
    paintSliderVal(*getLoopStart() - *getSampStart(),  SAMPLE_HEIGHT + 60, "LOOP STRT");
    paintSliderVal(*getLoopEnd()   - *getSampStart(),  SAMPLE_HEIGHT + 70, "LOOP END ");
    paintSliderVal(*getLoopEnd()   - *getLoopStart(),  SAMPLE_HEIGHT + 80, "LOOP LGTH");
    paintSliderVal(smpl_lgth,                          SAMPLE_HEIGHT + 90, "SMPL LGTH");
}


void paintSliders(void)
{
    if(*audioQueued()) paintSamplePos(samp2scr(*getSamplePos()));
    
    if(update_slider)
    {
        setSliderPos(START_SAMP_SLIDER, *getSampStart());
        setSliderPos(START_LOOP_SLIDER, *getLoopStart());
        setSliderPos(END_LOOP_SLIDER, *getLoopEnd());
    }

    if(sampStartSlider.x_pos > 0)
        paintIgnoredAreaOverlay(&sampStartSlider, 0);

    paint_slider(&sampStartSlider, sampStartSlider.clicked? STRT_CLICKD : STRT_SLIDER);
    
    if(!*isSampEditLoopEnabled()) return;

    if(loopEndSlider.x_pos > loopStartSlider.x_pos)
        paintLoopAreaOverlay(&loopStartSlider, &loopEndSlider);

    if(loopEndSlider.x_pos < SCREEN_WIDTH)
        paintIgnoredAreaOverlay(&loopEndSlider, SCREEN_WIDTH);

    paint_slider(&loopStartSlider, loopStartSlider.clicked? LOOP_SLIDER : LOOP_CLICKD);
    paint_slider(&loopEndSlider, loopEndSlider.clicked? LOOP_SLIDER : LOOP_CLICKD); 
}

bool mouseOverSlider(const int x, const int y)
{
    bool update_cursor = false;

    if(sliderHitBox(START_SAMP_SLIDER, x, y))
        update_cursor = true;
    else if (sampStartSlider.clicked)
        update_cursor = true;

    if(!wasButtonClicked(getLoopButton())) return update_cursor;
    else if(sliderHitBox(START_LOOP_SLIDER, x, y))
        update_cursor = true;
    else if(sliderHitBox(END_LOOP_SLIDER, x, y))
        update_cursor = true;
    else if (loopStartSlider.clicked)
        update_cursor = true;
    else if (loopEndSlider.clicked)
        update_cursor = true;

    return update_cursor;
}

void updateSliders(void) { update_slider = true; }
