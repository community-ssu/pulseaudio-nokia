#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>
#include <pulse/xmalloc.h>
#include <string.h>
#include "xprot.h"
void xprot_change_ambient_temperature(xprot *xp, int temperature)
{
	xp->xprot_left_channel->variable->t_amb = temperature;
	xp->xprot_right_channel->variable->t_amb = temperature;
}

/*void xprot_change_mode(xprot *xp, int mode)
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
}*/

void xprot_change_volume(xprot *xp, int16_t q15_vol_L, int16_t q15_vol_R)
{
	xp->xprot_left_variable->lin_vol = q15_vol_L;
	if ((unsigned short)(xp->stereo - 1) <= 2u) {
		xp->xprot_right_variable->lin_vol = q15_vol_R;
	}
}

void xprot_process_stereo_srcdst(xprot *xp, int16 *src_dst_left, int16 *src_dst_right, int16 length)
{
	if (length > 0) {
		int frame_length = xp->xprot_left_channel->constant->frame_length;
		int len = xp->xprot_left_channel->constant->frame_length;
		do {
			while ((unsigned short)(xp->stereo - 1) <= 1u) {
				a_xprot_func_s(xp->xprot_left_variable, xp->xprot_left_fixed, xp->xprot_right_variable, xp->xprot_right_fixed, src_dst_left, src_dst_right, xp->temp_limit, xp->displ_limit);
				src_dst_left += xp->xprot_left_channel->constant->frame_length;
				len += frame_length;
				src_dst_right += xp->xprot_left_channel->constant->frame_length;
				if (length <= (signed short) (len - frame_length))
					return;
			}
			a_xprot_func(xp->xprot_left_variable, xp->xprot_left_fixed, src_dst_left, xp->temp_limit, xp->displ_limit);
			if (xp->stereo == 3)
				a_xprot_func(xp->xprot_right_variable, xp->xprot_right_fixed, src_dst_right, xp->temp_limit, xp->displ_limit);
			src_dst_left += xp->xprot_left_channel->constant->frame_length;
			len += frame_length;
			src_dst_right += xp->xprot_left_channel->constant->frame_length;
		}
		while (length > (len - frame_length));
	}
}

void xprot_process_stereo(xprot *xp, int16 *src_left, int16 *src_right, int16 *dst_left, int16 *dst_right, int16 length)
{
	xprot_process_stereo_srcdst(xp, src_left, src_right, length);
	memcpy(dst_left, src_left, 4 * length);
	memcpy(dst_right, src_right, 4 * length);
}

void xprot_free(xprot *xp)
{
	pa_xfree(xp->xprot_right_channel);
	pa_xfree(xp->xprot_left_channel);
	pa_xfree(xp->xprot_right_fixed);
	pa_xfree(xp->xprot_right_variable);
	pa_xfree(xp->xprot_right_const);
	pa_xfree(xp->xprot_left_fixed);
	pa_xfree(xp->xprot_left_variable);
	pa_xfree(xp->xprot_left_const);
	pa_xfree(xp);
}

void xprot_change_params(xprot *xp, void *parameters, size_t length, int channel)
{
	xprot_channel *c;
	if (channel)
	{
		c = xp->xprot_right_channel;
	}
	else
	{
		c = xp->xprot_left_channel;
	}
	memcpy(c->constant, parameters, sizeof(parameters));
	a_xprot_init(c->variable, c->fixed, c->constant);
}

xprot *xprot_new(void)
{
	xprot *xp = (xprot *)pa_xmalloc(sizeof(xprot));
	if (!xp)
	{
		return 0;
	}
	xp->xprot_left_const = (XPROT_Constant *)pa_xmalloc(sizeof(XPROT_Constant));
	if (xp->xprot_left_const)
	{
		xp->xprot_left_variable = (XPROT_Variable *) pa_xmalloc(sizeof(XPROT_Variable));
		if (xp->xprot_left_variable)
		{
			xp->xprot_left_fixed = (XPROT_Fixed *) pa_xmalloc(sizeof(XPROT_Fixed));
			if (xp->xprot_left_fixed)
			{
				xp->xprot_right_const = (XPROT_Constant *) pa_xmalloc(sizeof(XPROT_Constant));
				if (xp->xprot_right_const)
				{
					xp->xprot_right_variable = (XPROT_Variable *) pa_xmalloc(sizeof(XPROT_Variable));
					if (xp->xprot_right_variable)
					{
						xp->xprot_right_fixed = (XPROT_Fixed *) pa_xmalloc(sizeof(XPROT_Fixed));
						if (xp->xprot_right_fixed)
						{
							xp->xprot_left_channel = (xprot_channel *) pa_xmalloc(sizeof(xprot_channel));
							if (xp->xprot_left_channel)
							{
								xp->xprot_right_channel = (xprot_channel *) pa_xmalloc(sizeof(xprot_channel));
								if (xp->xprot_right_channel)
								{
									xp->xprot_left_channel->variable = xp->xprot_left_variable;
									xp->xprot_right_channel->variable = xp->xprot_right_variable;
									xp->stereo = 3;
									xp->displ_limit = 1;
									xp->temp_limit = 1;
									xp->apssas = 0;
									xp->volume_level = 32767;
									xprot_change_volume(xp, 32767, 32767);
									xp->xprot_left_channel->fixed = xp->xprot_left_fixed;
									xp->xprot_left_channel->constant = xp->xprot_left_const;
									xp->xprot_right_channel->fixed = xp->xprot_right_fixed;
									xp->xprot_right_channel->constant = xp->xprot_right_const;
									return xp;
								}
								free(xp->xprot_left_channel);
							}
							free(xp->xprot_right_fixed);
						}
						free(xp->xprot_right_variable);
					}
					free(xp->xprot_right_const);
				}
				free(xp->xprot_left_fixed);
			}
			free(xp->xprot_left_variable);
		}
		free(xp->xprot_left_const);
	}
	free(xp);
	return 0;
}
