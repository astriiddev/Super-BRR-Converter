#include <math.h>

#include "sbc_utils.h"
#include "sbc_textbox.h"
#include "sbc_samp_edit.h"

#define CLAMP(x, min, max) (x) < (min) ? (min) : (x) > (max) ? (max) : (x)

static Audio_Buffer_t *copy_buffer = NULL;
static Sample_t *edit_buffer = NULL, *undo_buffer = NULL;

static char* sample_name = NULL;
static double resample_rate = 16744.0;

void initSampleBuffers(void)
{
    SBC_CALLOC(1, sizeof *edit_buffer, edit_buffer);
    SBC_CALLOC(1, sizeof *undo_buffer, undo_buffer);
    SBC_CALLOC(1, sizeof *copy_buffer, copy_buffer);
}

void setSampleEdit(const Sample_t *samp)
{
    assert(samp != NULL && samp->audio.buffer != NULL);

    SBC_FREE(edit_buffer->audio.buffer);

    memset(edit_buffer, 0, sizeof(Sample_t));
    edit_buffer->audio.length = samp->audio.length;

    SBC_MALLOC(edit_buffer->audio.length, sizeof *edit_buffer->audio.buffer, edit_buffer->audio.buffer);
    memcpy(edit_buffer->audio.buffer, samp->audio.buffer, samp->audio.length * sizeof *samp->audio.buffer);

    edit_buffer->rate = samp->rate;
    
    setLoopEnable(samp->is_looped);
    
    setSampStart(samp->samp_start);
    setLoopEnd(samp->loop_end);
    setLoopStart(samp->loop_start);

    SBC_LOG(SAMPLE START, %d, edit_buffer->samp_start);
    SBC_LOG(LOOP START, %d, edit_buffer->loop_start);
    SBC_LOG(LOOP END, %d, edit_buffer->loop_end);

    SBC_LOG(SAMPLE LOOP LENGTH, %d, edit_buffer->loop_end - edit_buffer->loop_start);

    edit_buffer->pos = 0.0;
}

void clearSampleEdit(void)
{
    SBC_LOG(SAMPLE, %s, "CLEARED");
    edit_buffer->audio.length = 1;
    edit_buffer->rate = 16726.0;
    
    edit_buffer->is_looped = 0;
    
    setLoopStart(0);
    setLoopEnd(0);
    setSampStart(0);

    edit_buffer->pos = 0.0;

    SBC_FREE(edit_buffer->audio.buffer);
    SBC_CALLOC(edit_buffer->audio.length, sizeof *edit_buffer->audio.buffer, edit_buffer->audio.buffer);
}

void setSampEditName(const char* name)
{
    size_t name_len = 0;
    
    assert(name != NULL);
    
    name_len = strlen(name);

    SBC_FREE(sample_name);

    sample_name = _strndup((char*) name, name_len);
}

void setUndoBuffer(void)
{
    const size_t buffer_size = sizeof *edit_buffer->audio.buffer;

    if (edit_buffer->audio.buffer == NULL) return;

    SBC_FREE(undo_buffer->audio.buffer);

    memcpy(undo_buffer, edit_buffer, sizeof *undo_buffer);
    undo_buffer->audio.length = edit_buffer->audio.length;

    SBC_CALLOC(undo_buffer->audio.length, buffer_size, undo_buffer->audio.buffer);
    
    memcpy(undo_buffer->audio.buffer, edit_buffer->audio.buffer, edit_buffer->audio.length * buffer_size);
}

bool handleUndo(void)
{
    const size_t buffer_size = sizeof *edit_buffer->audio.buffer;

    if(undo_buffer->audio.buffer == NULL || undo_buffer->audio.length <= 1) return false;

    SBC_FREE(edit_buffer->audio.buffer);

    memcpy(edit_buffer, undo_buffer, sizeof *edit_buffer);
    edit_buffer->audio.length = undo_buffer->audio.length;
    
    SBC_CALLOC(undo_buffer->audio.length, buffer_size, edit_buffer->audio.buffer);

    memcpy(edit_buffer->audio.buffer, undo_buffer->audio.buffer, undo_buffer->audio.length * buffer_size);

    SBC_FREE(undo_buffer->audio.buffer);

    return true;
}

void copySampleRange(const int start, const int end)
{
    const int range = end - start;

    if(edit_buffer->audio.buffer == NULL || edit_buffer->audio.length <= 1) return;

    SBC_FREE(copy_buffer->buffer);

    copy_buffer->length = range > 0 ? range : 1;

    SBC_CALLOC(copy_buffer->length, sizeof *copy_buffer->buffer, copy_buffer->buffer);

    memcpy(copy_buffer->buffer, edit_buffer->audio.buffer + start, copy_buffer->length * sizeof *copy_buffer->buffer);
}

bool cutAtCursor(const int index)
{
    copySampleRange(index, index);
    return deleteSingleSample(index);
}

bool cutSampleRange(const int start, const int end)
{
    copySampleRange(start, end);
    return deleteRangeSample(start, end, true);
}

bool cropSampleRange(const int start, const int end)
{
    Audio_Buffer_t temp_buffer = { NULL, end - start };
    const size_t buffer_size = sizeof *edit_buffer->audio.buffer;

    printf("Cropping...\n");
    
    if(edit_buffer->audio.buffer == NULL || edit_buffer->audio.length <= 1) return false;

    if(start < 0 || start > edit_buffer->audio.length) return false;
    if(end   < 0 || end   > edit_buffer->audio.length) return false;

    if(temp_buffer.length <= 1) 
    {
        clearSampleEdit();
        return true;
    }

    setUndoBuffer();

    SBC_CALLOC(temp_buffer.length, sizeof *temp_buffer.buffer, temp_buffer.buffer);
    
    memcpy(temp_buffer.buffer, edit_buffer->audio.buffer + start, temp_buffer.length * buffer_size);

    SBC_FREE(edit_buffer->audio.buffer);

    edit_buffer->audio.length = temp_buffer.length;

    SBC_CALLOC(edit_buffer->audio.length, buffer_size, edit_buffer->audio.buffer);

    memcpy(edit_buffer->audio.buffer, temp_buffer.buffer, edit_buffer->audio.length * buffer_size);

    setSampStart(0);
    
    if(edit_buffer->loop_start > end) setLoopStart(0);
    else setLoopStart(edit_buffer->loop_start - start);

    if(edit_buffer->loop_end < end) setLoopEnd(edit_buffer->loop_end - start);
    else setLoopEnd(edit_buffer->audio.length);

    SBC_FREE(temp_buffer.buffer);

    return true;
}

bool pasteAtCursor(const int index, const bool set_undo)
{
    Audio_Buffer_t temp_buffer = { NULL, copy_buffer->length + edit_buffer->audio.length };
    const size_t buffer_size = sizeof *edit_buffer->audio.buffer;

    if(copy_buffer->buffer == NULL || copy_buffer->length < 1) return false;

    if(index < 0) return false;  

    if(set_undo) setUndoBuffer();

    if(edit_buffer->audio.length == 1) temp_buffer.length--;

    SBC_CALLOC(temp_buffer.length, buffer_size, temp_buffer.buffer);

    if(edit_buffer->audio.buffer != NULL && edit_buffer->audio.length > 1)
    {
        memcpy(temp_buffer.buffer, edit_buffer->audio.buffer, index * buffer_size);

        if(index < edit_buffer->audio.length)
        {
            memcpy(temp_buffer.buffer + index + copy_buffer->length, edit_buffer->audio.buffer + index, 
                  (edit_buffer->audio.length - index) * buffer_size);
        }
    }

    if (temp_buffer.buffer == NULL) return false;
    memcpy(temp_buffer.buffer + index, copy_buffer->buffer, copy_buffer->length * buffer_size);

    SBC_FREE(edit_buffer->audio.buffer);

    edit_buffer->audio.length = temp_buffer.length;
    SBC_CALLOC(edit_buffer->audio.length, buffer_size, edit_buffer->audio.buffer);

    memcpy(edit_buffer->audio.buffer, temp_buffer.buffer, temp_buffer.length * buffer_size);

    if(edit_buffer->samp_start > index) setSampStart(edit_buffer->samp_start + copy_buffer->length);
    if(edit_buffer->loop_start > index) setLoopStart(edit_buffer->loop_start + copy_buffer->length);
    if(edit_buffer->loop_end   > index) setLoopEnd(edit_buffer->loop_end     + copy_buffer->length);
    
    SBC_FREE(temp_buffer.buffer);

    return true;
}

bool pasteOverRange(const int start, const int end)
{
    setUndoBuffer();

    if(deleteRangeSample(start, end, false) == false) return false;
    return pasteAtCursor(start, false);
}

bool deleteSingleSample(const int index)
{
    Audio_Buffer_t temp_buffer = { NULL, edit_buffer->audio.length - 1 };
    const size_t buffer_size = sizeof *edit_buffer->audio.buffer;

    const int byte_index = index * (int) buffer_size;
    
    if(edit_buffer->audio.buffer == NULL || edit_buffer->audio.length <= 1) return false;
    if(index < 0 || index > edit_buffer->audio.length) return false;

    setUndoBuffer();

    if(temp_buffer.length <= 1)
    {
        clearSampleEdit();
        return true;
    }

    SBC_MALLOC(temp_buffer.length, buffer_size, temp_buffer.buffer);

    if(index < edit_buffer->audio.length)
    {
        memcpy(temp_buffer.buffer, edit_buffer->audio.buffer, byte_index);
        memcpy(temp_buffer.buffer + index, edit_buffer->audio.buffer + index + 1, 
                (temp_buffer.length * buffer_size) - byte_index);
    }
    else
    {
        memcpy(temp_buffer.buffer, edit_buffer->audio.buffer, byte_index - 1);
    }

    edit_buffer->audio.length = temp_buffer.length;

    SBC_FREE(edit_buffer->audio.buffer);

    SBC_MALLOC(edit_buffer->audio.length, buffer_size, edit_buffer->audio.buffer);
    memcpy(edit_buffer->audio.buffer, temp_buffer.buffer, temp_buffer.length * buffer_size);

    if(index <  edit_buffer->samp_start) setSampStart(edit_buffer->samp_start - 1);
    if(index <  edit_buffer->loop_start) setLoopStart(edit_buffer->loop_start - 1);
    if(index <= edit_buffer->loop_end)   setLoopEnd(edit_buffer->loop_end - 1);

    SBC_FREE(temp_buffer.buffer);

    return true;
}

bool deleteRangeSample(const int start, const int end, const bool set_undo)
{
    const int range = end - start;
    Audio_Buffer_t temp_buffer = { NULL, edit_buffer->audio.length - range};
    const size_t buffer_size = sizeof *edit_buffer->audio.buffer;

    assert(end >= start);
    
    if(edit_buffer->audio.buffer == NULL || edit_buffer->audio.length <= 1) return false;

    if(start == end) return deleteSingleSample(start);

    if(start < 0 || start > edit_buffer->audio.length) return false;
    if(end   < 0 || end   > edit_buffer->audio.length) return false;

    if(set_undo) setUndoBuffer();

    if(temp_buffer.length <= 1)
    {
        clearSampleEdit();
        return true;
    }

    SBC_MALLOC(temp_buffer.length, buffer_size, temp_buffer.buffer);
    memcpy(temp_buffer.buffer, edit_buffer->audio.buffer, start * buffer_size);

    if(end < edit_buffer->audio.length) 
        memcpy(temp_buffer.buffer + start, edit_buffer->audio.buffer + end, 
                (edit_buffer->audio.length - end) * buffer_size);

    edit_buffer->audio.length = temp_buffer.length;

    SBC_MALLOC(edit_buffer->audio.length, sizeof *edit_buffer->audio.buffer, edit_buffer->audio.buffer);
    memcpy(edit_buffer->audio.buffer, temp_buffer.buffer, temp_buffer.length * buffer_size);
    
    if(edit_buffer->samp_start > end) setSampStart(edit_buffer->samp_start - range);
    else if(edit_buffer->samp_start > start) setSampStart(start);

    if(edit_buffer->loop_start > end) setLoopStart(edit_buffer->loop_start - range);
    else if(edit_buffer->loop_start > start) setLoopStart(start);

    if(edit_buffer->loop_end > end) setLoopEnd(edit_buffer->loop_end - range);
    else if(edit_buffer->loop_end > start) setLoopEnd(start);

    SBC_FREE(temp_buffer.buffer);

    return true;
}

void setResampleRate(const double rate) { resample_rate = rate; }

int get_resample_val(const int in, const double ratio) { return (int) ceil((double) in / ratio); }

bool handleResample(void)
{
    bool success = true;
    Sample_t *resample_buffer = NULL;

    const size_t buffer_size = sizeof *edit_buffer->audio.buffer;
    const double resample_ratio = edit_buffer->rate / resample_rate;

    if(resample_rate == edit_buffer->rate) return false;
    if(resample_rate < 1000 || resample_rate > 48000) return false;
    if(edit_buffer->audio.buffer == NULL || edit_buffer->audio.length < 2) return false;
    
    setUndoBuffer();

    SBC_CALLOC(1, sizeof(Sample_t), resample_buffer);

    resample_buffer->rate = resample_rate;

    resample_buffer->audio.length = get_resample_val(edit_buffer->audio.length, resample_ratio);
  
    resample_buffer->samp_start   = get_resample_val(edit_buffer->samp_start, resample_ratio);
    resample_buffer->loop_start   = get_resample_val(edit_buffer->loop_start, resample_ratio);
    resample_buffer->loop_end     = get_resample_val(edit_buffer->loop_end,   resample_ratio);

    resample_buffer->is_looped = edit_buffer->is_looped;

    resample_buffer->pos = 0.0;
    
    SBC_CALLOC(resample_buffer->audio.length, buffer_size, resample_buffer->audio.buffer);

    for(int i = 0; i < resample_buffer->audio.length; i++)
    {
        const int pos = (int) floor(resample_buffer->pos);

        if(pos >= edit_buffer->audio.length) break;

        resample_buffer->audio.buffer[i] = edit_buffer->audio.buffer[pos];

        resample_buffer->pos += resample_ratio;
    }

    setSampleEdit(resample_buffer);

    SBC_FREE(resample_buffer->audio.buffer);
    SBC_FREE(resample_buffer);

    return success;
}

char *getSampEditName(void) { return sample_name; }

Sample_t *getSampleEdit(void) { return edit_buffer; }
int16_t **getSampleEditBuffer(void) { return &edit_buffer->audio.buffer; }

void setLoopEnable(const int enable) { edit_buffer->is_looped = edit_buffer->audio.buffer == NULL ? false : enable; }
_Atomic bool *isSampEditLoopEnabled(void) { return &edit_buffer->is_looped; }

void setSampStart(const int samp) { edit_buffer->samp_start = CLAMP(samp, 0, edit_buffer->loop_start); }
void setLoopStart(const int samp) { edit_buffer->loop_start = CLAMP(samp, edit_buffer->samp_start, edit_buffer->loop_end); }
void setLoopEnd(const int samp)   { edit_buffer->loop_end   = CLAMP(samp, edit_buffer->loop_start, edit_buffer->audio.length); }

_Atomic int *getSampStart(void) { return &edit_buffer->samp_start; }
_Atomic int *getLoopStart(void) { return &edit_buffer->loop_start; }
_Atomic int *getLoopEnd(void)   { return &edit_buffer->loop_end; }

_Atomic int *getSampleEditLength(void) 
{ 
    if(edit_buffer->audio.buffer == NULL) 
        edit_buffer->audio.length = 1; 
        
    return &edit_buffer->audio.length; 
}

void setSampleEditSampleRate(const double rate) { edit_buffer->rate = rate; }
_Atomic double *getSampleEditSampleRate(void) { return &edit_buffer->rate; }

void resetSampPos(void) { edit_buffer->pos = (double) edit_buffer->samp_start; }

void cleanUpAndFreeSampleEdit(void)
{
    SBC_FREE(edit_buffer->audio.buffer);
    SBC_FREE(sample_name);

    SBC_FREE(undo_buffer->audio.buffer);
    SBC_FREE(copy_buffer->buffer);

    SBC_FREE(edit_buffer);
    SBC_FREE(copy_buffer);
    SBC_FREE(undo_buffer);
}
