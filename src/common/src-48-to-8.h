#ifndef __SRC_48_TO_8_H__
#define __SRC_48_TO_8_H__

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

#define SRC_48_TO_8_MAX_INPUT_FRAMES 960

static inline
int output_frames_src_48_to_8(int input_frames) {
    if ((input_frames%6) != 0 || input_frames > SRC_48_TO_8_MAX_INPUT_FRAMES)
	return -1;
    return input_frames/6;
}

static inline
int output_frames_src_48_to_8_total(int input_frames) {
    if ((input_frames%6) != 0)
	return -1;
    return input_frames/6;
}

struct src_48_to_8;
typedef struct src_48_to_8 src_48_to_8;

src_48_to_8 *alloc_src_48_to_8(void);

void free_src_48_to_8(src_48_to_8 *src);

int process_src_48_to_8(src_48_to_8 *src, short *output, short *input, int input_frames);

int process_src_48_to_8_stereo_to_mono(src_48_to_8 *src, short *output, short *input, int input_frames);

#endif /* __SRC_48_TO_8_H__ */
