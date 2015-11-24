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
#include "eq_iir.h"

void iir_eq_process_mono(struct iir_eq *eq, int16_t *src, int16_t *dst, int32_t samples)
{
  //todo address 0x0002365C
}

void iir_eq_free(struct iir_eq *eq)
{
  if (eq->channel.coeff[1])
  {
    free(eq->channel.coeff[1]);
  }
  if (eq->channel.delay[1])
  {
    free(eq->channel.delay[1]);
  }
  free(eq->channel.coeff[0]);
  free(eq->channel.delay[0]);
  free(eq->scratch);
  free(eq);
}

struct iir_eq *iir_eq_new(int max_samples_per_frame, int channels)
{
  struct iir_eq *i = pa_xnew(struct iir_eq, 1);
  if (!i)
  {
    return NULL;
  }
  i->scratch = (int32_t *)pa_xmalloc(channels * max_samples_per_frame * 4);
  if (i->scratch)
  {
    i->channel.delay[0] = (int32_t *)pa_xmalloc0(40);
    if (i->channel.delay[0])
    {
      i->channel.coeff[0] = (int16_t *)pa_xmalloc(76);
      if (i->channel.coeff[0])
      {
        if (channels != 2)
        {
          i->channel.coeff[1] = 0;
          i->channel.delay[1] = 0;
          return i;
        }
        i->channel.delay[1] = (int32_t *)pa_xmalloc0(40);
        if (i->channel.delay[1])
        {
          i->channel.coeff[1] = (int16_t *)pa_xmalloc(76);
          if (i->channel.coeff[1])
          {
            return i;
          }
          free(i->channel.delay[1]);
        }
        free(i->channel.coeff[0]);
      }
      free(i->channel.delay[0]);
    }
    free(i->scratch);
  }
  free(i);
  return 0;
}

void iir_eq_change_params(struct iir_eq *eq, void *parameters, size_t length)
{
  //todo address 0x000237CC
}
