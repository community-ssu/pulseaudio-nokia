#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <pulsecore/modargs.h>
#include <pulsecore/sink-input.h>
#include <pulsecore/source-output.h>
#include <pulsecore/module.h>
#include <pulsecore/thread.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/semaphore.h>
#include <pulsecore/fdsem.h>
#include <pulsecore/dbus-shared.h>

#include <dbus/dbus.h>
#include "eq_fir.h"

void fir_eq_free(struct fir_eq *eq)
{
  //todo address 0x00023864
}

void fir_eq_process_stereo(struct fir_eq *eq, int16_t *src_left, int16_t *src_right, int16_t *dst_left, int16_t *dst_right, int32_t samples)
{
  //todo address 0x00023900
}

void fir_eq_change_params(struct fir_eq *eq, void *parameters, size_t length)
{
  //todo address 0x00023948
}

struct fir_eq *fir_eq_new(int sampling_frequency, int channels)
{
  //todo address 0x00023D28
  return 0;
}
