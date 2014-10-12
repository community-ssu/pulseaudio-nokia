#ifndef EAP_DATA_TYPES_H
#define EAP_DATA_TYPES_H

#include <stdint.h>

typedef int16_t int16;
typedef int32_t int32;

#define EAP_INT16_MAX ((int16)32767)
#define EAP_INT16_MIN ((int16)(-32767 - 1))
#define EAP_INT24_MAX (8388607)
#define EAP_INT24_MIN (-8388607 - 1)
#define EAP_INT32_MAX (2147483647)
#define EAP_INT32_MIN (-2147483647 - 1)

#endif // EAP_DATA_TYPES_H
