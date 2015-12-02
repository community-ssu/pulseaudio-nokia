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

#include "equ.h"

void
iir_eq_process_mono(struct iir_eq *eq, int16_t *src, int16_t *dst,
                    int32_t samples)
{
  a_equ(src, eq->channel.coeff[0], eq->channel.delay[0], dst, samples,
        eq->scratch);
}

void
iir_eq_free(struct iir_eq *eq)
{
  if (eq->channel.coeff[1])
    free(eq->channel.coeff[1]);

  if (eq->channel.delay[1])
    free(eq->channel.delay[1]);

  free(eq->channel.coeff[0]);
  free(eq->channel.delay[0]);
  free(eq->scratch);
  free(eq);
}

struct iir_eq *
    iir_eq_new(int max_samples_per_frame, int channels)
{
  struct iir_eq *eq = pa_xnew(struct iir_eq, 1);

  if (!eq)
    return NULL;

  eq->scratch = (int32_t *)pa_xmalloc(channels * max_samples_per_frame * 4);

  if (eq->scratch)
  {
    eq->channel.delay[0] = (int32_t *)pa_xmalloc0(40);

    if (eq->channel.delay[0])
    {
      eq->channel.coeff[0] = (int16_t *)pa_xmalloc(76);

      if (eq->channel.coeff[0])
      {
        if (channels != 2)
        {
          eq->channel.coeff[1] = NULL;
          eq->channel.delay[1] = NULL;

          return eq;
        }

        eq->channel.delay[1] = (int32_t *)pa_xmalloc0(40);

        if (eq->channel.delay[1])
        {
          eq->channel.coeff[1] = (int16_t *)pa_xmalloc(76);

          if (eq->channel.coeff[1])
            return eq;

          free(eq->channel.delay[1]);
        }

        free(eq->channel.coeff[0]);
      }

      free(eq->channel.delay[0]);
    }

    free(eq->scratch);
  }

  free(eq);

  return NULL;
}

void
iir_eq_change_params(struct iir_eq *eq, const void *parameters, size_t length)
{
  const int16_t *coeffs = (const int16_t *)parameters;

  pa_assert(*(coeffs) + 1 <= 5);
  pa_assert(length == (((2 + 1) + (4 + 3)*(*(coeffs) + 1)) * sizeof(int16_t)));

  memcpy(eq->channel.coeff[0], parameters, length);
  memset(eq->channel.delay[0], 0, 4 * (*(coeffs) + 1) * sizeof(int16_t));

  if (eq->channel.coeff[1])
    memcpy(eq->channel.coeff[1], parameters, length);

  if (eq->channel.delay[1])
    memset(eq->channel.delay[1], 0, 4 * (*(coeffs) + 1) * sizeof(int16_t));
}
