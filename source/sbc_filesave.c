#include <stdlib.h>
#include <math.h>

#include "sbc_utils.h"
#include "sbc_samp_edit.h"
#include "sbc_filesave.h"

#define BE16(a)     (uint16_t) (((a) & 0xFF00) >>  8 | ((a) & 0x00FF) << 8 ) 

#define BE32(a)     (uint32_t) (((a) & 0xFF000000) >> 24 | ((a) & 0x00FF0000) >>  8 | \
                                ((a) & 0x0000FF00) <<  8 | ((a) & 0x000000FF) <<  24) 

/* 
*   converts number of samples to number of bytes as per BRR's 16 sample to 9 byte ratio 
*   equivalent to (int) round(9.0 * (double) in / 16) 
*/
#define BRRPOS2BYTEPOS(x)       ((int) (((9 * (((int64_t) (x) << 16) / 16)) + 0x8000) >> 16))

static uint32_t get_chunk_id(const char *chunk_name)
{
    assert(strlen(chunk_name) == 4);
    return *(uint32_t*) chunk_name;
}

static bool write_wav_hdr(FILE *out_file, const Sample_t *samp)
{
	/* RIFF WAVE format header for writing WAV files */

#pragma pack(push,1)
    struct Wav_Header_u
    {
        uint32_t riff_id;
        uint32_t riff_size;
        uint32_t wave_id;

        struct fmt__Chunk_s
        {
            uint32_t fmt__id;
            uint32_t fmt__size;
            uint16_t comp_type;
            uint16_t num_chan;
            uint32_t samp_per_sec;
            uint32_t byte_per_sec;
            uint16_t block_align;
            uint16_t bits_per_samp;
        } fmt_;

        struct data_Chunk_s
        {
            uint32_t data_id;
            uint32_t data_size;
        } data;
    } wav_hdr;
#pragma pack(pop)

	const uint16_t bit_depth = exporting16bit() ? 16 : 8;
    const uint32_t samp_len = (uint32_t) samp->audio.length,
                   data_size = samp_len * (bit_depth / 8);

    const size_t hdr_numb = sizeof(struct Wav_Header_u);

    wav_hdr.riff_id = get_chunk_id("RIFF");
    wav_hdr.riff_size = data_size + (uint32_t) (hdr_numb - 8);
    wav_hdr.wave_id = get_chunk_id("WAVE");

    wav_hdr.fmt_.fmt__id = get_chunk_id("fmt ");
    wav_hdr.fmt_.fmt__size = (uint32_t) (sizeof(struct fmt__Chunk_s) - 8);
    wav_hdr.fmt_.comp_type = 1;
    wav_hdr.fmt_.num_chan = 1;
    wav_hdr.fmt_.samp_per_sec = (uint32_t) floor(samp->rate);
    wav_hdr.fmt_.byte_per_sec = (wav_hdr.fmt_.samp_per_sec * wav_hdr.fmt_.num_chan * bit_depth) / 8;
    wav_hdr.fmt_.block_align  = (wav_hdr.fmt_.num_chan * bit_depth) / 8;
    wav_hdr.fmt_.bits_per_samp  = bit_depth;

    wav_hdr.data.data_id = get_chunk_id("data");
    wav_hdr.data.data_size = data_size; 

    if(samp->is_looped) wav_hdr.riff_size += 68;   // sizeof(Smpl_Header_s)

    // tests for num bytes written, instead of num elements
    return (fwrite(&wav_hdr, 1, hdr_numb, out_file) == hdr_numb);
}

static bool write_smpl_hdr(FILE *out_file, const Sample_t *samp)
{
	/* smpl header for writing WAV file loop */

#pragma pack(push,1)
    struct Smpl_Header_s
    {
        uint32_t smpl_id;
        uint32_t smpl_size;

        uint32_t manufacturer;
        uint32_t product;
        uint32_t period;
        
        uint32_t midi_note;
        uint32_t midi_fraction;

        uint32_t SMPTE_format;
        uint32_t SMPTE_offset;
        
        uint32_t num_loops;
        uint32_t smpl_data;
        uint32_t padding0;
        uint32_t padding1;
        
        uint32_t loop_start;
        uint32_t loop_end;
        
        uint32_t padding2;
        uint32_t padding3;
    } smpl_hdr;
#pragma pack(pop)

    const size_t hdr_numb = sizeof(struct Smpl_Header_s);
    const uint32_t samp_len = (uint32_t) samp->audio.length;

    memset(&smpl_hdr, 0, hdr_numb);  // barely any of this chunk's data is used, but still needs to be included

    smpl_hdr.smpl_id   = get_chunk_id("smpl");
    smpl_hdr.smpl_size = (uint32_t) (hdr_numb - 8);
    smpl_hdr.midi_note = 60;
    smpl_hdr.num_loops = 1;

    smpl_hdr.loop_start = samp->loop_start >= (int) samp->audio.length ? 0 : (uint32_t) samp->loop_start;
    smpl_hdr.loop_end   = samp->loop_end >= (int) samp_len ? samp_len - 1 : (uint32_t) samp->loop_end;

    // tests for num bytes written, instead of num elements
    return (fwrite(&smpl_hdr, 1, hdr_numb, out_file) == hdr_numb);
}

static bool save_wav(FILE *out_file, Sample_t *samp)
{
    assert(samp != NULL);
    assert(samp->audio.buffer != NULL);
    assert(samp->audio.length > 1);

    if(!write_wav_hdr(out_file, samp)) return false;

    if(exporting16bit())
    {
        for(int i = 0; i < samp->audio.length; i++)
        {
            if(fwrite(&samp->audio.buffer[i], 1, 2, out_file) != 2) return false;
        }
    }
    else
    {
        for(int i = 0; i < samp->audio.length; i++)
        {
            uint8_t temp_samp = (samp->audio.buffer[i] >> 8) ^ 0x80;
            if(fwrite(&temp_samp, 1, 1, out_file) != 1) return false;
        }
    }

    if(*isSampEditLoopEnabled()) 
        if(!write_smpl_hdr(out_file, samp)) return false;
    
    return true;
}

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#define FloatToUnsigned(f) ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

void ConvertToIeeeExtended(double num, uint8_t (*bytes)[10])
{
    int sign, expon;
    double fMant, fsMant;
    unsigned long hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else {    /* Finite */
            expon += 16382;
            if (expon < 0) {    /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);          
            fsMant = floor(fMant); 
            hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32); 
            fsMant = floor(fMant); 
            loMant = FloatToUnsigned(fsMant);
        }
    }
    
    (*bytes)[0] = (uint8_t) (expon >> 8);
    (*bytes)[1] = (uint8_t) (expon);
    (*bytes)[2] = (uint8_t) (hiMant >> 24);
    (*bytes)[3] = (uint8_t) (hiMant >> 16);
    (*bytes)[4] = (uint8_t) (hiMant >> 8);
    (*bytes)[5] = (uint8_t) (hiMant);
    (*bytes)[6] = (uint8_t) (loMant >> 24);
    (*bytes)[7] = (uint8_t) (loMant >> 16);
    (*bytes)[8] = (uint8_t) (loMant >> 8);
    (*bytes)[9] = (uint8_t) (loMant);
}

static bool write_aif_hdr(FILE *out_file, const Sample_t *samp)
{
	/* FORM AIFF format header for writing AIF files */

#pragma pack(push,1)
    struct Aif_Header_s
    {
        uint32_t form_id;
        uint32_t form_size;
        uint32_t aiff_id;

        struct COMM_Chunk_s
        {
            uint32_t comm_id;
            uint32_t comm_size;
            uint16_t num_chan;
            uint32_t num_samp_frames;
            uint16_t samp_size;

            uint8_t samp_rate[10];
        } comm;

        struct SSND_Chunk_s
        {
            uint32_t ssnd_id;
            uint32_t ssnd_size;
            uint32_t offset;
            uint32_t block_size;
        } ssnd;
        
    } aif_hdr;
#pragma pack(pop)

    const uint16_t bit_depth = exporting16bit() ? 16  : 8, num_chan = 1;
    const size_t hdr_numb = sizeof(struct Aif_Header_s);
    const uint32_t samp_len = (uint32_t) samp->audio.length,
                   data_len = samp_len * (bit_depth / 8);

    aif_hdr.form_id = get_chunk_id("FORM");
    aif_hdr.form_size = BE32(data_len + (uint32_t) (hdr_numb - 8));
    aif_hdr.aiff_id = get_chunk_id("AIFF");

    aif_hdr.comm.comm_id = get_chunk_id("COMM");
    aif_hdr.comm.comm_size = BE32((uint32_t) (sizeof(struct COMM_Chunk_s) - 8));
    aif_hdr.comm.num_chan = BE16(num_chan);
    aif_hdr.comm.num_samp_frames = BE32(samp_len * (uint32_t) num_chan);
    aif_hdr.comm.samp_size = BE16(bit_depth);

    ConvertToIeeeExtended(samp->rate, &aif_hdr.comm.samp_rate);

    aif_hdr.ssnd.ssnd_id = get_chunk_id("SSND");
    aif_hdr.ssnd.ssnd_size = BE32(data_len);
    aif_hdr.ssnd.offset = aif_hdr.ssnd.block_size = 0;

    // tests for num bytes written, instead of num elements
    return (fwrite(&aif_hdr, 1, hdr_numb, out_file) == hdr_numb);
}

static bool save_aif(FILE *out_file, Sample_t *samp)
{
    assert(samp != NULL);
    assert(samp->audio.buffer != NULL);
    assert(samp->audio.length > 1);

    if(!write_aif_hdr(out_file, samp)) return false;

    if(exporting16bit())
    {
        for(int i = 0; i < samp->audio.length; i++)
        {
            int16_t temp_samp = BE16(samp->audio.buffer[i]);
            if(fwrite(&temp_samp, 1, 2, out_file) != 2) return false;
        }
    }
    else
    {
        for(int i = 0; i < samp->audio.length; i++)
        {
            int8_t temp_samp = (samp->audio.buffer[i] >> 8);
            if(fwrite(&temp_samp, 1, 1, out_file) != 1) return false;
        }
    }


    return true;    
}

static bool write_iff_hdr(FILE *out_file, const Sample_t *samp)
{
	/* FORM 8SVX format header for writing IFF files */

#pragma pack(push,1)
    struct Svx8_Header_u
    {
        uint32_t form_id;
        uint32_t form_size;
        uint32_t svx8_id;

        struct VHDR_Chunck_s
        {
            uint32_t vhdr_id;
            uint32_t vhdr_size;
            uint32_t loop_start;
            uint32_t loop_length;
            uint32_t samp_per_cycle;
            uint16_t samp_per_sec;
            uint8_t  num_oct;
            uint8_t  comp_type;
            uint32_t volume;
        } vhdr;

        struct BODY_Chunck_s
        {
            uint32_t body_id;
            uint32_t body_size;
        } body;
    } svx8_hdr;
#pragma pack(pop)

    const bool loop_enable = samp->is_looped;
    const uint16_t samp_rate = (uint16_t) (samp->rate / ceil(samp->rate / UINT16_MAX));
    const size_t hdr_numb = sizeof(struct Svx8_Header_u);

    const uint32_t samp_len = (uint32_t) samp->audio.length;

    svx8_hdr.form_id = get_chunk_id("FORM");
    svx8_hdr.form_size = BE32(samp_len + (hdr_numb - 8));
    svx8_hdr.svx8_id = get_chunk_id("8SVX");

    svx8_hdr.vhdr.vhdr_id = get_chunk_id("VHDR");
    svx8_hdr.vhdr.vhdr_size = BE32(20);
    
    svx8_hdr.vhdr.loop_start  = loop_enable ? BE32((uint32_t) samp->loop_start) : BE32(samp_len);
    svx8_hdr.vhdr.loop_length = loop_enable ? BE32(samp->loop_end  - samp->loop_start) : 0;

    svx8_hdr.vhdr.samp_per_cycle = BE32(32);
    svx8_hdr.vhdr.samp_per_sec   = BE16(samp_rate);
    svx8_hdr.vhdr.num_oct   = (uint8_t) 1;
    svx8_hdr.vhdr.comp_type = (uint8_t) 0;
    svx8_hdr.vhdr.volume = BE32(0x10000);

    svx8_hdr.body.body_id = get_chunk_id("BODY");
    svx8_hdr.body.body_size = BE32(samp_len);

    // tests for num bytes written, instead of num elements
    return (fwrite(&svx8_hdr, 1, hdr_numb, out_file) == hdr_numb);
}

static bool save_iff(FILE *out_file, const Sample_t *samp)
{
    assert(samp != NULL);
    assert(samp->audio.buffer != NULL);
    assert(samp->audio.length > 1);

    if(!write_iff_hdr(out_file, samp)) return false;

    for (int i = 0; i < samp->audio.length; i++)
    {
        int8_t temp_samp = (samp->audio.buffer[i] & 0xFF00) >> 8;
        if(fwrite(&temp_samp, 1, 1, out_file) < 1) return false;
    }

    return true;
}

static int16_t CLAMP16(int n)
{
    if ((int16_t) n != n) n = (int16_t)(0x7FFF - (n >> 24));
    return (int16_t) n;
}

/*
*   BRR conversion from kode54's BRR converter:
*       https://forums.nesdev.org/viewtopic.php?t=5737
*       https://web.archive.org/web/20140921083332/http://kode54.foobar2000.org/brr.cpp.gz (direct link to brr.cpp.gz file)
*/
static double AdpcmMashS(int16_t* value, int filter, int16_t* inputBuffer, int shiftStep, uint8_t* outputBuffer)
{
    int16_t* ip, * itop;
    uint8_t* output_ptr;
    int ox = 0;
    int16_t v0, v1, step;
    double d2;

    ip = inputBuffer;		/* point input to 1st input sample for this channel */
    itop = inputBuffer + 16;
    v0 = value[0];
    v1 = value[1];
    d2 = 0;

    step = 1 << shiftStep;

    output_ptr = outputBuffer;			/* output pointer (or NULL) */
    for (; ip < itop; ip ++)
    {
        int vlin = 0, d, da, dp, c;

        switch (filter)
        {
        case 0:
            vlin = 0;
            break;

        case 1:
            vlin = v0 >> 1;
            vlin += (-v0) >> 5;
            break;

        case 2:
            vlin = v0;
            vlin += (-(v0 + (v0 >> 1))) >> 5;
            vlin -= v1 >> 1;
            vlin += v1 >> 5;
            break;

        case 3:
            vlin = v0;
            vlin += (-(v0 + (v0 << 2) + (v0 << 3))) >> 7;
            vlin -= v1 >> 1;
            vlin += (v1 + (v1 >> 1)) >> 4;
            break;
        }
        d = (*ip >> 1) - vlin;		/* difference between linear prediction and current sample */
        da = abs(d);
        if (da > 16384 && da < 32768)
        {
            /* Take advantage of wrapping */
            d = d - 32768 * (d >> 24);
        }
        dp = d + (step << 2) + (step >> 2);
        c = 0;
        if (dp > 0)
        {
            if (step > 1)
                c = dp / (step / 2);
            else
                c = dp * 2;
            if (c > 15)
                c = 15;
        }
        c -= 8;
        dp = (c << shiftStep) >> 1;		/* quantized estimate of samp - vlin */
        /* edge case, if caller even wants to use it */
        if (shiftStep > 12)
            dp = (dp >> 14) & ~0x7FF;
        c &= 0x0f;		/* mask to 4 bits */

        v1 = v0;			/* shift history */
        v0 = (signed short)(CLAMP16(vlin + dp) * 2);

        d = *ip - v0;
        d2 += (double)d * d;		/* update square-error */

        if (output_ptr)
        {			/* if we want output, put it in proper place */
            output_ptr[ox >> 1] |= c << (4 - (4 * (ox & 1)));
            ox ++;
        }
    }
    
    d2 /= 16.0;			/* be sure it's non-negative */

    if (output_ptr)
    {
        /* when generating real output, we want to return these */
        value[0] = v0;
        value[1] = v1;
    }
    return sqrt(d2);
}

static void AdpcmBlockMashI(signed short* ip, unsigned char* obuff, signed short* v)
{
    int shift = 1, shift_min = 0;
    int coeff = 0, coeff_min = 0;

    double dmin = 0.0;

    memset(obuff, 0, 9);

    for (shift = 0; shift < 13; shift++)
    {
        for (coeff = 0; coeff < 4; coeff++)
        {
            double d = AdpcmMashS(&v[0], coeff, ip, shift, NULL);

            if ((!shift && !coeff) || d < dmin)
            {
                coeff_min = coeff;
                dmin = d;
                shift_min = shift;
            }
        }
    }

    obuff[0] = (char)((shift_min << 4) | (coeff_min << 2));

    AdpcmMashS(&v[0], coeff_min, ip, shift_min, obuff + 1);
}

static bool save_brr(FILE *out_file, const Sample_t *samp)
{
    bool success = true, loop_enable = false;
    int brr_offset = 0, block_count = 0, pad_offset = 0;
    int sample_length = 0, block_offset = 0;

    size_t brr_len = 0;
    uint8_t* brr_buffer = NULL;
    int16_t v[2] = { 0, 0 };

    assert(samp != NULL);
    assert(samp->audio.buffer != NULL);
    assert(samp->audio.length > 1);

    loop_enable = samp->is_looped;
    brr_offset = loop_enable ? 2 : 0;
    sample_length = loop_enable ? samp->loop_end : samp->audio.length;
    block_offset = (sample_length % 16) == 0 ? 0 : 16 - (sample_length % 16);

    for (int i = 0; i < 16; i++)
    {
        if (samp->audio.buffer[i] != 0)
        {
            pad_offset = 9;
            break;
        }
    }

    block_count = pad_offset + brr_offset;

    if((brr_len = (size_t) (BRRPOS2BYTEPOS(sample_length + block_offset)) + block_count) < 10) return false;

    SBC_CALLOC(brr_len, sizeof *brr_buffer, brr_buffer);

    for (int i = 0; i < sample_length + block_offset; i += 16)
    {
        int16_t tempSamp[16];
        uint8_t tempBrr  [9];

        for (int j = 0; j < 16; j++)
        {
            if (i < 16 && j <= block_offset) 
                tempSamp[j] = 0;
            else if ((i + j) < sample_length + block_offset)
                tempSamp[j] = samp->audio.buffer[i + j - block_offset];
            else
                break;
        }

        AdpcmBlockMashI(tempSamp, tempBrr, (short*)&v);

        for (int b = 0; b < 9; b++)
        {
            if (block_count + b < (int) brr_len)
                brr_buffer[block_count + b] = tempBrr[b];
            else
                break;

            if (loop_enable && b == 0)
                brr_buffer[block_count + b] ^= 2;
        }

        block_count += 9;
    }

    brr_buffer[brr_len - 9] ^= 1;

    if (loop_enable)
    {
        uint16_t start_loop_block = (uint16_t) pad_offset + (uint16_t) BRRPOS2BYTEPOS(samp->loop_start + block_offset);

        brr_buffer[0] = (uint8_t)(start_loop_block & ~0xFF00);
        brr_buffer[1] = (uint8_t)((start_loop_block & ~0x00FF) >> 8);
    }
        
    if(fwrite(brr_buffer, 1, brr_len, out_file) != brr_len) success = false;

    SBC_FREE(brr_buffer);

    return success;
}

static bool save_mu(FILE *out_file, const Sample_t *samp)
{
    assert(samp != NULL);
    assert(samp->audio.buffer != NULL);
    assert(samp->audio.length > 1);

    for (int i = 0; i < samp->audio.length; i++)
    {
        const int16_t temp_samp = samp->audio.buffer[i];
        const double encoding = (255.0 * (double) abs(temp_samp)) / 32767.0;
        uint8_t encodedSample = (uint8_t) round(log(1.0 + encoding) * 22.91);

        if(temp_samp > 0) encodedSample ^= 0x80;
        if(fwrite(&encodedSample, 1, 1, out_file) < 1) return false;
    }

    return true;
}

static bool save_raw(FILE *out_file, const Sample_t *samp)
{
    assert(samp != NULL);
    assert(samp->audio.buffer != NULL);
    assert(samp->audio.length > 1);

    if(exporting16bit()) return (fwrite(samp->audio.buffer, 
                                    sizeof *samp->audio.buffer, 
                                    samp->audio.length, 
                                    out_file) == (size_t) samp->audio.length);

    for (int i = 0; i < samp->audio.length; i++)
    {
        int8_t temp_samp = (samp->audio.buffer[i]) >> 8;
        if(fwrite(&temp_samp, 1, 1, out_file) < 1) return false;
    }

    return true;
}

static bool cleanup_and_close(const char* filepath, FILE **out_file, Sample_t **samp)
{
    bool success = true;
    long file_len = 1;

    assert(out_file  != NULL &&   samp  != NULL);
    assert(*out_file != NULL && (*samp) != NULL);

    SBC_FREE((*samp)->audio.buffer);
    SBC_FREE((*samp));

	fseek((*out_file), 0, SEEK_END);
    file_len = ftell(*out_file);

    if(fclose((*out_file)) != 0) success = false;
    if(file_len == 0) if(remove(filepath) != 0) success = false;

    return success;
}

void saveFile (const char* filepath)
{
	Sample_t *samp_export = NULL, *samp_edit = getSampleEdit();
    const size_t EXT_LEN = 4, buf_size = sizeof *samp_edit->audio.buffer;

    size_t pathlen;
    bool success = 1;

    FILE *out_file = NULL;
    char file_type[8];

    assert(samp_edit != NULL);

    if(isStringEmpty(filepath)) return;
    if(samp_edit->audio.buffer == NULL || samp_edit->audio.length <= 1) return;

	if((out_file = fopen(filepath, "wb")) == NULL)
	{
		showErrorMsgBox("File Write Error", "Unable to save file! ", strerror(errno));
		return;
	}

    SBC_CALLOC(1, sizeof(Sample_t), samp_export);

    samp_export->audio.length = samp_edit->audio.length - samp_edit->samp_start;
    
    if(samp_export->audio.length <= 0)
    {
        fclose(out_file);
        SBC_FREE(samp_export);
        return;
    }

    SBC_MALLOC(samp_export->audio.length, buf_size, samp_export->audio.buffer);
    memcpy(samp_export->audio.buffer, samp_edit->audio.buffer + samp_edit->samp_start, samp_export->audio.length * buf_size);

    samp_export->rate       = samp_edit->rate;
    samp_export->is_looped  = samp_edit->is_looped;
    samp_export->loop_end   = samp_edit->loop_end   - samp_edit->samp_start;
    samp_export->loop_start = samp_edit->loop_start - samp_edit->samp_start;

    pathlen = strlen(filepath);
    memset(file_type, '\0', 8);
    
    if(pathlen - (_strcasestr(filepath, ".wav") - filepath) == EXT_LEN) 
    { 
        memcpy(file_type, "WAV", 4);
        success = save_wav(out_file, samp_export);
    }
    else if((pathlen - (_strcasestr(filepath, ".aif")  - filepath) == EXT_LEN) ||
            (pathlen - (_strcasestr(filepath, ".aiff") - filepath) == EXT_LEN + 1))
    {
        memcpy(file_type, "AIFF", 5);
        success = save_aif(out_file, samp_export);
    }
    else if(pathlen - (_strcasestr(filepath, ".iff") - filepath) == EXT_LEN)
    {
        memcpy(file_type, "8SVX", 5);
        success = save_iff(out_file, samp_export);
    }
    else if(pathlen - (_strcasestr(filepath, ".brr") - filepath) == EXT_LEN) 
    {
        memcpy(file_type, "BRR", 4);
        success = save_brr(out_file, samp_export);
    }
    else if(pathlen - (_strcasestr(filepath, ".bin") - filepath) == EXT_LEN)
    {
        memcpy(file_type, "mu-Law", 7);
        success = save_mu(out_file, samp_export);
    }
    else
    {
        memcpy(file_type, "raw PCM", 8);
        success = save_raw(out_file, samp_export);
    }

    if(!success)
    {
        char box_msg[46];
        memset(box_msg, '\0', 46);

        snprintf(box_msg, 46, "Unexpected error while writing %s file!\n", file_type);
        showErrorMsgBox("File Write Error", box_msg, strerror(errno));
    }

    if(!cleanup_and_close(filepath, &out_file, &samp_export)) 
		showErrorMsgBox("File Close Error", "Unexpected error while closing file!\n", strerror(errno));
}
