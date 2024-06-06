#ifndef __SBC_SAMP_EDIT_H
#define __SBC_SAMP_EDIT_H

#include <stdatomic.h>
#include "sbc_defs.h"

typedef struct 
{
    int16_t *buffer;
    _Atomic int length;
} Audio_Buffer_t;

typedef struct
{
    Audio_Buffer_t audio;

    _Atomic bool is_looped;
    _Atomic int loop_start;
    _Atomic int loop_end;
    _Atomic int samp_start;

	_Atomic double rate;
	_Atomic double pos;
} Sample_t;

void initSampleBuffers(void);
void setSampleEdit(const Sample_t *);
void clearSampleEdit(void);

void setLoopEnable(const int enable);
void setSampStart(const int samp);
void setLoopStart(const int samp);
void setLoopEnd(const int samp);

void setSampEditName(const char* name);

void setUndoBuffer(void);
bool handleUndo(void);
void copySampleRange(const int start, const int end);

bool cutAtCursor(const int index);
bool cutSampleRange(const int start, const int end);

bool cropSampleRange(const int start, const int end);

bool pasteAtCursor(const int index, const bool set_undo);
bool pasteOverRange(const int start, const int end);

bool deleteSingleSample(const int index);
bool deleteRangeSample(const int start, const int end, const bool set_undo);

void setResampleRate(const double rate);
bool handleResample(void);

char *getSampEditName(void);

int16_t **getSampleEditBuffer(void);
_Atomic int *getSampleEditLength(void);
Sample_t *getSampleEdit(void);

void setSampleEditSampleRate(const double rate);
_Atomic double *getSampleEditSampleRate(void);

_Atomic bool *isSampEditLoopEnabled(void);
_Atomic int  *getSampStart(void);
_Atomic int  *getLoopStart(void);
_Atomic int  *getLoopEnd(void);

void resetSampPos(void);
void cleanUpAndFreeSampleEdit(void);

#endif /* __SBC_SAMP_EDIT_H */
