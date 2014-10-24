#ifndef EAP_CLIP_H
#define EAP_CLIP_H

#include "eap_data_types.h"

inline static int32
EAP_Clip16(int32 input)
{
  if (input < EAP_INT16_MIN)
    return EAP_INT16_MIN;

  if (input > EAP_INT16_MAX)
    return EAP_INT16_MAX;

  return input;
}

#endif // EAP_CLIP_H
