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

#include "src-8-to-48.h"

#ifdef ARM_DSP
#include <dspfns.h>
#endif

#define FILTER_LENGTH_1 40
#define FILTER_MEMORY_1 39
#define FILTER_LENGTH_2 8
#define FILTER_MEMORY_2 7

struct src_8_to_48 {
    short filter_memory_1[FILTER_LENGTH_1];
    short filter_memory_2[FILTER_LENGTH_2];
};

static short temp_buffer[SRC_8_TO_48_MAX_INPUT_FRAMES * 2];

static const signed short filter_coeffs_1_A[] =
{
  -12, 8, -12, 17, -24, 35, -49, 67, -92, 125, 
  -167, 222, -291, 378, -484, 608, -741, 842, 
  -753, 732, -954, 755, -742, 523, -292, -83,
  554, -1168, 1917, -2806, 3801, -4828, 5703,
  -6039, 5045, -1170, -8172, 23119, 15362, 1128
};

static const signed short filter_coeffs_1_B[] =
{
  4, 8, -9, 14, -19, 27, -35, 46, -57, 68, -79,
  88, -91, 85, -62, 12, 83, -252, 522, -626,
  666, -1110, 1263, -1616, 1878, -2188, 2437,
  -2639, 2723, -2638, 2288, -1552, 262, 1780,
  -4754, 8568, -11910, 8672, 24284, 5923
};

static const signed short filter_coeffs_2_A[] =
{
  49, -397, 1717, -1850, -4402, 26226, 11172, 223
};

static const signed short filter_coeffs_2_B[] =
{
  9, -74, 114, 2228, -9191, 18871, 19363, 1418
};

static const signed short filter_coeffs_2_C[] =
{
  0, 66, -546, 3099, -7134, 6637, 25811, 4810
};

src_8_to_48 *alloc_src_8_to_48(void)
{
    src_8_to_48 *src = (src_8_to_48 *) malloc(sizeof(src_8_to_48));
    memset(src, 0, sizeof(*src));
    return src;
}

void free_src_8_to_48(src_8_to_48 *src)
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
{
  input = input < (-32768) ? (-32768) : input;
  input = input > 32767 ?  32767 : input;
  return input;
}
#endif
#endif

int process_src_8_to_48(src_8_to_48 *s,
			short *output,
			short *input,
			int input_frames)
{
    signed short *input_samples = 0;
    signed short *output_samples = 0;
    signed short *poly_start = 0;
    int intermediate_frames = 2 * input_frames;
    int output_frames = 6 * input_frames;
    int samples_to_process_1 = input_frames - FILTER_MEMORY_1;
    int samples_to_process_2 = intermediate_frames - FILTER_MEMORY_2;
    signed int result_A = 0;
    signed int result_B = 0;
    signed int result_C = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    /* low pass filtering 1st stage */

    output_samples = &temp_buffer[0];

    /* first process the filter memory */
    for (i = 0; i < FILTER_MEMORY_1; i++)
    {
      result_A = 0;
      result_B = 0;

      for (j = i, k = 0; j < FILTER_MEMORY_1; j++, k++)
      {
	result_A += (int)s->filter_memory_1[j] * filter_coeffs_1_A[k];
	result_B += (int)s->filter_memory_1[j] * filter_coeffs_1_B[k];
      }

      for (j = 0; j < i + 1; j++, k++)
      {
	result_A += (int)input[j] * filter_coeffs_1_A[k];
	result_B += (int)input[j] * filter_coeffs_1_B[k];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
#else
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
#endif
    }

    poly_start = &input[0];

    /* then process rest of the input */
    for (i = 0; i < samples_to_process_1 ; i++)
    {
      result_A = 0;
      result_B = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_1_A[j];
	result_B += (int)(*input_samples++) * filter_coeffs_1_B[j];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
#else
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
#endif
      poly_start++;
    }

    /* copy unused samples to filter_memory */
    memcpy((void*)(&(s->filter_memory_1[0])),
	   (void*)(&(input[samples_to_process_1])),
	   FILTER_MEMORY_1 * sizeof(short));

    /* low pass filtering 2st stage */

    output_samples = &output[0];

    /* first process the filter memory */
    for (i = 0; i < FILTER_MEMORY_2; i++)
    {
      result_A = 0;
      result_B = 0;
      result_C = 0;

      for (j = i, k = 0; j < FILTER_MEMORY_2; j++, k++)
      {
	result_A += (int)s->filter_memory_2[j] * filter_coeffs_2_A[k];
	result_B += (int)s->filter_memory_2[j] * filter_coeffs_2_B[k];
	result_C += (int)s->filter_memory_2[j] * filter_coeffs_2_C[k];
      }

      for (j = 0; j < i + 1; j++, k++)
      {
	result_A += (int)temp_buffer[j] * filter_coeffs_2_A[k];
	result_B += (int)temp_buffer[j] * filter_coeffs_2_B[k];
	result_C += (int)temp_buffer[j] * filter_coeffs_2_C[k];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_C + 16384) >> 15);
#else
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
      *output_samples++ = (short)((result_C + 16384) >> 15);
#endif
    }

    poly_start = &temp_buffer[0];

    /* then process rest of the input */
    for (i = 0; i < samples_to_process_2; i++)
    {
      result_A = 0;
      result_B = 0;
      result_C = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_2_A[j];
	result_B += (int)(*input_samples) * filter_coeffs_2_B[j];
	result_C += (int)(*input_samples++) * filter_coeffs_2_C[j];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_C + 16384) >> 15);
#else
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
      *output_samples++ = (short)((result_C + 16384) >> 15);
#endif
      poly_start++;
    }

    /* copy unused samples to filter_memory */
    memcpy((void*)(&(s->filter_memory_2[0])),
           (void*)(&(temp_buffer[samples_to_process_2])),
           FILTER_MEMORY_2 * sizeof(short));

    return output_frames;
}

int process_src_8_to_48_mono_to_stereo(src_8_to_48 *s,
				       short *output,
				       short *input,
				       int input_frames)

{
    signed short *input_samples = NULL;
    signed short *output_samples = NULL, *prev_output_samples = NULL;
    signed short *poly_start = NULL;
    int intermediate_frames = 2 * input_frames;
    int output_frames = 12 * input_frames;
    int samples_to_process_1 = input_frames - FILTER_MEMORY_1;
    int samples_to_process_2 = intermediate_frames - FILTER_MEMORY_2;
    signed int result_A = 0;
    signed int result_B = 0;
    signed int result_C = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    /* low pass filtering 1st stage */

    output_samples = &temp_buffer[0];

    /* first process the filter memory */
    for (i = 0; i < FILTER_MEMORY_1; i++)
    {
      result_A = 0;
      result_B = 0;

      for (j = i, k = 0; j < FILTER_MEMORY_1; j++, k++)
      {
	result_A += (int)s->filter_memory_1[j] * filter_coeffs_1_A[k];
	result_B += (int)s->filter_memory_1[j] * filter_coeffs_1_B[k];
      }

      for (j = 0; j < i + 1; j++, k++)
      {
	result_A += (int)input[j] * filter_coeffs_1_A[k];
	result_B += (int)input[j] * filter_coeffs_1_B[k];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
#else
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
#endif
    }

    poly_start = &input[0];

    /* then process rest of the input */
    for (i = 0; i < samples_to_process_1; i++)
    {
      result_A = 0;
      result_B = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_1_A[j];
	result_B += (int)(*input_samples++) * filter_coeffs_1_B[j];
      }

#ifdef USE_SATURATION
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
#else
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
#endif
      poly_start++;
    }

    /* copy unused samples to filter_memory */
    memcpy((void*)(&(s->filter_memory_1[0])),
	   (void*)(&(input[samples_to_process_1])),
	   FILTER_MEMORY_1 * sizeof(short));

    /* low pass filtering 2st stage */

    output_samples = &output[0];

    /* first process the filter memory */
    for (i = 0; i < FILTER_MEMORY_2; i++)
    {
      result_A = 0;
      result_B = 0;
      result_C = 0;

      for (j = i, k = 0; j < FILTER_MEMORY_2; j++, k++)
      {
	result_A += (int)s->filter_memory_2[j] * filter_coeffs_2_A[k];
	result_B += (int)s->filter_memory_2[j] * filter_coeffs_2_B[k];
	result_C += (int)s->filter_memory_2[j] * filter_coeffs_2_C[k];
      }

      for (j = 0; j < i + 1; j++, k++)
      {
	result_A += (int)temp_buffer[j] * filter_coeffs_2_A[k];
	result_B += (int)temp_buffer[j] * filter_coeffs_2_B[k];
	result_C += (int)temp_buffer[j] * filter_coeffs_2_C[k];
      }

#ifdef USE_SATURATION
      prev_output_samples = output_samples;
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)EAP_Clip16((result_C + 16384) >> 15);
      *output_samples++ = *prev_output_samples;
#else
      prev_output_samples = output_samples;
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)((result_B + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)((result_C + 16384) >> 15);
      *output_samples++ = *prev_output_samples;
#endif
    }

    poly_start = &temp_buffer[0];

    /* then process rest of the input */
    for (i = 0; i < samples_to_process_2; i++)
    {
      result_A = 0;
      result_B = 0;
      result_C = 0;
      input_samples = poly_start;

      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_2_A[j];
	result_B += (int)(*input_samples) * filter_coeffs_2_B[j];
	result_C += (int)(*input_samples++) * filter_coeffs_2_C[j];
      }

#ifdef USE_SATURATION
      prev_output_samples = output_samples;
      *output_samples++ = (short)EAP_Clip16((result_A + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)EAP_Clip16((result_B + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)EAP_Clip16((result_C + 16384) >> 15);
      *output_samples++ = *prev_output_samples;
#else
      prev_output_samples = output_samples;
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)((result_B + 16384) >> 15);
      *output_samples++ = *prev_output_samples;

      prev_output_samples = output_samples;
      *output_samples++ = (short)((result_C + 16384) >> 15);
      *output_samples++ = *prev_output_samples;
#endif
      poly_start++;
    }

    /* copy unused samples to filter_memory */
    memcpy((void*)(&(s->filter_memory_2[0])),
	   (void*)(&(temp_buffer[samples_to_process_2])),
           FILTER_MEMORY_2 * sizeof(short));

    return output_frames;
}


#if 0
int process_src_8_to_48(src_8_to_48 *s, short *output, short *input, int input_frames)
{
    signed short *input_samples = 0;
    signed short *output_samples = 0;
    signed int result_A = 0;
    signed int result_B = 0;
    signed int result_C = 0;
    signed short *poly_start = 0;
    int intermediate_frames = 2*input_frames;
    int output_frames = 6*input_frames;
    int i = 0;
    int j = 0;

    memcpy((void*)(&(s->input_temp_buffer_1[39])),
	   (void*)input,
	   input_frames * sizeof(short));

    poly_start = &s->input_temp_buffer_1[0];
    output_samples = &s->input_temp_buffer_2[7];

    /* low pass filtering */
    for (i = 0; i < input_frames; i++)
    {
      result_A = 0;
      result_B = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_1_A[j];
	result_B += (int)(*input_samples++) * filter_coeffs_1_B[j];
      }
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
      poly_start++;
    }

    memcpy((void*)(&(s->input_temp_buffer_1[0])),
	   (void*)(&(s->input_temp_buffer_1[input_frames])),
	   39 * sizeof(short));

    poly_start = &s->input_temp_buffer_2[0];
    output_samples = output;

    /* low pass filtering */
    for (i = 0; i < intermediate_frames; i++)
    {
      result_A = 0;
      result_B = 0;
      result_C = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_2_A[j];
	result_B += (int)(*input_samples) * filter_coeffs_2_B[j];
	result_C += (int)(*input_samples++) * filter_coeffs_2_C[j];
      }
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
      *output_samples++ = (short)((result_C + 16384) >> 15);
      poly_start++;
    }

    memcpy((void*)(&(s->input_temp_buffer_2[0])),
            (void*)(&(s->input_temp_buffer_2[intermediate_frames])),
           7 * sizeof(short));


    return output_frames;
}


int process_src_8_to_48_mono_to_stereo(src_8_to_48 *s, short *output, short *input, int input_frames)
{
    signed short *input_samples = 0;
    signed short *output_samples = 0;
    signed int result_A = 0;
    signed int result_B = 0;
    signed int result_C = 0;
    signed short *poly_start = 0;
    int i = 0;
    int j = 0;
    int intermediate_frames = 2*input_frames;
    int output_frames = 12*input_frames;

    memcpy((void*)(&(s->input_temp_buffer_1[39])),
	   (void*)input,
	   input_frames * sizeof(short));

    poly_start = &s->input_temp_buffer_1[0];
    output_samples = &s->input_temp_buffer_2[7];

    /* low pass filtering */
    for (i = 0; i < input_frames; i++)
    {
      result_A = 0;
      result_B = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_1; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_1_A[j];
	result_B += (int)(*input_samples++) * filter_coeffs_1_B[j];
      }
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
      poly_start++;
    }

    memcpy((void*)(&(s->input_temp_buffer_1[0])),
	   (void*)(&(s->input_temp_buffer_1[input_frames])),
	   39 * sizeof(short));

    poly_start = &s->input_temp_buffer_2[0];
    output_samples = output;

    /* low pass filtering */
    for (i = 0; i < intermediate_frames; i++)
    {
      result_A = 0;
      result_B = 0;
      result_C = 0;
      input_samples = poly_start;
      for (j = 0; j < FILTER_LENGTH_2; j++)
      {
	result_A += (int)(*input_samples) * filter_coeffs_2_A[j];
	result_B += (int)(*input_samples) * filter_coeffs_2_B[j];
	result_C += (int)(*input_samples++) * filter_coeffs_2_C[j];
      }
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_A + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
      *output_samples++ = (short)((result_B + 16384) >> 15);
      *output_samples++ = (short)((result_C + 16384) >> 15);
      *output_samples++ = (short)((result_C + 16384) >> 15);
      poly_start++;
    }

    memcpy((void*)(&(s->input_temp_buffer_2[0])),
           (void*)(&(s->input_temp_buffer_2[intermediate_frames])),
           7 * sizeof(short));


    return output_frames;
}
#endif
