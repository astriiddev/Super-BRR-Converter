#include "sbc_utils.h"
#include "sbc_gui.h"

#include "sbc_buttons.h"
#include "sbc_waveform.h"
#include "sbc_sliders.h"

#include "sbc_samp_edit.h"
#include "sbc_optmenu.h"

#include "sbc_textbox.h"

static bool mouse_is_down  = false, repaint_gui = true;
static bool mouse_on_thumb = false, mouse_on_slider = false;

bool paintSBC(void)
{
    bool repaint = repaint_gui;

	if(updateTextboxCursor()) repaint = repaint_gui = true;
	if(!getRepaintTimer()) return false;

	paintGUI();



	if(paintWaveform()) repaint = true;;
	paintOptions();

	return repaint;
}

void repaintGUI(void) { repaint_gui = true; }

void paintGUI(void)
{
	if(!repaint_gui) return;

	paintButtons();
	drawLoopWindow();
	paintSliderValues();
	paintTextBoxes();

	repaint_gui = false;
}

bool mousedn(const int x, const int y)
{
	bool update = false;

	if (mouse_is_down) return false;

	mouse_is_down = true;

    if(mouseDownTextbox(x, y) || mouseDownOnButton(x, y)) repaint_gui = update = true;
	
	if(optionsIsShowing())
	{
		if(optMenuMouseDown(x, y))  repaint_gui = update = true;

		return update;
	}

	else if(hitbox(getScrollbarBack(), x, y) && isZoomed())
	{
		update = true;
		scrollClicked(true);
		setScrollFactor(x);
		mouse_on_thumb = true;
        repaintWaveform();
	}

	else if(sliderHitBox(START_SAMP_SLIDER, x, y) && !sliderHitBox(START_LOOP_SLIDER, x, y))
	{	
		clickSlider(START_SAMP_SLIDER, true);
		mouse_on_slider = true;
	}
	else if(sliderHitBox(START_LOOP_SLIDER, x, y))
	{	
		clickSlider(START_LOOP_SLIDER, true);
		mouse_on_slider = true;
	}
	else if(sliderHitBox(END_LOOP_SLIDER, x, y) && !sliderHitBox(START_LOOP_SLIDER, x, y))
	{
		clickSlider(END_LOOP_SLIDER, true);
		mouse_on_slider = true;
	}

    return update;
}

bool mouseup(const int x, const int y)
{
	bool update = false;

	if (!mouse_is_down) return false;

	mouse_is_down = false;

    if(mouseUpOnButton(x, y)) repaint_gui = update = true;

	if(optionsIsShowing()) return update;
    
	else if(mouse_on_thumb)
	{
		scrollClicked(false);
		update = true;
		mouse_on_thumb = false;
        repaintWaveform();
	}

	else if(wasSliderClicked(START_SAMP_SLIDER))
	{	
		clickSlider(START_SAMP_SLIDER, false);
		mouse_on_slider = false;
	}
	else if(wasSliderClicked(START_LOOP_SLIDER))
	{	
		clickSlider(START_LOOP_SLIDER, false);
		mouse_on_slider = false;
	}
	else if(wasSliderClicked(END_LOOP_SLIDER))
	{	
		clickSlider(END_LOOP_SLIDER, false);
		mouse_on_slider = false;
	}

    return update;
}

bool mousemotion(const int x, const int y)
{
	bool update = false;
	const int tb_mouse = mouseDragTextbox(x, y, mouse_is_down);

	if(optionsIsShowing())
	{
		if(mouseOverButton(x, y) || optMenuMouseMove(x, y, mouse_is_down)) 
			setHandCursor();
		else if(tb_mouse & 1)
			setTextCursor();
		else setStandardCursor();
	}
	else if(mouseOverButton(x, y) || mouseOverScrollBar(x, y)) 
		setHandCursor();
	else if(mouseOverSlider(x, y) && !optionsIsShowing())
		setDragCursor();
	else if(tb_mouse & 1)
		setTextCursor();
	else setStandardCursor();
	
	if(tb_mouse & 2) return (repaint_gui = update = true);
	if(!mouse_is_down || update) return update;

	if(optionsIsShowing()) return update;

	else if(mouse_on_thumb)
	{
		update = true;
		setScrollFactor(x);
	}

	else if(wasSliderClicked(START_SAMP_SLIDER))
	{
		setSlider(START_SAMP_SLIDER, START_LOOP_SLIDER, x);
		setLoopFromSlider(START_SAMP_SLIDER);
		repaint_gui = update = true;
	}
	else if(wasSliderClicked(START_LOOP_SLIDER))
	{
		setSlider(START_LOOP_SLIDER, START_SAMP_SLIDER, x);
		setLoopFromSlider(START_LOOP_SLIDER);
		repaint_gui = update = true;
	}
	else if(wasSliderClicked(END_LOOP_SLIDER))
	{
		setSlider(END_LOOP_SLIDER, START_SAMP_SLIDER, x);
		setLoopFromSlider(END_LOOP_SLIDER);
		repaint_gui = update = true;
	}

    return update;
}

bool isMouseOnSlider(void) { return mouse_on_slider; }

void handleSampleNameText(const char *samp_name) { setSampleNameBoxText(samp_name); }
void handleSampleRateText(const double rate) { setSampleRateBoxText(rate); }
void handleResampleRateText(const double rate) { setResampleRateBoxText(rate); }
