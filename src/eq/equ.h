#ifndef __IIR_H__
#define __IIR_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void
a_equ(int16_t *src, int16_t *coeff, int32_t *delay, int16_t *dst,
      int32_t samples, int32_t *scratch);

#ifdef __cplusplus
}
#endif

#endif
