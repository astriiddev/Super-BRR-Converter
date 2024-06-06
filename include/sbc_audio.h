#ifndef __SBC_AUDIO_H
#define __SBC_AUDIO_H

#include <stdatomic.h>
#include "sbc_defs.h"

typedef enum
{
	NEAREST = 0,
	LINEAR  = 1,
	CUBIC   = 2,
	GAUSS   = 3
} InterpolationType_t;

typedef struct Interpolation
{
    float tmpL[4], tmpR[4];
	
    InterpolationType_t type;
} Interpolation_t;

void queueAudio(void); 
void pauseAudio(void);
bool *audioQueued(void);

void audioPaused(void);
void playAudio(void);

_Atomic int *getSamplePos(void);

bool initAudio(void);
void closeAudioDevice(void);
void freeAudio(void);

void setPlaybackSampleRate(const double rate);

InterpolationType_t getCurrentInterpolationType(void);
void setInterpolationType(const InterpolationType_t interpol);
void clearInterpolation(Interpolation_t* i);

int getCurrentAudioDriver(void);
void setAudioDriver(const int drv);

int getCurrentOutDev(void);
void setOutputDevice(const int dev);

int getCurrentInDev(void);
void setInputDevice(const int dev);

int getAudioBufferSize(void);
bool setAudioBufferSize(const int size);

int getDeviceSampleRate(void);
bool setDeviceSampleRate(const int rate);

#endif /* __SBC_AUDIO_H */
