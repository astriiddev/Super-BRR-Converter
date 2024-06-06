#include <SDL2/SDL.h>

#include "sbc_utils.h"
#include "sbc_samp_edit.h"
#include "sbc_audio.h"

#define S16TOF32(x)		(float) ((x) > 0 ? ((double) (x) / 32767.) : ((double) (x) / 32768.))
#define   CLAMPF(x)     (x) > 1.f ? 1.f : (x) < -1.f ? -1.f : (x)

static struct Audio_Config_s
{
	SDL_AudioDeviceID output_dev;

	uint8_t num_channels;
	uint16_t buffer_size;
	int outdev_num, indev_num, driver; 

	double sample_rate;

} *audio_config;

static struct Playback_s
{
	_Atomic int pos;
	float vol;

	Interpolation_t interpolation;

	double sample_rate;
	bool is_playing, rampVolDown;
} *playback;

void queueAudio(void) 
{ 
	clearInterpolation(&playback->interpolation);

	playback->pos = *getSampStart();
	resetSampPos();

	playback->is_playing = true; 
	playback->rampVolDown = false; 
	playback->vol = 1.0f; 

	SBC_LOG(AUDIO PLAING, %s, "TRUE");
}

void pauseAudio(void) { SBC_LOG(AUDIO PLAING, %s, "FALSE"); playback->rampVolDown = true; }

bool *audioQueued(void) { return &playback->is_playing; }

void audioPaused(void)
{
	SDL_PauseAudioDevice(audio_config->output_dev, SDL_TRUE);
	SDL_UnlockAudioDevice(audio_config->output_dev);

	if(playback->is_playing)
		playback->is_playing = false;
}

void playAudio(void)
{
	if(audio_config->output_dev == 0) return;

	setPlaybackSampleRate(*getSampleEditSampleRate());

	SDL_PauseAudioDevice(audio_config->output_dev, SDL_FALSE);

	SDL_LockAudioDevice(audio_config->output_dev);

	if(playback->pos > *getSampleEditLength())
	{
		SDL_PauseAudioDevice(audio_config->output_dev, SDL_TRUE);

		playback->is_playing = false;
	}

	SDL_UnlockAudioDevice(audio_config->output_dev);
}

_Atomic int *getSamplePos(void) { return &playback->pos; }

/*
 * S-DSP Gaussian interpolation lookup table translated for 16-bit PCM audio
 * courtesy of 8bitbubsy, adapted from blargg's SPC lookup table
 */
static const double fSpc700Gaussian[4 * 256] =
{
	0.180664, 0.637207, 0.182617, 0.000000, 0.178711, 0.637207, 0.184570, 0.000000,
	0.176758, 0.636719, 0.186035, 0.000000, 0.174805, 0.636719, 0.187988, 0.000000,
	0.172852, 0.636719, 0.189941, 0.000000, 0.171387, 0.636719, 0.191895, 0.000000,
	0.169434, 0.636719, 0.193848, 0.000000, 0.167480, 0.636230, 0.195801, 0.000000,
	0.165527, 0.636230, 0.197754, 0.000000, 0.164062, 0.636230, 0.200195, 0.000000,
	0.162109, 0.635742, 0.202148, 0.000000, 0.160156, 0.635742, 0.204102, 0.000000,
	0.158691, 0.635254, 0.206055, 0.000000, 0.156738, 0.634766, 0.208008, 0.000000,
	0.155273, 0.634766, 0.209961, 0.000000, 0.153320, 0.634277, 0.211914, 0.000000,
	0.151855, 0.633789, 0.214355, 0.000488, 0.149902, 0.633301, 0.216309, 0.000488,
	0.148438, 0.633301, 0.218262, 0.000488, 0.146484, 0.632812, 0.220215, 0.000488,
	0.145020, 0.632324, 0.222656, 0.000488, 0.143066, 0.631836, 0.224609, 0.000488,
	0.141602, 0.631348, 0.226562, 0.000488, 0.139648, 0.630859, 0.229004, 0.000488,
	0.138184, 0.630371, 0.230957, 0.000488, 0.136719, 0.629883, 0.232910, 0.000488,
	0.134766, 0.628906, 0.235352, 0.000488, 0.133301, 0.628418, 0.237305, 0.000977,
	0.131836, 0.627930, 0.239746, 0.000977, 0.130371, 0.626953, 0.241699, 0.000977,
	0.128418, 0.626465, 0.243652, 0.000977, 0.126953, 0.625977, 0.246094, 0.000977,
	0.125488, 0.625000, 0.248047, 0.000977, 0.124023, 0.624512, 0.250488, 0.000977,
	0.122559, 0.623535, 0.252441, 0.001465, 0.121094, 0.622559, 0.254883, 0.001465,
	0.119629, 0.622070, 0.257324, 0.001465, 0.118164, 0.621094, 0.259277, 0.001465,
	0.116699, 0.620117, 0.261719, 0.001465, 0.115234, 0.619629, 0.263672, 0.001953,
	0.113770, 0.618652, 0.266113, 0.001953, 0.112305, 0.617676, 0.268555, 0.001953,
	0.110840, 0.616699, 0.270508, 0.001953, 0.109375, 0.615723, 0.272949, 0.001953,
	0.107910, 0.614746, 0.274902, 0.002441, 0.106445, 0.613770, 0.277344, 0.002441,
	0.104980, 0.612793, 0.279785, 0.002441, 0.103516, 0.611816, 0.281738, 0.002441,
	0.102539, 0.610840, 0.284180, 0.002930, 0.101074, 0.609375, 0.286621, 0.002930,
	0.099609, 0.608398, 0.289062, 0.002930, 0.098145, 0.607422, 0.291016, 0.002930,
	0.097168, 0.605957, 0.293457, 0.003418, 0.095703, 0.604980, 0.295898, 0.003418,
	0.094238, 0.604004, 0.298340, 0.003418, 0.093262, 0.602539, 0.300293, 0.003906,
	0.091797, 0.601562, 0.302734, 0.003906, 0.090820, 0.600098, 0.305176, 0.003906,
	0.089355, 0.599121, 0.307617, 0.004395, 0.087891, 0.597656, 0.310059, 0.004395,
	0.086914, 0.596191, 0.312500, 0.004395, 0.085449, 0.595215, 0.314453, 0.004883,
	0.084473, 0.593750, 0.316895, 0.004883, 0.083496, 0.592285, 0.319336, 0.004883,
	0.082031, 0.590820, 0.321777, 0.005371, 0.081055, 0.589355, 0.324219, 0.005371,
	0.079590, 0.588379, 0.326660, 0.005371, 0.078613, 0.586914, 0.329102, 0.005859,
	0.077637, 0.585449, 0.331055, 0.005859, 0.076172, 0.583984, 0.333496, 0.006348,
	0.075195, 0.582520, 0.335938, 0.006348, 0.074219, 0.581055, 0.338379, 0.006836,
	0.073242, 0.579102, 0.340820, 0.006836, 0.071777, 0.577637, 0.343262, 0.007324,
	0.070801, 0.576172, 0.345703, 0.007324, 0.069824, 0.574707, 0.348145, 0.007324,
	0.068848, 0.573242, 0.350586, 0.007812, 0.067871, 0.571289, 0.353027, 0.007812,
	0.066895, 0.569824, 0.355469, 0.008301, 0.065430, 0.568359, 0.357422, 0.008301,
	0.064453, 0.566406, 0.359863, 0.008789, 0.063477, 0.564941, 0.362305, 0.009277,
	0.062500, 0.562988, 0.364746, 0.009277, 0.061523, 0.561523, 0.367188, 0.009766,
	0.060547, 0.559570, 0.369629, 0.009766, 0.059570, 0.558105, 0.372070, 0.010254,
	0.058594, 0.556152, 0.374512, 0.010254, 0.057617, 0.554688, 0.376953, 0.010742,
	0.057129, 0.552734, 0.379395, 0.011230, 0.056152, 0.550781, 0.381836, 0.011230,
	0.055176, 0.549316, 0.384277, 0.011719, 0.054199, 0.547363, 0.386719, 0.011719,
	0.053223, 0.545410, 0.389160, 0.012207, 0.052246, 0.543457, 0.391602, 0.012695,
	0.051758, 0.541504, 0.393555, 0.013184, 0.050781, 0.540039, 0.395996, 0.013184,
	0.049805, 0.538086, 0.398438, 0.013672, 0.048828, 0.536133, 0.400879, 0.014160,
	0.048340, 0.534180, 0.403320, 0.014160, 0.047363, 0.532227, 0.405762, 0.014648,
	0.046387, 0.530273, 0.408203, 0.015137, 0.045898, 0.528320, 0.410645, 0.015625,
	0.044922, 0.526367, 0.413086, 0.015625, 0.043945, 0.524414, 0.415527, 0.016113,
	0.043457, 0.522461, 0.417480, 0.016602, 0.042480, 0.520508, 0.419922, 0.017090,
	0.041992, 0.518066, 0.422363, 0.017578, 0.041016, 0.516113, 0.424805, 0.017578,
	0.040527, 0.514160, 0.427246, 0.018066, 0.039551, 0.512207, 0.429688, 0.018555,
	0.039062, 0.510254, 0.431641, 0.019043, 0.038086, 0.507812, 0.434082, 0.019531,
	0.037598, 0.505859, 0.436523, 0.020020, 0.037109, 0.503906, 0.438965, 0.020508,
	0.036133, 0.501465, 0.441406, 0.020996, 0.035645, 0.499512, 0.443359, 0.021484,
	0.034668, 0.497559, 0.445801, 0.021973, 0.034180, 0.495117, 0.448242, 0.022461,
	0.033691, 0.493164, 0.450684, 0.022949, 0.032715, 0.490723, 0.452637, 0.023438,
	0.032227, 0.488770, 0.455078, 0.023926, 0.031738, 0.486816, 0.457520, 0.024414,
	0.031250, 0.484375, 0.459473, 0.024902, 0.030273, 0.482422, 0.461914, 0.025391,
	0.029785, 0.479980, 0.464355, 0.025879, 0.029297, 0.477539, 0.466309, 0.026367,
	0.028809, 0.475586, 0.468750, 0.026855, 0.028320, 0.473145, 0.471191, 0.027344,
	0.027344, 0.471191, 0.473145, 0.028320, 0.026855, 0.468750, 0.475586, 0.028809,
	0.026367, 0.466309, 0.477539, 0.029297, 0.025879, 0.464355, 0.479980, 0.029785,
	0.025391, 0.461914, 0.482422, 0.030273, 0.024902, 0.459473, 0.484375, 0.031250,
	0.024414, 0.457520, 0.486816, 0.031738, 0.023926, 0.455078, 0.488770, 0.032227,
	0.023438, 0.452637, 0.490723, 0.032715, 0.022949, 0.450684, 0.493164, 0.033691,
	0.022461, 0.448242, 0.495117, 0.034180, 0.021973, 0.445801, 0.497559, 0.034668,
	0.021484, 0.443359, 0.499512, 0.035645, 0.020996, 0.441406, 0.501465, 0.036133,
	0.020508, 0.438965, 0.503906, 0.037109, 0.020020, 0.436523, 0.505859, 0.037598,
	0.019531, 0.434082, 0.507812, 0.038086, 0.019043, 0.431641, 0.510254, 0.039062,
	0.018555, 0.429688, 0.512207, 0.039551, 0.018066, 0.427246, 0.514160, 0.040527,
	0.017578, 0.424805, 0.516113, 0.041016, 0.017578, 0.422363, 0.518066, 0.041992,
	0.017090, 0.419922, 0.520508, 0.042480, 0.016602, 0.417480, 0.522461, 0.043457,
	0.016113, 0.415527, 0.524414, 0.043945, 0.015625, 0.413086, 0.526367, 0.044922,
	0.015625, 0.410645, 0.528320, 0.045898, 0.015137, 0.408203, 0.530273, 0.046387,
	0.014648, 0.405762, 0.532227, 0.047363, 0.014160, 0.403320, 0.534180, 0.048340,
	0.014160, 0.400879, 0.536133, 0.048828, 0.013672, 0.398438, 0.538086, 0.049805,
	0.013184, 0.395996, 0.540039, 0.050781, 0.013184, 0.393555, 0.541504, 0.051758,
	0.012695, 0.391602, 0.543457, 0.052246, 0.012207, 0.389160, 0.545410, 0.053223,
	0.011719, 0.386719, 0.547363, 0.054199, 0.011719, 0.384277, 0.549316, 0.055176,
	0.011230, 0.381836, 0.550781, 0.056152, 0.011230, 0.379395, 0.552734, 0.057129,
	0.010742, 0.376953, 0.554688, 0.057617, 0.010254, 0.374512, 0.556152, 0.058594,
	0.010254, 0.372070, 0.558105, 0.059570, 0.009766, 0.369629, 0.559570, 0.060547,
	0.009766, 0.367188, 0.561523, 0.061523, 0.009277, 0.364746, 0.562988, 0.062500,
	0.009277, 0.362305, 0.564941, 0.063477, 0.008789, 0.359863, 0.566406, 0.064453,
	0.008301, 0.357422, 0.568359, 0.065430, 0.008301, 0.355469, 0.569824, 0.066895,
	0.007812, 0.353027, 0.571289, 0.067871, 0.007812, 0.350586, 0.573242, 0.068848,
	0.007324, 0.348145, 0.574707, 0.069824, 0.007324, 0.345703, 0.576172, 0.070801,
	0.007324, 0.343262, 0.577637, 0.071777, 0.006836, 0.340820, 0.579102, 0.073242,
	0.006836, 0.338379, 0.581055, 0.074219, 0.006348, 0.335938, 0.582520, 0.075195,
	0.006348, 0.333496, 0.583984, 0.076172, 0.005859, 0.331055, 0.585449, 0.077637,
	0.005859, 0.329102, 0.586914, 0.078613, 0.005371, 0.326660, 0.588379, 0.079590,
	0.005371, 0.324219, 0.589355, 0.081055, 0.005371, 0.321777, 0.590820, 0.082031,
	0.004883, 0.319336, 0.592285, 0.083496, 0.004883, 0.316895, 0.593750, 0.084473,
	0.004883, 0.314453, 0.595215, 0.085449, 0.004395, 0.312500, 0.596191, 0.086914,
	0.004395, 0.310059, 0.597656, 0.087891, 0.004395, 0.307617, 0.599121, 0.089355,
	0.003906, 0.305176, 0.600098, 0.090820, 0.003906, 0.302734, 0.601562, 0.091797,
	0.003906, 0.300293, 0.602539, 0.093262, 0.003418, 0.298340, 0.604004, 0.094238,
	0.003418, 0.295898, 0.604980, 0.095703, 0.003418, 0.293457, 0.605957, 0.097168,
	0.002930, 0.291016, 0.607422, 0.098145, 0.002930, 0.289062, 0.608398, 0.099609,
	0.002930, 0.286621, 0.609375, 0.101074, 0.002930, 0.284180, 0.610840, 0.102539,
	0.002441, 0.281738, 0.611816, 0.103516, 0.002441, 0.279785, 0.612793, 0.104980,
	0.002441, 0.277344, 0.613770, 0.106445, 0.002441, 0.274902, 0.614746, 0.107910,
	0.001953, 0.272949, 0.615723, 0.109375, 0.001953, 0.270508, 0.616699, 0.110840,
	0.001953, 0.268555, 0.617676, 0.112305, 0.001953, 0.266113, 0.618652, 0.113770,
	0.001953, 0.263672, 0.619629, 0.115234, 0.001465, 0.261719, 0.620117, 0.116699,
	0.001465, 0.259277, 0.621094, 0.118164, 0.001465, 0.257324, 0.622070, 0.119629,
	0.001465, 0.254883, 0.622559, 0.121094, 0.001465, 0.252441, 0.623535, 0.122559,
	0.000977, 0.250488, 0.624512, 0.124023, 0.000977, 0.248047, 0.625000, 0.125488,
	0.000977, 0.246094, 0.625977, 0.126953, 0.000977, 0.243652, 0.626465, 0.128418,
	0.000977, 0.241699, 0.626953, 0.130371, 0.000977, 0.239746, 0.627930, 0.131836,
	0.000977, 0.237305, 0.628418, 0.133301, 0.000488, 0.235352, 0.628906, 0.134766,
	0.000488, 0.232910, 0.629883, 0.136719, 0.000488, 0.230957, 0.630371, 0.138184,
	0.000488, 0.229004, 0.630859, 0.139648, 0.000488, 0.226562, 0.631348, 0.141602,
	0.000488, 0.224609, 0.631836, 0.143066, 0.000488, 0.222656, 0.632324, 0.145020,
	0.000488, 0.220215, 0.632812, 0.146484, 0.000488, 0.218262, 0.633301, 0.148438,
	0.000488, 0.216309, 0.633301, 0.149902, 0.000488, 0.214355, 0.633789, 0.151855,
	0.000000, 0.211914, 0.634277, 0.153320, 0.000000, 0.209961, 0.634766, 0.155273,
	0.000000, 0.208008, 0.634766, 0.156738, 0.000000, 0.206055, 0.635254, 0.158691,
	0.000000, 0.204102, 0.635742, 0.160156, 0.000000, 0.202148, 0.635742, 0.162109,
	0.000000, 0.200195, 0.636230, 0.164062, 0.000000, 0.197754, 0.636230, 0.165527,
	0.000000, 0.195801, 0.636230, 0.167480, 0.000000, 0.193848, 0.636719, 0.169434,
	0.000000, 0.191895, 0.636719, 0.171387, 0.000000, 0.189941, 0.636719, 0.172852,
	0.000000, 0.187988, 0.636719, 0.174805, 0.000000, 0.186035, 0.636719, 0.176758,
	0.000000, 0.184570, 0.637207, 0.178711, 0.000000, 0.182617, 0.637207, 0.180664
};

/* 
*  Input samples are 16-bit signed PCM.
*  offset is the fractional sampling position (0.0 .. 0.999inf).
*  Output is -32768.0 .. 32767.0f.
*/
static inline double getGaussianInterpolation(const double s0, const double s1, const double s2, const double s3, const double offset)
{
	const int32_t frac256 = (int32_t) (offset * 256.0f);
	const double *t = &fSpc700Gaussian[frac256 << 2];

	return ((s0 * t[0]) + (s1 * t[1]) + (s2 * t[2]) + (s3 * t[3]));
}

static inline double getCubicInterpolation(const double s0, const double s1, const double s2, const double s3, const double offset)
{
	const double a = (3. * (s1 - s2) - s0 + s3) / 2.,
				 b = 2. * s2 + s0 - (5. * s1 + s3) / 2.,
				 c = (s2 - s0) / 2.;

	return (((a * offset) + b) * offset + c) * offset + s1;
}

static inline double getLinearInterpolations(const double s2, const double s3, const double offset)
{
	return s2 + (offset * (s3 - s2));
}

static void applySampleInterpolation(const Interpolation_t *i, double *outL, double *outR, const double offset)
{
	switch(i->type)
	{
		case NEAREST:

			*outL = i->tmpL[3];
			*outR = i->tmpL[3];

			break;

		case LINEAR:

			*outL = getLinearInterpolations(i->tmpL[2], i->tmpL[3], offset);
			*outR = getLinearInterpolations(i->tmpR[2], i->tmpR[3], offset);

			break;

		case CUBIC:

			*outL = getCubicInterpolation(i->tmpL[0], i->tmpL[1], i->tmpL[2], i->tmpL[3], offset);
			*outR = getCubicInterpolation(i->tmpR[0], i->tmpR[1], i->tmpR[2], i->tmpR[3], offset);

			break;

		case GAUSS:
                        
			*outL = getGaussianInterpolation(i->tmpL[0], i->tmpL[1], i->tmpL[2], i->tmpL[3], offset);
			*outR = getGaussianInterpolation(i->tmpR[0], i->tmpR[1], i->tmpR[2], i->tmpR[3], offset);

			break;
	}
}

static void shiftFilterCoeff(Interpolation_t *i, const float in_samp)
{
	i->tmpL[0] = i->tmpL[1];
	i->tmpL[1] = i->tmpL[2];
	i->tmpL[2] = i->tmpL[3];
	i->tmpL[3] = in_samp;
	
	i->tmpR[0] = i->tmpR[1];
	i->tmpR[1] = i->tmpR[2];
	i->tmpR[2] = i->tmpR[3];
	i->tmpR[3] = in_samp;
}

static void incrementSample(Interpolation_t* i, Sample_t* s, float *bufL, float *bufR, const double sampleRate)
{
	const int pos = (int) floor(s->pos);

	const double deltaRate = sampleRate / (audio_config->sample_rate);
	const double offset = s->pos - (double) pos;

	double outL = 0.0, outR = 0.0;

	applySampleInterpolation(i, &outL, &outR, offset);

	s->pos += deltaRate;

	if((int) floor(s->pos) > s->audio.length)
	{
		*bufL = *bufR = playback->is_playing = 0;
		return;
	}

	if((int) floor(s->pos) > pos) shiftFilterCoeff(i, (float) s->audio.buffer[pos]);

	if(s->is_looped && s->pos > (double) s->loop_end)
		s->pos = (double) s->loop_start;

	*bufL = S16TOF32(outL);
	*bufR = S16TOF32(outR);
}

static void SDLCALL audioCallback(void *data, uint8_t *stream, int len)
{
	float *out = (float*) stream;

	const int BYTES_PER_SAMPLE = audio_config->num_channels * sizeof *out;
	int numSamples = len / BYTES_PER_SAMPLE;

	(void) data;

	while(--numSamples >= 0)
	{
		float sampL = 0.f, sampR = 0.f;

		incrementSample(&playback->interpolation, getSampleEdit(), &sampL, &sampR, playback->sample_rate);

		*out++ = CLAMPF(sampL * playback->vol);
		*out++ = CLAMPF(sampR * playback->vol);

		if(playback->rampVolDown) playback->vol *= 0.999f;
	}

	playback->pos = (int) floor(getSampleEdit()->pos);
	if(playback->vol <= 0.001f) playback->is_playing = false;
}

static bool outputConfigChanged(void)
{
	SDL_AudioSpec want, have;
	const char* outdev_name;
	
	closeAudioDevice();
	resetSampPos();

	memset(&want, 0, sizeof(want));
	want.freq = (int) audio_config->sample_rate;
	want.format = AUDIO_F32SYS;
	want.channels = audio_config->num_channels;
	want.callback = audioCallback;
	want.samples = audio_config->buffer_size;

	outdev_name = SDL_GetAudioDeviceName(audio_config->outdev_num, 0);
	printf("\033[0;32mOpening output audio device %s\033[0m\n\n", outdev_name);

	if((audio_config->output_dev = SDL_OpenAudioDevice(outdev_name, 0, &want, &have, ~SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == 0)
		return false;

	if(have.freq != (int) audio_config->sample_rate)
	{
		printf("Audio sample rate not supported!\n");

		return false;
	}
	
	if(have.format != AUDIO_F32SYS)
	{
		printf("Audio format not supported!\n");

		return false;
	}

	return true;
}

static int checkDevice(const int want, const int max) { return want < 0 ? 0 : want >= max ? max - 1 : want; }

bool initAudio(void)
{
	const int num_drivers = SDL_GetNumAudioDrivers();
	int backup_driver = 0;

	SBC_CALLOC(1, sizeof(struct Audio_Config_s), audio_config);
	SBC_CALLOC(1, sizeof(struct Playback_s), playback);
	
	audio_config->num_channels = 2;
	audio_config->sample_rate = 48000.0;
	audio_config->buffer_size = 1024;
	audio_config->outdev_num = checkDevice(0, SDL_GetNumAudioDevices(0));
	audio_config->indev_num  = checkDevice(0, SDL_GetNumAudioDevices(1));

	playback->interpolation.type = GAUSS;

	clearInterpolation(&playback->interpolation);

	playback->vol = 1.0f;
	playback->sample_rate = 16744.0;

	printf("\033[0;34mAvailable audio drivers:\033[0m\n");

	for(int i = 0; i < num_drivers; i++)
	{
#ifdef _WIN32
		if(_strcasestr("wasapi", SDL_GetAudioDriver(i)))
			audio_config->driver = i;
		else if(_strcasestr("directsound", SDL_GetAudioDriver(i)))
			backup_driver = i;
#elif defined __linux__
		if(_strcasestr("pulseaudio", SDL_GetAudioDriver(i)))
			audio_config->driver = i;
		else if(_strcasestr("alsa", SDL_GetAudioDriver(i)))
			backup_driver = i;

		printf("\t%02d)  %s\n", i + 1, SDL_GetAudioDriver(i));
#endif
	}
	
	printf("\033[0;34mAttempting to use %s driver...\033[0m\n", SDL_GetAudioDriver(audio_config->driver));

	if(SDL_AudioInit(SDL_GetAudioDriver(audio_config->driver)) == 0)
		printf("\033[0;32mUsing audio driver: %s\033[0m\n", SDL_GetCurrentAudioDriver());
	else
	{
		printf("\031[0;32mAudio driver %s failed!\n\033[0m\n", SDL_GetAudioDriver(audio_config->driver));

		audio_config->driver = backup_driver;

		printf("\033[0;34mAttempting to use %s driver...\033[0m\n", SDL_GetAudioDriver(audio_config->driver));
		if(SDL_AudioInit(SDL_GetAudioDriver(audio_config->driver)) == 0)
			printf("\033[0;32mUsing audio driver: %s\033[0m\n", SDL_GetCurrentAudioDriver());
	}

	return outputConfigChanged();
}

void closeAudioDevice(void)
{
	if(playback == NULL) return;

	playback->is_playing = false;
	audioPaused();

	if(audio_config->output_dev > 0)
	{
		printf("\033[0;34mClosing audio device %s...\033[0m\n", SDL_GetAudioDeviceName(audio_config->outdev_num, 0));

		SDL_PauseAudioDevice(audio_config->output_dev, SDL_TRUE);
		SDL_CloseAudioDevice(audio_config->output_dev);
		audio_config->output_dev = 0;
	}
}

void freeAudio(void)
{
	closeAudioDevice();

	SBC_FREE(playback);
	SBC_FREE(audio_config);
}

InterpolationType_t getCurrentInterpolationType(void) { return playback->interpolation.type; }

void setInterpolationType(const InterpolationType_t interpol)
{
	if(playback->interpolation.type == interpol) return;
	playback->interpolation.type = interpol;
}

void clearInterpolation(Interpolation_t* i)
{
	memset(i, 0, sizeof *i - sizeof(i->type));
}

void setPlaybackSampleRate(const double rate) { playback->sample_rate = rate; }

int getCurrentAudioDriver(void) { return audio_config->driver; }

int getCurrentOutDev(void) { return audio_config->outdev_num; }

void setAudioDriver(const int drv)
{
	const int curr_drv = audio_config->driver;
	int num_dev = -1;

	if(drv == curr_drv) return;

	closeAudioDevice();

	audio_config->driver = checkDevice(drv, SDL_GetNumAudioDrivers());

	printf("\033[0;34mAttempting to use %s driver...\033[0m\n", SDL_GetAudioDriver(audio_config->driver));

	if(SDL_AudioInit(SDL_GetAudioDriver(audio_config->driver)) == 0)
		printf("\033[0;32mUsing audio driver: %s\033[0m\n", SDL_GetCurrentAudioDriver());
	else 
	{
		showErrorMsgBox("Audio Config Error", "Invalid Audio Driver!", SDL_GetError());
		printf("\031[0;32mAudio driver %s failed!\n\033[0m\n", SDL_GetAudioDriver(audio_config->driver));
		setAudioDriver(curr_drv);
	}

	if(audio_config->outdev_num >= (num_dev = SDL_GetNumAudioDevices(0)))
		audio_config->outdev_num = num_dev - 1;

	audio_config->outdev_num = audio_config->indev_num = 0;
	outputConfigChanged();
}

void setOutputDevice(const int dev)
{
	const int curr_dev = audio_config->outdev_num;
	if(dev == curr_dev) return;
	
	audio_config->outdev_num = checkDevice(dev, SDL_GetNumAudioDevices(0));
	
	SBC_LOG(OUTPUT DEVICE NUMBER, %d, audio_config->outdev_num);
	SBC_LOG(OUTPUT DEVICE NAME, %s, SDL_GetAudioDeviceName(audio_config->outdev_num, 0));
	
	if(!outputConfigChanged())
	{
		audio_config->outdev_num = curr_dev;
		showErrorMsgBox("Audio Config Error", "Invalid Device!", SDL_GetError());
		
		if(!outputConfigChanged())
		{
			showErrorMsgBox("Audio Config Error", "Cannot restore configuration!", SDL_GetError());
			closeAudioDevice();
		}
	}
}

int getCurrentInDev(void) { return audio_config->indev_num; }

void setInputDevice(const int dev)
{
	if(dev == audio_config->indev_num) return;

	audio_config->indev_num = checkDevice(dev, SDL_GetNumAudioDevices(1));
}

int getAudioBufferSize(void) 
{ 
	switch(audio_config->buffer_size)
	{
		case  256: return 0;
		case  512: return 1;
		case 1024: return 2;
		case 2048: return 3;

		default: 
			audio_config->buffer_size = 1024;
			return 2;
	}
}

bool setAudioBufferSize(const int size)
{
	const uint16_t curr_size = audio_config->buffer_size;

	switch (size)
	{
	case 0:
		if(curr_size == 256) return false;
		audio_config->buffer_size = 256;
		break;
	case 1:
		if(curr_size == 512) return false;
		audio_config->buffer_size = 512;
		break;
	case 2:
		if(curr_size == 1024) return false;
		audio_config->buffer_size = 1024;
		break;
	case 3:
		if(curr_size == 2048) return false;
		audio_config->buffer_size = 2048;
		break;
	
	default:
		if(curr_size == 1024) return false;
		audio_config->buffer_size = 1024;
		break;
	}
	
	if(!outputConfigChanged())
	{
		audio_config->buffer_size = curr_size;
		showErrorMsgBox("Audio Config Error", "Invalid Buffer Size!", SDL_GetError());
		
		if(!outputConfigChanged())
		{
			showErrorMsgBox("Audio Config Error", "Cannot restore configuration!", SDL_GetError());
			closeAudioDevice();
		}

		return false;
	}

	return true;
}

int getDeviceSampleRate(void) 
{ 
	const int sample_rate = (int) audio_config->sample_rate;

	switch(sample_rate)
	{
		case 32000: return 0;
		case 44100: return 1;
		case 48000: return 2;
		case 96000: return 3;

		default: 
			audio_config->sample_rate = 48000.0; 
			return 2;
	}
}

bool setDeviceSampleRate(const int rate)
{
	const int curr_rate = (int) audio_config->sample_rate;

	SBC_LOG(SAMPLE RATE SELECT, %d, rate);
	switch (rate)
	{
	case 0:
		if(curr_rate == 32000) return false;
		audio_config->sample_rate = 32000.0;
		break;
	case 1:
		if(curr_rate == 44100) return false;
		audio_config->sample_rate = 44100.0;
		break;
	case 2:
		if(curr_rate == 48000) return false;
		audio_config->sample_rate = 48000.0;
		break;
	case 3:
		if(curr_rate == 96000) return false;
		audio_config->sample_rate = 96000.0;
		break;
	
	default:
		if(curr_rate == 48000) return false;
		audio_config->sample_rate = 48000.0;
		break;
	}

	SBC_LOG(DEVICE SAMPLE RATE, %ld, audio_config->sample_rate);
	
	if(!outputConfigChanged())
	{
		audio_config->sample_rate = (double) curr_rate;
		showErrorMsgBox("Audio Config Error", "Invalid Sample Rate!", SDL_GetError());
		
		if(!outputConfigChanged())
		{
			showErrorMsgBox("Audio Config Error", "Cannot restore configuration!", SDL_GetError());
			closeAudioDevice();
		}

		return false;
	}

	return true;
}
