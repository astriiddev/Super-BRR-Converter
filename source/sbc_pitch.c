#include <math.h>

#if defined (_WIN32)
#include <Windows.h>
#define MAX_THREADS 1
#define EXIT_THREAD 0
#define pitch_thread_t DWORD WINAPI
#else
#include <pthread.h>
#define EXIT_THREAD NULL
#define pitch_thread_t void* 
#endif

#include "sbc_utils.h"
#include "sbc_pitch.h"
#include "sbc_gui.h"
#include "sbc_samp_edit.h"

#define SIN(x)  ((x) < 0 ? -1 : 1) 
#define C_FREQ  523.251130601
#define S162FLOAT(x)   ((float) (x) / INT16_MAX)

/*
*   autocorrelation based on:
*       https://medium.com/@jeremygustine/guitar-tuner-pitch-detection-for-dummies-64c4ae27e7ae
*/
static float autocorrelation_lag(const int16_t *samples, const int length, const int lag)
{
    float sum = 0.f;

    assert(samples != NULL);

    for(int i = 0; i <= length - lag - 1; i++)
        sum += (S162FLOAT(samples[i]) * S162FLOAT(samples[i + lag]));

    return sum;
}

static double autocorrelation_samp_rate(const int16_t *samples, const int length)
{
    float init_max = 0, curr_max = 0, last_samp = 0, curr_samp = 0;
    int samp_per_cycle = 0;

    assert(samples != NULL);

    for(int lag = 0; lag < length; lag++)
    {
        curr_samp = autocorrelation_lag(samples, length, lag);

        if(lag == 0)
        {
            init_max = last_samp = curr_samp;
            continue;
        }

        /* get the next peak after the initial peak */
        if(curr_samp > last_samp) curr_max = curr_samp;
        else if(last_samp > curr_samp && curr_max >= ((init_max * 90) / 100))
        {
            SBC_LOG(INITIAL MAX, %f, init_max);
            SBC_LOG(CURRENT MAX, %f, curr_max);

            break;
        }

        samp_per_cycle++;
        last_samp = curr_samp;
    }

    SBC_LOG(SAMPLES PER CYCLE, %d, samp_per_cycle);

    return ((double) samp_per_cycle * C_FREQ);
}

/*
*   autocorrelation doesn't work too well with wavetables, 
*       use zero-crossing detection instead.
*/
static double zerocross_samp_rate(const int16_t *samples, const int length)
{
    int zero_cross = 0, samp_per_cycle = 0;

    assert(samples != NULL);

    if(length > 1024) return autocorrelation_samp_rate(samples, length);

    for(int i = 1; i < length; i++)
    {
        if(SIN(samples[i]) != SIN(samples[i - 1])) zero_cross++;

        if(zero_cross > 1)
        {

            if(i + (samp_per_cycle << 1) >= length) break;
            
            zero_cross = 0;
            samp_per_cycle = 0;
        }

        if(zero_cross) samp_per_cycle++;
    }

    SBC_LOG(SAMPLES PER CYCLE, %d, samp_per_cycle);
    
    return (double) samp_per_cycle * C_FREQ;
}

/*
*   get nearest power of 2, from:
*       https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
*/
static double nearest_2_power(const int val)
{
    unsigned int v = val;

    if (v < 3) return v == 2 ? v : 1;

    v--;

    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    v++;

    return (double) v < 3 ? 1 : (v / 2);
}

static double samp_rate_from_c(const int16_t *samples, const int length)
{
    const double freq    = length > 1024 ? autocorrelation_samp_rate(samples, length) :
                                          zerocross_samp_rate(samples, length);

    const double divider = nearest_2_power((int) floor (freq / (C_FREQ * 8)));

    SBC_LOG(DETECTED FREQ, %lf, freq);

    return (freq / divider);
}

double get_rate_from_loop_len(const int length)
{
    const double freq = (double) length * C_FREQ,
                 divider = nearest_2_power((int) floor (freq / (C_FREQ * 16)));

    assert(length < 129);

    SBC_LOG(DETECTED FREQ, %lf, freq);

    return (freq / divider);
}

pitch_thread_t detect_pitch(void* arg) 
{
    int16_t *samp_buffer = NULL, *samp_edit = (int16_t*) *getSampleEditBuffer();;
    
    const int samp_len = *getSampleEditLength() > 0x2000 ? 0x2000 : *getSampleEditLength();
    const int resample = arg == NULL ? 0 : *(int*) arg;

    double rate = 0;

    if(samp_edit == NULL || samp_len < 2)
    {
        showErrorMsgBox("Threading Error!", "Unable to access sample buffer in pitch detect thread!", NULL);
        return EXIT_THREAD;
    }

    SBC_CALLOC(samp_len, sizeof *samp_buffer, samp_buffer);
    memcpy(samp_buffer, *getSampleEditBuffer(), samp_len * sizeof *samp_buffer);

    rate = round(samp_rate_from_c(samp_buffer, samp_len));
    if(rate < (C_FREQ * 2)) rate = 16744;

    SBC_FREE(samp_buffer);
    
    if(resample)
    {
        setResampleRate(rate);
        handleResampleRateText(rate);
    }
    else
    {
        setSampleEditSampleRate(rate);
        handleSampleRateText(*getSampleEditSampleRate());
    }
    
    repaintGUI();

    SBC_FREE(arg);

    SBC_LOG(ADJUSTED FREQ, %lf, rate);

    return EXIT_THREAD;
}

void detectCenterPitch(const int resample)
{
    const int length = *getSampleEditLength(),
              loop_len = *getLoopEnd() - *getLoopStart();

    int *resample_arg = NULL;

    if(length <= 1) return;
    if(length <= 0x400 && loop_len <= 0x80) 
    {
        const double rate = get_rate_from_loop_len(loop_len);
        
        if(resample)
        {
            setResampleRate(rate);
            handleResampleRateText(rate);
        }
        else
        {
            setSampleEditSampleRate(rate);
            handleSampleRateText(*getSampleEditSampleRate());
        }

        SBC_LOG(ADJUSTED FREQ, %lf, rate);
        return;
    }
    else
    {
#if defined (_WIN32)
        DWORD dwThreadIdArray[1];

		SBC_MALLOC(1, sizeof(int), resample_arg);
		*resample_arg = resample;

        if(CreateThread(NULL, 0, detect_pitch, resample_arg, 0, &dwThreadIdArray[0]) == NULL)
        {
            LPSTR lpMsgBuf = NULL;
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);
    
            showErrorMsgBox("Threading Error!", "Unable to create pitch detect thread!", lpMsgBuf);
        }
#else
        pthread_t pitch_thread;

		SBC_MALLOC(1, sizeof(int), resample_arg);
        *resample_arg = resample;

        errno = 0;
        
        if(pthread_create(&pitch_thread, NULL, detect_pitch, resample_arg) != 0)
        {
            showErrorMsgBox("Threading Error!", "Unable to create pitch detect thread!", strerror(errno));
        }
#endif
    }
}
