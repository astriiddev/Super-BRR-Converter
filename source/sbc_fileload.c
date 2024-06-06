#include <math.h>

#include "sbc_utils.h"
#include "sbc_pitch.h"

#include "sbc_samp_edit.h"
#include "sbc_fileload.h"

#define LE16(a,i)       (*(uint16_t *) ((a) + i))
#define LE32(a,i)       (*(uint32_t *) ((a) + i))

#define BE16(a,b)       ((uint8_t)(a) <<  8 | (uint8_t)(b) <<  0)
#define BE32(a,b,c,d)   ((uint8_t)(a) << 24 | (uint8_t)(b) << 16 | (uint8_t)(c) <<  8 | (uint8_t)(d) <<  0)

#define BYTE2WORD(x)    ((int16_t) (((uint8_t) (x) << 8) | (uint8_t) (x)))
#define CLAMP16(s)      ((int16_t) (s) == (s)) ? (int16_t) (s) : (int16_t) (INT16_MAX ^ ((s) >> 4))

/* equivalent to (int16_t) round(numerator / denominator *  in) */
#define FIXEDMATH16_16(x,n,d)   ((int16_t) (((x) * ((int64_t) (n) << 16) / (d)) >> 16))

/* 
*   converts number of bytes to number of samples as per BRR's 16 sample to 9 byte ratio 
*   equivalent to (int) round(16.0 / 9.0 * (double) in) 
*/
#define BYTEPOS2BRRPOS(x)       ((int) (((int64_t) (x) * ((16 << 16) / 9) + 0x8000) >> 16))

typedef struct BrrFilter_s
{
    int16_t a, b, tmp[2];
} brrfilter_t;

static void set_loop_points(Sample_t *s, const bool enable, const int start, const int end)
{
    assert(s != NULL);

    s->is_looped  = enable;
    s->loop_start = start;
    s->loop_end   = end;
    s->samp_start = 0;
}

static bool create_sample_buffer(Sample_t **s, const int sample_len)
{
    assert(s != NULL);
    assert(sample_len > 1);

    (*s) = calloc(1, sizeof(Sample_t));

    if((*s) == NULL)
    {
        showErrorMsgBox("Memory allocation error", "Insufficient memory!", NULL);
        return false;
    }

    (*s)->audio.length = sample_len;

    (*s)->audio.buffer = malloc((*s)->audio.length * sizeof *(*s)->audio.buffer);

    if((*s)->audio.buffer == NULL)
    {
        showErrorMsgBox("Memory allocation error", "Insufficient memory!", NULL);
        return false;
    }

    return true;
}

void load_sample_and_free(Sample_t **s)
{
    assert(s != NULL && *s != NULL);
    assert((*s)->audio.buffer != NULL);

    setSampleEdit(*s);

    SBC_FREE((*s)->audio.buffer);
    SBC_FREE((*s));
}

static int find_chunk_name(const char* haystack, const char* needle, const int len)
{
    const uint32_t chunk  = LE32(needle, 0);
    const char* temp = haystack;

    int position = -1, bytesToRead = len;

    while (--bytesToRead >= 0)
    {
        const uint32_t chunkTest = LE32(temp++, 0);

        if(chunk == chunkTest) 
        {
            position = (int) (temp - haystack - 1);
            break;
        }
    }

    return position;
}

static bool find_chunks(const char* haystack, const char *chunk[], int numchunks, const int len)
{
    const int MAIN_CHUNK_POS = 0, SUB1_CHUNK_POS = 8, SUB2_CHUNK_POS = 12;

    if(find_chunk_name(haystack, chunk[0], len) != MAIN_CHUNK_POS) return false;
    if(find_chunk_name(haystack, chunk[1], len) != SUB1_CHUNK_POS) return false;

    if(numchunks < 3) return true;

    if(find_chunk_name(haystack, chunk[2], len) != SUB2_CHUNK_POS)
    {
        if(numchunks < 4) return false;
        if(find_chunk_name(haystack, chunk[3], len) != SUB2_CHUNK_POS) return false;
    }

    return true;
}

static bool rawpcmread(const char* file_buf, const int file_len)
{
    Sample_t *samp_load = NULL;

    assert(file_buf != NULL);

    if(!create_sample_buffer(&samp_load, file_len)) 
    {
        SBC_FREE(samp_load);
        return false;
    }

    for(int i = 0; i < samp_load->audio.length; i++)
        samp_load->audio.buffer[i] = BYTE2WORD(file_buf[i]);

    set_loop_points(samp_load, false, 0, samp_load->audio.length);
    samp_load->rate = 16726.0;

    load_sample_and_free(&samp_load);
    detectCenterPitch(false);

    return true;
}

static int wavparse(const char* file_buf, const int file_len)
{
    Sample_t *samp_load = NULL;
    
    int smplpos = -1, data_pos = 0;
    bool loop_enable = false;

    uint32_t samp_rate  = 0, new_samp_len = 0,
             loop_start = 0, loop_end = 0;
    int16_t num_chan = 0, bit_depth = 0;

    assert(file_buf != NULL);

    if((data_pos = find_chunk_name(file_buf, "data", file_len)) > 0) data_pos += 8;
    else return 1;

    num_chan         = LE16(file_buf, 22);
    samp_rate        = LE32(file_buf, 24);
    bit_depth        = LE16(file_buf, 34);
    new_samp_len     = LE32(file_buf, data_pos - 4);

    if(num_chan > 2) return -1;
    else if(num_chan == 2)
        showErrorMsgBox("Stereo WAV File", "Stereo WAV file detected. Reading from left channel...", NULL);

    new_samp_len /= (bit_depth / 8);
    new_samp_len /= num_chan;
    
    if(!create_sample_buffer(&samp_load, new_samp_len))
    {
        SBC_FREE(samp_load);
        return false;
    }
    
    for(int i = 0; i < samp_load->audio.length; i++)
    {
        const int n = i * num_chan;

        if(bit_depth == 8)
        {
            int8_t usamp = (int8_t) (file_buf[n + data_pos] ^ 0x80);
            samp_load->audio.buffer[i] = BYTE2WORD(usamp);
        }
        else if(bit_depth == 16)
        {
            samp_load->audio.buffer[i] = LE16(file_buf, (n * 2) + data_pos);
        }
        else if(bit_depth == 24)
        {
            samp_load->audio.buffer[i] = LE16(file_buf, (n * 3) + data_pos + 1);
        }
        else
        {
            float fsamp = *(float *) (file_buf + (n * 4));
            samp_load->audio.buffer[i] = (int16_t) floorf(fsamp * 32767.5f);
        }
    }

    if((smplpos = find_chunk_name(file_buf, "smpl", file_len)) > -1)
    {
        loop_enable = true;

        smplpos += 52;

        loop_start = LE32(file_buf, smplpos + 0);
        loop_end   = LE32(file_buf, smplpos + 4);

        loop_end++;

        if(loop_start <= 0) loop_start = 0;

        if((int) loop_end > samp_load->audio.length)
                 loop_end = samp_load->audio.length;

        if((int) loop_start > samp_load->audio.length)
        {
            loop_start  = 0;
            loop_end    = samp_load->audio.length;
            loop_enable = false;
        }

        if(loop_end <= 0)
        {
            loop_start  = 0;
            loop_end    = samp_load->audio.length;
            loop_enable = false;
        }
    }
    else
    {
        loop_start  = 0;
        loop_end    = samp_load->audio.length;
        loop_enable = false;
    }

    set_loop_points(samp_load, loop_enable, loop_start, loop_end);
    samp_load->rate = (double) samp_rate;

    load_sample_and_free(&samp_load);

    return 0;
}

/*
 * C O N V E R T   F R O M   I E E E   E X T E N D E D  
 */
/* 
 * Copyright (C) 1988-1991 Apple Computer, Inc.
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

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

# define UnsignedToFloat(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

double ConvertFromIeeeExtended(unsigned char (*bytes)[10])
{
    double    f;
    int    expon;
    unsigned long hiMant, loMant;
    
    expon = (((*bytes)[0] & 0x7F) << 8) | ((*bytes)[1] & 0xFF);
    hiMant    =    ((unsigned long)((*bytes)[2] & 0xFF) << 24)
            |    ((unsigned long)((*bytes)[3] & 0xFF) << 16)
            |    ((unsigned long)((*bytes)[4] & 0xFF) << 8)
            |    ((unsigned long)((*bytes)[5] & 0xFF));
    loMant    =    ((unsigned long)((*bytes)[6] & 0xFF) << 24)
            |    ((unsigned long)((*bytes)[7] & 0xFF) << 16)
            |    ((unsigned long)((*bytes)[8] & 0xFF) << 8)
            |    ((unsigned long)((*bytes)[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0) {
        f = 0;
    }
    else {
        if (expon == 0x7FFF) {    /* Infinity or NaN */
            f = HUGE_VAL;
        }
        else {
            expon -= 16383;
            f  = ldexp(UnsignedToFloat(hiMant), expon -= 31);
            f += ldexp(UnsignedToFloat(loMant), expon -  32);
        }
    }

    if ((*bytes)[0] & 0x80)
        return -f;
    else
        return f;
}

static bool aifparse(const char* file_buf, const int file_len)
{
    Sample_t *samp_load = NULL;

    int data_pos = 0, comm_pos = 0, num_chan = 0, bit_depth = 0, new_samp_len = 0;
    double samp_rate = 0.0;
    uint8_t samp_rate_data[10];

    assert(file_buf != NULL);

    if((comm_pos = find_chunk_name(file_buf, "COMM", file_len)) < 0) return false;
    if((data_pos = find_chunk_name(file_buf, "SSND", file_len)) > 0) data_pos += 16;
    else return false;

    memset(samp_rate_data, 0, 10);

    num_chan         = BE16(file_buf[comm_pos + 8], file_buf[comm_pos + 9]);
    bit_depth        = BE16(file_buf[comm_pos + 14], file_buf[comm_pos + 15]);
    new_samp_len     = BE32(file_buf[data_pos - 12], file_buf[data_pos - 11],
                            file_buf[data_pos - 10], file_buf[data_pos -  9]);
    new_samp_len    -= 8;

    if(file_len < comm_pos + 26) return false;
    memcpy(samp_rate_data, (file_buf + comm_pos + 16), 10);
    samp_rate = ConvertFromIeeeExtended(&samp_rate_data);

    if(num_chan > 2 || bit_depth > 16) return false;
    else if(num_chan == 2)
        showErrorMsgBox("Stereo WAV File", "Stereo WAV file detected. Reading from left channel...", NULL);


    new_samp_len /= (bit_depth / 8);
    new_samp_len /= num_chan;
    
    if(!create_sample_buffer(&samp_load, new_samp_len))
    {
        SBC_FREE(samp_load);
        return false;
    }

    for(int i = 0; i < samp_load->audio.length; i++)
    {
        const int n = i * num_chan;

        if(bit_depth == 8)
        {
            samp_load->audio.buffer[i] = BYTE2WORD(file_buf[n + data_pos]);
        }
        else
        {
            int d = (n * 2) + data_pos;
            samp_load->audio.buffer[i] = BE16(file_buf[d], file_buf[d + 1]);
        }
    }

    set_loop_points(samp_load, false, 0, samp_load->audio.length);
    samp_load->rate = samp_rate;

    load_sample_and_free(&samp_load);

    return true;
}

static bool iffparse(const char* file_buf, const int file_len)
{
    Sample_t *samp_load = NULL;

    int data_pos = 0, samp_rate = 0, new_samp_len = 0, loop_start = 0, loop_end = 0;;
    bool loop_enable = false;

    assert(file_buf != NULL);

    if((data_pos = find_chunk_name(file_buf, "BODY", file_len)) > 0) data_pos += 8;
    else return false;

    samp_rate        = BE16(file_buf[0x20], file_buf[0x21]);
    new_samp_len     = BE32(file_buf[data_pos - 4], file_buf[data_pos - 3],
                            file_buf[data_pos - 2], file_buf[data_pos - 1]);
    
    if(!create_sample_buffer(&samp_load, new_samp_len))
    {
        SBC_FREE(samp_load);
        return false;
    }

    for(int i = 0; i < samp_load->audio.length; i++)
            samp_load->audio.buffer[i] = BYTE2WORD(file_buf[i + data_pos]);

    loop_start = BE32(file_buf[20], file_buf[21], file_buf[22], file_buf[23]);
    loop_end   = BE32(file_buf[24], file_buf[25], file_buf[26], file_buf[27]);

    if(loop_start > samp_load->audio.length)
    {
        loop_start  = 0;
        loop_end    = samp_load->audio.length;
        loop_enable = false;
    }
    else if(loop_end <= 0)
    {
        loop_start  = 0;
        loop_end    = samp_load->audio.length;
        loop_enable = false;
    }
    else
    {
        loop_end   += loop_start;
        loop_enable = true;
    }

    set_loop_points(samp_load, loop_enable, loop_start, loop_end);
    samp_load->rate = (double) samp_rate;

    load_sample_and_free(&samp_load);

    return true;
}

static bool vcparse(const char* file_buf, const int file_len)
{
    Sample_t *samp_load = NULL;

    int data_pos = 0x1500, new_samp_len = 0x4000, loop_start = 0, loop_end = new_samp_len;
    bool loop_enable = false;

    assert(file_buf != NULL);

    if(data_pos + new_samp_len >= file_len) return false;

    if(!create_sample_buffer(&samp_load, new_samp_len))
    {
        SBC_FREE(samp_load);
        return false;
    }
    
    for(int i = 0; i < samp_load->audio.length; i++)
        samp_load->audio.buffer[i] = BYTE2WORD(file_buf[i + data_pos]);

    if(file_buf[0x133B])
    {
        loop_enable = true;

        loop_start = (file_buf[0x1332] + 1) << 7;
        loop_end   = (file_buf[0x1333] + 2) << 7;

        if(loop_start <= 0)
                loop_start = 0;

        if(loop_end > samp_load->audio.length)
                loop_end = samp_load->audio.length;

        if(loop_start > samp_load->audio.length)
        {
                loop_start  = 0;
                loop_end    = samp_load->audio.length;
                loop_enable = false;
        }

        if(loop_end <= 0)
        {
                loop_start  = 0;
                loop_end    = samp_load->audio.length;
                loop_enable = false;
        }
    }

    set_loop_points(samp_load, loop_enable, loop_start, loop_end);
    samp_load->rate = 16744.0;

    load_sample_and_free(&samp_load);

    return true;
}

static bool mulawdecode(const char* file_buf, const int file_len)
{
    Sample_t *samp_load = NULL;

    assert(file_buf != NULL);

    if(!create_sample_buffer(&samp_load, file_len))
    {
        SBC_FREE(samp_load);
        return false;
    }

    for(int i = 0; i < samp_load->audio.length; i++)
    {
        double   temp_decode = (file_buf[i] & 0x80) ? 1.0 : -1.0;
        int32_t temp_samp = 0;

        temp_decode *= pow(256, (double)(file_buf[i] & 0x7F) / 127) - 1;
        temp_decode /= 255;

        temp_samp = (int32_t)round(temp_decode * INT16_MAX);

        samp_load->audio.buffer[i] = CLAMP16(temp_samp);
    }

    set_loop_points(samp_load, false, 0, samp_load->audio.length);
    samp_load->rate = 22050.;

    load_sample_and_free(&samp_load);

    return true;
}

static void set_brr_coeff(brrfilter_t* f, char type)
{
    switch (type)
    {
    case 1:
        f->a = FIXEDMATH16_16(f->tmp[0], 15, 16);
        f->b = 0;
        break; 
                    
    case 2:      
        f->a = FIXEDMATH16_16(f->tmp[0], 61, 32);
        f->b = FIXEDMATH16_16(f->tmp[1], 15, 16);
        break;
                    
    case 3:
        f->a = FIXEDMATH16_16(f->tmp[0], 115, 64);
        f->b = FIXEDMATH16_16(f->tmp[1],  13, 16);
        break;

    default:
        f->a = f->b = 0;
        break;
    }
}

static void filter_brr(brrfilter_t* f, const int16_t shifted_samp, int16_t *sample)
{
    const int16_t out = shifted_samp + f->a - f->b;

    f->tmp[1] = f->tmp[0];
    f->tmp[0] = out;
    *sample = out;
}

static bool brrdecode(const char* file_buf, const int file_len)
{
    Sample_t *samp_load = NULL;
    int16_t *src_buffer = NULL;

    assert(file_buf != NULL);

    int data_pos = 0, end_pos = file_len, new_samp_len = 0, loop_start = 0, loop_end = 0;
    char filter_flag = 0, shifter = 0;

    bool loop_enable = false, loopFlag = false, endFlag = false;

    int16_t temp_samp;
    brrfilter_t brrfilter;

    if (file_len % 9 == 0)
    {
        data_pos = 0;
        new_samp_len = BYTEPOS2BRRPOS(file_len);

        loop_enable = false;
        loop_start  = 0;
        loop_end    = file_len;
    }
    else if ((file_len - 2) % 9 == 0)
    {
        loop_enable = true;
        loop_start  = BYTEPOS2BRRPOS(LE16(file_buf, 0));

        data_pos = 2;
        new_samp_len = BYTEPOS2BRRPOS(file_len - 2);
    }
    else return false;

    if(!create_sample_buffer(&samp_load, new_samp_len))
    {
        SBC_FREE(samp_load);
        return false;
    }

    memset(&brrfilter, 0, sizeof(brrfilter_t));
    memset(samp_load->audio.buffer, 0, samp_load->audio.length * sizeof *samp_load->audio.buffer);

    src_buffer = (int16_t*) samp_load->audio.buffer;

    for(int i = 0; i < file_len - data_pos; i++)
    {
        const uint8_t in_byte = (uint8_t) file_buf[i + data_pos];

        if (i % 9 == 0)
        {
            shifter = (in_byte & ~0x0F) >> 4;
            if (shifter > 12) shifter = 12;

            filter_flag = (in_byte & ~0xF3) >> 2;
            if (filter_flag > 3) filter_flag = 3;

            loopFlag = (in_byte & 0x02) ? true : false;

            if (in_byte & 0x01)
            {
                endFlag = true;
                end_pos = i + data_pos;

                if (loopFlag && endFlag)
                {
                    loop_enable = true;
                    loop_end = BYTEPOS2BRRPOS(end_pos + 7);
                }
                else
                {
                    loop_enable = false;
                    loop_start = 16;
                    loop_end = new_samp_len;
                }
            }
            else
            {
                end_pos = file_len;
            }
        }

        else if (i % 9 != 0 && shifter <= 12)
        {
            char nibble = ((in_byte & ~0x0F) >> 4);
            if (nibble > 7) nibble ^= -16;

            set_brr_coeff(&brrfilter, filter_flag);
            filter_brr(&brrfilter, nibble << shifter, &temp_samp);

            *src_buffer++ = i + data_pos <= end_pos + 8 ? temp_samp : 0;

            nibble = (in_byte & ~0xF0);
            if (nibble > 7) nibble ^= -16;

            set_brr_coeff(&brrfilter, filter_flag);
            filter_brr(&brrfilter, nibble << shifter, &temp_samp);

            *src_buffer++ = i + data_pos <= end_pos + 8 ? temp_samp : 0;
        }
    }

    set_loop_points(samp_load, loop_enable, loop_start, loop_end);

    samp_load->rate = 16744.0;

    load_sample_and_free(&samp_load);

    detectCenterPitch(false);

    return true;
}

int loadFile(const char* file_path)
{
    char * file_buf = NULL;
    int file_len = 0, sample_loaded = 1;

	FILE *fd = NULL;

    if(file_path == NULL) return 0;

	fd = fopen(file_path, "rb");

	if (fd == NULL)
	{
        showErrorMsgBox("Error 404", "File not found!", NULL);
		return 0;
	}

	fseek(fd, 0, SEEK_END);
	file_len = (int) ftell(fd);
	rewind(fd);

	file_buf = malloc(file_len * sizeof *file_buf);

	if (file_buf)
	{
		if (fread(file_buf, 1, file_len, fd) != (size_t) file_len)
		{
            showErrorMsgBox("File Read Error", "Cannot read file!", NULL);
            SBC_FREE(file_buf);
            fclose(fd);
            return 0;
		}
	}
    else
    {
        fclose(fd);
        return 0;
    }

    fclose(fd);

    if(find_chunks(file_buf, (const char*[]){"RIFF", "WAVE", "fmt ", "JUNK"}, 4, file_len))
    {
        int success = 0;

        if((success = wavparse(file_buf, file_len)) != 0)
        {
            if(success < 2) showErrorMsgBox("WAV File Error!", success == 1 ?  
                                            "\'data\' chunk not found!" : 
                                            "Channel format not supported!", NULL);
            sample_loaded = 0;
        }            
    }
    
    else if(find_chunks(file_buf, (const char*[]){"FORM", "AIFF"}, 2, file_len))
    {
        if(!aifparse(file_buf, file_len))
        {
            printf("Error reading AIF samples!\n");
            sample_loaded = 0;
        }
    } 
    
    else if(find_chunks(file_buf, (const char*[]){"FORM", "8SVX", "VHDR"}, 3, file_len))
    {
        if(!iffparse(file_buf, file_len))
        {
            printf("Error reading IFF samples!\n");
            sample_loaded = 0;
        }
    }
    
    else if(_strcasestr(file_path, ".vc"))
    {
        if(!vcparse(file_buf, file_len))
        {
            printf("Error reading VC samples!\n");
            sample_loaded = 0;
        }
    }
    else if(_strcasestr(file_path, ".brr"))
    {
        if(!brrdecode(file_buf, file_len))
        {
            printf("Error reading BRR samples!\n");
            sample_loaded = 0;
        }
    }

    else if(_strcasestr(file_path, ".bin") || _strcasestr(file_path, ".eii"))
    {
        if(!mulawdecode(file_buf, file_len))
        {
                printf("Error reading MuLAW samples!\n");
                sample_loaded = 0;
        }
    }

    else
    {
        if(!rawpcmread(file_buf, file_len))
        {
            printf("Error reading RAW samples!\n");
            sample_loaded = 0;
        }
    }

    SBC_FREE(file_buf);

    return sample_loaded;
}
