/*

Copyright (C) 2008, 2009 Nokia Corporation.
This material, including documentation and any related
computer programs, is protected by copyright controlled by
Nokia Corporation. All rights are reserved. Copying,
including reproducing, storing,  adapting or translating, any
or all of this material requires the prior written consent of
Nokia Corporation. This material also contains confidential
information which may not be disclosed to others without the
prior written consent of Nokia Corporation.

*/

#include <string.h>
#include <stdlib.h>
#include "src-48-to-8.h"

#ifdef ARM_DSP
#include <dspfns.h>
#endif

#define DOWNSAMPLING_FACTOR_1 3
#define FILTER_LENGTH_1 16
#define FILTER_MEMORY_1 15
/* FILTER_MEMORY_HOP_1 = (int) FILTER_LENGTH_1 / DOWNSAMPLING_FACTOR_1 */
#define FILTER_MEMORY_HOP_1 5
/* STEREO_DOWNSAMPLING_FACTOR_1 = DOWNSAMPLING_FACTOR_1 * 2 */
#define STEREO_DOWNSAMPLING_FACTOR_1 6

#define DOWNSAMPLING_FACTOR_2 2
#define FILTER_LENGTH_2 80
#define FILTER_MEMORY_2 79
/* (int) FILTER_LENGTH_2 / DOWNSAMPLING_FACTOR_2 */
#define FILTER_MEMORY_HOP_2 40

static short temp_buffer[SRC_48_TO_8_MAX_INPUT_FRAMES / 3];

struct src_48_to_8
{
  short filter_memory_1[FILTER_LENGTH_1 * 2];
  short filter_memory_2[FILTER_LENGTH_2];
};

static const signed short filter_coeffs_1[] =
{
  8, 69, 185, 94, -502, -1527, -2132, -1143,
  1841, 5719, 8467, 8671, 6566, 3651, 1390, 290
};

static const signed short filter_coeffs_2[] =
{
  2, -6, 4, 4, -5, -6, 7, 9, -10, -12, 13, 17,
  -18, -24, 23, 34, -28, -46, 34, 62, -40, -84,
  44, 111, -46, -146, 43, 189, -31, -242, 6, 304,
  42, -370, -126, 421, 261, -376, -313, 366, 333,
  -477, -555, 377, 631, -371, -808, 261, 939, -146,
  -1094, -42, 1218, 277, -1319, -584, 1361, 958,
  -1319, -1403, 1144, 1901, -776, -2414, 131, 2851,
  890, -3020, -2377, 2523, 4284, -585, -5955, -4086,
  4336, 11559, 12142, 7681, 2961, 564
};

src_48_to_8 *alloc_src_48_to_8(void)
{
  src_48_to_8 *src = (src_48_to_8 *) malloc(sizeof(src_48_to_8));
  memset(src, 0, sizeof(*src));
  return src;
}

void free_src_48_to_8(src_48_to_8 *src)
{
  free(src);
}

#ifdef USE_SATURATION
#ifdef ARM_DSP
static inline int EAP_Clip16(int input)
{
  return __ssat( input, 16 );
}
#else
static inline int EAP_Clip16(int input)
{  input = input < (-32768) ? (-32768) : input;
  input = input > 32767 ?  32767 : input;
  return input;
}
#endif
#endif

int process_src_48_to_8(src_48_to_8 *s,
			short *output,
			short *input,
			int input_frames)
{
    signed short *input_samples = 0;
    signed short *poly_start = 0;
    signed short *output_samples = 0;
    signed int result = 0;
    int intermediate_frames = input_frames / 3;
    int output_frames = input_frames / 6;
    int samples_to_process_1 = intermediate_frames - FILTER_MEMORY_HOP_1;
    int samples_to_process_2 = output_frames - FILTER_MEMORY_HOP_2;
    int start_point = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    /* polyphase filter from 48kHz to 16kHz */

    /* first process the filter memory */
    start_point = 0;

    for (i = 0; i < FILTER_MEMORY_HOP_1; i++)
    {
      result = 0;

      for (j = start_point, k = 0; j < FILTER_MEMORY_1; j++, k++)
      {
	result += (int)(s->filter_memory_1[j]) * filter_coeffs_1[k];
      }

      for (j = 0; j < start_point + 1; j++, k++)
      {
	result += (int)(input[j]) * filter_coeffs_1[k];
      }

#ifdef USE_SATURATION
      temp_buffer[i] = (short)EAP_Clip16((result + 16384) >> 15);
#else
      temp_buffer[i] = (short)((result + 16384) >> 15);
#endif

      start_point += DOWNSAMPLING_FACTOR_1;
    }

    output_samples = &temp_buffer[FILTER_MEMORY_HOP_1];
    poly_start = &input[0];

    /* then process the rest of the input buffer */
    for (i = 0; i < samples_to_process_1; i++)
    {
      result = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_1[j];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result + 16384) >> 15);
#else
      *output_samples++ = (short)((result + 16384) >> 15);
#endif

      poly_start += DOWNSAMPLING_FACTOR_1;
    }

    /* copy filter memory to the beginning of scratch buffer */
    memcpy((void*)(&(s->filter_memory_1[0])),
	   (void*)poly_start,
	   FILTER_MEMORY_1 * sizeof(short));

    /* polyphase filter from 16kHz to 8kHz */

    /* first process the filter memory */
    start_point = 0;
    for (i = 0; i < FILTER_MEMORY_HOP_2; i++)
    {
      result = 0;

      for (j = start_point, k = 0; j < FILTER_MEMORY_2; j++, k++)
      {
	result += (int)(s->filter_memory_2[j]) * filter_coeffs_2[k];
      }

      for (j = 0; j < start_point + 1; j++, k++)
      {
	result += (int)(temp_buffer[j]) * filter_coeffs_2[k];
      }

#ifdef USE_SATURATION
      output[i] = (short)EAP_Clip16((result + 16384) >> 15);
#else
      output[i] = (short)((result + 16384) >> 15);
#endif

      start_point += DOWNSAMPLING_FACTOR_2;
    }

    output_samples = &output[FILTER_MEMORY_HOP_2];
    poly_start = &temp_buffer[1];

    /* then process the rest of the temp buffer */
    for (i = 0; i < samples_to_process_2; i++)
    {
      result = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_2[j];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result + 16384) >> 15);
#else
      *output_samples++ = (short)((result + 16384) >> 15);
#endif

      poly_start += DOWNSAMPLING_FACTOR_2;
    }

    /* copy filter memory */
    memcpy((void*)(&(s->filter_memory_2[0])),
           (void*)poly_start,
           FILTER_MEMORY_2 * sizeof(short));

    return output_frames;
}

int process_src_48_to_8_stereo_to_mono(src_48_to_8 *s,
				       short *output,
				       short *input,
				       int input_frames)
{
    signed short *input_samples = 0;
    signed short *poly_start = 0;
    signed short *output_samples = 0;
    signed int result = 0;
    int intermediate_frames = input_frames / 6;
    int output_frames = input_frames / 12;
    int samples_to_process_1 = intermediate_frames - FILTER_MEMORY_HOP_1;
    int samples_to_process_2 = output_frames - FILTER_MEMORY_HOP_2;
    int start_point = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    /* polyphase filter from 48kHz to 16kHz */

    /* first process the filter memory */
    start_point = 0;

    for (i = 0; i < FILTER_MEMORY_HOP_1; i++)
    {
      result = 0;

      for (j = start_point, k = 0; j < FILTER_MEMORY_1 * 2; j += 2, k++)
      {
	result += (int)(s->filter_memory_1[j]) * filter_coeffs_1[k];
      }

      for (j = 0; j < (start_point + 1) * 2; j += 2, k++)
      {
	result += (int)(input[j]) * filter_coeffs_1[k];
      }

#ifdef USE_SATURATION
      temp_buffer[i] = (short)EAP_Clip16((result + 16384) >> 15);
#else
      temp_buffer[i] = (short)((result + 16384) >> 15);
#endif

      start_point += STEREO_DOWNSAMPLING_FACTOR_1;
    }

    output_samples = &temp_buffer[FILTER_MEMORY_HOP_1];
    poly_start = &input[0];

    /* then process the rest of the input buffer */
    for (i = 0; i < samples_to_process_1; i++)
    {
      result = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result += (int)(*input_samples) * filter_coeffs_1[j];
	input_samples += 2;
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result + 16384) >> 15);
#else
      *output_samples++ = (short)((result + 16384) >> 15);
#endif

      poly_start += STEREO_DOWNSAMPLING_FACTOR_1;
    }

    /* copy filter memory to the beginning of scratch buffer */
    memcpy((void*)(&(s->filter_memory_1[0])),
	   (void*)poly_start,
	   2 * FILTER_MEMORY_1 * sizeof(short));

    /* polyphase filter from 16kHz to 8kHz */

    /* first process the filter memory */
    start_point = 0;
    for (i = 0; i < FILTER_MEMORY_HOP_2; i++)
    {
      result = 0;

      for (j = start_point, k = 0; j < FILTER_MEMORY_2; j++, k++)
      {
	result += (int)(s->filter_memory_2[j]) * filter_coeffs_2[k];
      }

      for (j = 0; j < start_point + 1; j++, k++)
      {
	result += (int)(temp_buffer[j]) * filter_coeffs_2[k];
      }

#ifdef USE_SATURATION
      output[i] = (short)EAP_Clip16((result + 16384) >> 15);
#else
      output[i] = (short)((result + 16384) >> 15);
#endif

      start_point += DOWNSAMPLING_FACTOR_2;
    }

    output_samples = &output[FILTER_MEMORY_HOP_2];
    poly_start = &temp_buffer[1];

    /* then process the rest of the temp buffer */
    for (i = 0; i < samples_to_process_2; i++)
    {
      result = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_2[j];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result + 16384) >> 15);
#else
      *output_samples++ = (short)((result + 16384) >> 15);
#endif

      poly_start += DOWNSAMPLING_FACTOR_2;
    }

    /* copy filter memory */
    memcpy((void*)(&(s->filter_memory_2[0])),
           (void*)poly_start,
           FILTER_MEMORY_2 * sizeof(short));

    return output_frames;
}

#if 0
int process_src_48_to_8_stereo_to_mono(src_48_to_8 *s, short *output, short *input, int input_frames)
{
    signed short *input_samples = 0;
    signed short *poly_start = 0;
    signed short *output_samples = 0;
    signed int result = 0;
    int intermediate_frames = input_frames / 6;
    int output_frames = input_frames / 12;
    int start_point = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    /* polyphase filter from 48kHz to 16kHz */

    /* first process the filter memory */
    start_point = 0;
    for (i = 0; i < 5; i++)
    {
      result = 0;

      for (j = start_point, k = 0; j < (FILTER_LENGTH_1 - 1) * 2; j += 2, k++)
      {
	result += (int)(s->filter_memory_1[j]) * filter_coeffs_1[k];
      }

      for (j = 0; j < (start_point + 1) * 2; j += 2, k++)
      {
	result += (int)(input[j]) * filter_coeffs_1[k];
      }

      temp_buffer[i] = (short)((result + 16384) >> 15);

      start_point += DOWNSAMPLING_FACTOR_1 * 2;
    }

    output_samples = &temp_buffer[5];
    poly_start = &input[0];

    /* then process the rest of the input buffer */
    for (i = 0; i < intermediate_frames - 5; i++)
    {
      result = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_1[j];
	input_samples++;
      }

      *output_samples++ = (short)((result + 16384) >> 15);
      poly_start += DOWNSAMPLING_FACTOR_1 * 2;
    }

    /* copy filter memory to the beginning of scratch buffer */
    memcpy((void*)(&(s->filter_memory_1[0])),
	   (void*)(&(input[465 * 2])),
	   2 * 15 * sizeof(short));

    /* polyphase filter from 16kHz to 8kHz */

    /* first process the filter memory */
    start_point = 0;
    for (i = 0; i < 40; i++)
    {
      result = 0;

      for (j = start_point, k = 0; j < FILTER_LENGTH_2 - 1; j++, k++)
      {
	result += (int)(s->filter_memory_2[j]) * filter_coeffs_2[k];
      }

      for (j = 0; j < start_point + 1; j++, k++)
      {
	result += (int)(temp_buffer[j]) * filter_coeffs_2[k];
      }

      output[i] = (short)((result + 16384) >> 15);
      start_point += DOWNSAMPLING_FACTOR_2;
    }

    output_samples = &output[40];
    poly_start = &temp_buffer[1];

    /* then process the rest of the temp buffer */
    for (i = 0; i < 40; i++)
    {
      result = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_2[j];
      }

      *output_samples++ = (short)((result + 16384) >> 15);
      poly_start += DOWNSAMPLING_FACTOR_2;
    }

    /* copy filter memory */
    memcpy((void*)(&(s->filter_memory_2[0])),
           (void*)(&(temp_buffer[81])),
           79 * sizeof(short));

    return output_frames;
}

int process_src_48_to_8(src_48_to_8 *s, short *output, short *input, int input_frames)
{
    signed short *input_samples = 0;
    signed short *poly_start = 0;
    signed short *output_samples = 0;
    signed int result = 0;
    int intermediate_frames = (input_frames/3);
    int output_frames = (input_frames/6);
    int i = 0;
    int j = 0;

    /* copy input to scratch buffer to make looping easier */
    memcpy((void*)(&(s->input_temp_buffer_1[15])),
	   input,
	   input_frames * sizeof(short));

    input_samples = &s->input_temp_buffer_1[0];
    output_samples = &s->input_temp_buffer_2[79];
    poly_start = &s->input_temp_buffer_1[0];

    /* polyphase filter from 48kHz to 16kHz */
    for (i = 0; i < intermediate_frames; i++)
    {
      result = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_1[j];
      }
      *output_samples++ = (short)((result + 16384) >> 15);
      poly_start += DOWNSAMPLING_FACTOR_1;
    }

    /* copy filter memory to the beginning of scratch buffer */
    memcpy((void*)(&(s->input_temp_buffer_1[0])),
	   (void*)(&(s->input_temp_buffer_1[input_frames])),
	   15 * sizeof(short));

    input_samples = &s->input_temp_buffer_2[0];
    output_samples = output;
    poly_start = &s->input_temp_buffer_2[0];

    /* polyphase filter from 16kHz to 8kHz */
    for (i = 0; i < output_frames; i++)
    {
      result = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_2[j];
      }
      *output_samples++ = (short)((result + 16384) >> 15);
      poly_start += DOWNSAMPLING_FACTOR_2;
    }

    /* copy filter memory to the beginning of scratch buffer */
    memcpy((void*)(&(s->input_temp_buffer_2[0])),
           (void*)(&(s->input_temp_buffer_2[intermediate_frames])),
           79 * sizeof(short));

    return output_frames;
}


int process_src_48_to_8_stereo_to_mono(src_48_to_8 *s, short *output, short *input, int input_frames)
{
    signed short *input_samples = 0;
    signed short *poly_start = 0;
    signed short *output_samples = 0;
    signed int result = 0;
    int intermediate_frames = (input_frames/3);
    int output_frames = (input_frames/6);
    int i = 0;
    int j = 0;

    /* copy only the interleaved 2nd channel */
    for (i = 0; i < input_frames; i++)
    {
      s->input_temp_buffer_1[15+i] = input[i*2];
    }

    input_samples = &s->input_temp_buffer_1[0];
    output_samples = &s->input_temp_buffer_2[79];
    poly_start = &s->input_temp_buffer_1[0];

    /* polyphase filter from 48kHz to 16kHz */
    for (i = 0; i < intermediate_frames; i++)
    {
      result = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_1[j];
      }
      *output_samples++ = (short)((result + 16384) >> 15);
      poly_start += DOWNSAMPLING_FACTOR_1;
    }

    /* copy filter memory to the beginning of scratch buffer */
    memcpy((void*)(&(s->input_temp_buffer_1[0])),
	   (void*)(&(s->input_temp_buffer_1[input_frames])),
	   15 * sizeof(short));

    input_samples = &s->input_temp_buffer_2[0];
    output_samples = output;
    poly_start = &s->input_temp_buffer_2[0];

    /* polyphase filter from 16kHz to 8kHz */
    for (i = 0; i < output_frames; i++)
    {
      result = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result += (int)(*input_samples++) * filter_coeffs_2[j];
      }
      *output_samples++ = (short)((result + 16384) >> 15);
      poly_start += DOWNSAMPLING_FACTOR_2;
    }

    /* copy filter memory to the beginning of scratch buffer */
    memcpy((void*)(&(s->input_temp_buffer_2[0])),
           (void*)(&(s->input_temp_buffer_2[intermediate_frames])),
           79 * sizeof(short));

    return output_frames;
}
#endif
