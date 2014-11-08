#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>
#include "xprot.h"
#include "dsp.h"


static int
fexp(int32_t a, int32_t b) __attribute__ ( ( naked ) );

static int
fexp(int32_t a, int32_t b)
{
  int32_t tmp1, tmp2;

  __asm__ __volatile (
        "MOV             %2, #0\n"
        "MOV             %1, %1,LSL#6\n"
        "QSUB16          %0, %2, %1\n"
        "SXTH            %1, %1\n"
        "SXTH            %0, %0\n"
        "QADD16          %0, %0, %1\n"
        "SXTH            %1, %0\n"
        "ANDS            %0, %1, #0x3F\n"
        "MOVEQ           %3, %2\n"

        /*
          Stock code was like that :

        "BEQ             l1%=\n"
        "SSAT            %3, #0x10, %0,LSL#9\n"
        "l1%=:\n"

          I guess we can save an instruction and a branch by using SSATNE
        */

        "SSATNE          %3, #0x10, %0,LSL#9\n"
        "QSUB16          %0, %2, %3\n"
        "MOV             %1, %1,ASR#6\n"
        "MOVW            %3, #0x7FFF\n"
        "MOVW            %2, #0x556C\n"
        "MOV             %3, %3,LSR %1\n"
        "MOVW            %1, #0x15EE\n"
        "SMULBB          %1, %0, %1\n"
        "SBFX            %1, %1, #0xF, #0x10\n"
        "QADD16          %1, %1, %2\n"
        "SMULBB          %0, %1, %0\n"
        "MOVW            %1, #0x7FBC\n"
        "QADD            %0, %0, %0\n"
        "MOV             %0, %0,ASR#16\n"
        "QADD16          %0, %0, %1\n"
        "SMULBB          %0, %3, %0\n"
        "QADD            %0, %0, %0\n"
        "MOV             %0, %0,ASR#16\n"
        "BX              LR"
        : "=r"(a), "=r"(b), "=r"(tmp1), "=r"(tmp2));
  return a;
}

static int32_t
dB100toLin(int32_t a, int16_t b)
{
  return fexp(__qdadd(32768, __smulbb(a, 3483)) >> 16, b);
}

void
a_xprot_func(XPROT_Variable *var, XPROT_Fixed *fix, int16 *in,
             int16 temp_limit, int16 displ_limit)
{
  pa_assert(var);
  pa_assert(fix);
  pa_assert(in);

  if (temp_limit > 0)
    a_xprot_temp_limiter(var, fix, in);

  if (displ_limit == 1)
  {
    int32 tmp;
    int16 x_peak = a_xprot_dp_filter(in, var->DP_IIR_w, var->DP_IIR_d,
                                     var->lin_vol, fix->frame_length);

    if ( var->x_peak < x_peak )
      var->x_peak = x_peak;

    tmp = __smulbb(fix->t_rav[0], var->x_peak);
    var->x_peak = __qadd(tmp, tmp) >> 16;

    a_xprot_coeff_calc(var, fix);

    a_xprot_lfsn_mono(in, var, fix->t_rav[2], fix->frame_length);

    if (fix->compute_nltm == 1)
    {
      a_xprot_dpvl_mono(in, var, fix->frame_length);

      var->u_d_sum = __qdadd(var->u_d_sum * fix->frame_average >> 15,
                             __smulbt(var->u_d_sum, fix->frame_average));

      var->x_d_sum = __qdadd(var->x_d_sum * fix->frame_average >> 15,
                             __smulbt(var->x_d_sum, fix->frame_average));
    }
  }

  if (temp_limit > 0)
    a_xprot_temp_predictor(var, fix, in);
}

void
a_xprot_func_s(XPROT_Variable *var_left, XPROT_Fixed *fix_left,
               XPROT_Variable *var_right, XPROT_Fixed *fix_right,
               int16 *in_left, int16 *in_right,
               int16 temp_limit, int16 displ_limit)
{
  pa_assert(var_left);
  pa_assert(var_right);
  pa_assert(fix_left);
  pa_assert(fix_right);
  pa_assert(in_left);
  pa_assert(in_right);
}

void
a_xprot_init(XPROT_Variable *var, XPROT_Fixed *fix, XPROT_Constant *cns)
{
  pa_assert(var);
  pa_assert(fix);
  pa_assert(cns);
}
