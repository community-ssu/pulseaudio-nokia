#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>
#include <pulsecore/aupdate.h>
#include <pulse/xmalloc.h>
#include "xprot.h"
void xprot_change_ambient_temperature(xprot *xp, int temperature)
{
	xp->xprot_left_variables[0]->t_amb = temperature;
	xp->xprot_left_variables[1]->t_amb = temperature;
	if ((unsigned __int16)(xp->stereo - 1) <= 2u)
	{
		xp->xprot_right_variables[0]->t_amb = temperature;
		xp->xprot_right_variables[1]->t_amb = temperature;
	}
}

void xprot_change_mode(xprot *xp, int mode)
{
	xp->stereo = mode;
}

void xprot_temp_enable(xprot *xp, int enable)
{
	xp->temp_limit = enable;
	pa_log_debug("Xprot temperature limiter: %d",enable);
}

void xprot_displ_enable(xprot *xp, int enable)
{
	xp->displ_limit = enable;
	pa_log_debug("Xprot displacement limiter: %d",enable);
}

void xprot_change_volume(xprot *xp, int16_t q15_vol_L, int16_t q15_vol_R)
{
	xp->xprot_left_variables[0]->lin_vol = q15_vol_L;
	xp->xprot_left_variables[1]->lin_vol = q15_vol_L;
	pa_log_debug("Xprot left volume: %d",q15_vol_L);
	if ((unsigned __int16)(xp->stereo - 1) <= 2u)
	{
		xp->xprot_right_variables[0]->lin_vol = q15_vol_R;
		xp->xprot_right_variables[1]->lin_vol = q15_vol_R;
		pa_log_debug("Xprot right volume: %d",q15_vol_R);
	}
}

void xprot_process_stereo_srcdst(xprot *xp, int16 *src_dst_left, int16 *src_dst_right, int16 length)
{
	int16 frame_length = xp->xprot_left_const->frame_length;
	unsigned int i1 = pa_aupdate_read_begin(xp->a[0]);
	unsigned int i2 = pa_aupdate_read_begin(xp->a[1]);
	pa_assert((length % frame_length) == 0);
	pa_assert(frame_length == xp->xprot_right_const->frame_length);
	if (length > 0)
	{
		int len = frame_length;
		int pos = 0;
		do
		{
			if ((unsigned __int16)(xp->stereo - 1) > 1u)
			{
				a_xprot_func(xp->xprot_left_variables[i1],xp->xprot_left_fixed[i1],src_dst_left + pos,xp->temp_limit,xp->displ_limit);
				if (xp->stereo == 3)
				{
					 a_xprot_func(xp->xprot_right_variables[i2],xp->xprot_right_fixed[i2],src_dst_right + pos,xp->temp_limit,xp->displ_limit);
				}
			}
			else
			{
				a_xprot_func_s(xp->xprot_left_variables[i1],xp->xprot_left_fixed[i1],xp->xprot_right_variables[i2],xp->xprot_right_fixed[i2],src_dst_left + pos,src_dst_right + pos,xp->temp_limit,xp->displ_limit);
			}
			len += frame_length;
			pos += 2 * frame_length;
		}
		while (length > (len - frame_length);
	}
	pa_aupdate_read_end(xp->a[0]);
	pa_aupdate_read_end(xp->a[1]);
}

void xprot_process_stereo(xprot *xp, int16 *src_left, int16 *src_right, int16 *dst_left, int16 *dst_right, int16 length)
{
	xprot_process_stereo_srcdst(xp, src_left, src_right, length);
	memcpy(dst_left, src_left, 4 * length);
	memcpy(dst_right, src_right, 4 * length);
}

void xprot_free(xprot *xp)
{
	pa_xfree(xp->xprot_right_const);
	pa_xfree(xp->xprot_left_const);
	pa_aupdate_free(xp->a[0]);
	pa_xfree(xp->xprot_right_fixed[0]);
	pa_xfree(xp->xprot_right_variables[0]);
	pa_xfree(xp->xprot_left_fixed[0]);
	pa_xfree(xp->xprot_left_variables[0]);
	pa_aupdate_free(xp->a[1]);
	pa_xfree(xp->xprot_right_fixed[1]);
	pa_xfree(xp->xprot_right_variables[1]);
	pa_xfree(xp->xprot_left_fixed[1]);
	pa_xfree(xp->xprot_left_variables[1]);
	pa_xfree(xp);
}

void xprot_change_params(xprot *xp, void *parameters, size_t length, int channel)
{
	pa_assert(length == sizeof(XPROT_Constant));
	pa_assert(channel == 0 || channel == 1);
	int i = pa_aupdate_write_begin(xp->a[channel]);
	XPROT_Constant *xprot_constant;
	XPROT_Variable *xprot_var;
	XPROT_Fixed *xprot_fix;
	if (channel)
	{
		xprot_constant = xp->xprot_right_const;
		xprot_var = xp->xprot_right_variables[i];
		xprot_fix = xp->xprot_right_fixed[i];
	}
	else
	{
		xprot_constant = xp->xprot_left_const;
		xprot_var = xp->xprot_left_variables[i];
		xprot_fix = xp->xprot_left_fixed[i];
	}
	memcpy(xprot_constant, parameters, sizeof(XPROT_Constant));
	a_xprot_init(xprot_var, xprot_fix, xprot_constant);
	pa_aupdate_write_end(xp->a[channel]);
	pa_log_debug"Xprot parameters updated (channel: %d) (set: %d)",channel,i);
}

xprot *xprot_new()
{
	xprot *xp = (xprot *)pa_xmalloc(sizeof(xprot));
	xp->xprot_left_const = (XPROT_Constant *)pa_xmalloc(sizeof(XPROT_Constant));
	xp->xprot_right_const = (XPROT_Constant *)pa_xmalloc(sizeof(XPROT_Constant));
	xp->a[0] = pa_aupdate_new();
	xp->xprot_left_variables[0] = (XPROT_Variable *)pa_xmalloc(sizeof(XPROT_Variable));
	xp->xprot_left_fixed[0] = (XPROT_Fixed *)pa_xmalloc(sizeof(XPROT_Fixed));
	xp->xprot_right_variables[0] = (XPROT_Variable *)pa_xmalloc(sizeof(XPROT_Variable));
	xp->xprot_right_fixed[0] = (XPROT_Fixed *)pa_xmalloc(sizeof(XPROT_Fixed));
	xp->a[1] = pa_aupdate_new();
	xp->xprot_left_variables[1] = (XPROT_Variable *)pa_xmalloc(sizeof(XPROT_Variable));
	xp->xprot_left_fixed[1] = (XPROT_Fixed *)pa_xmalloc(sizeof(XPROT_Fixed));
	xp->xprot_right_variables[1] = (XPROT_Variable *)pa_xmalloc(sizeof(XPROT_Variable));
	xp->xprot_right_fixed[1] = (XPROT_Fixed *)pa_xmalloc(sizeof(XPROT_Fixed));
	xp->displ_limit = 1;
	xp->temp_limit = 1;
	xp->stereo = 3;
	xp->apssas = 0;
	xp->volume_level = 32767;
	xprot_change_volume(xp,32767,32767);
	pa_log_debug("Xprot created");
}
