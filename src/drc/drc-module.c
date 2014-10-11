#include <pulsecore/modargs.h>
#include <pulsecore/namereg.h>
#include <pulsecore/msgobject.h>
#include <pulsecore/macro.h>
#include <meego/algorithm-base.h>
#include <meego/optimize.h>
#include "module-nokia-algorithm-drc.h"

PA_MODULE_AUTHOR("abc");
PA_MODULE_DESCRIPTION("Nokia proprietary drc algorithm module");
PA_MODULE_USAGE("");
PA_MODULE_VERSION(PACKAGE_VERSION);
pa_hook_result_t mumdrc_stereo_cb(pa_core *c, meego_algorithm_hook_data *data, userdata *u)
{
	pa_assert(c);
	pa_assert(data);
	pa_assert(data->channels == 2);
	pa_assert(data->channel[0].length == data->channel[1].length);
	pa_assert(u);
	pa_memblock *b1 = pa_memblock_new(c->mempool, 2 * data->channel[0].length);
	pa_memblock *b2 = pa_memblock_new(c->mempool, 2 * data->channel[8].length);
	short *data1 = (short *) pa_memblock_acquire(data->channel[0].memblock) + data->channel[0].index / sizeof(short);
	short *data2 = (short *) pa_memblock_acquire(data->channel[1].memblock) + data->channel[1].index / sizeof(short);
	int32_t *data3 = (int32_t *) pa_memblock_acquire(b1);
	int32_t *data4 = (int32_t *) pa_memblock_acquire(b2);
	move_16bit_to_32bit(data3, data1, data->channel[0].length >> 1);
	move_16bit_to_32bit(data4, data2, data->channel[0].length >> 1);
	mumdrc_process(u->mumdrc_data, data3, data4, data3, data4, (2 * data->channel[0].length) >> 2);
	move_32bit_to_16bit(data1, data3, data->channel[0].length >> 1);
	move_32bit_to_16bit(data2, data4, data->channel[0].length >> 1);
	pa_memblock_release(b2);
	pa_memblock_release(b1);
	pa_memblock_release(data->channel[0].memblock);
	pa_memblock_release(data->channel[1].memblock);
	pa_memblock_unref(b1);
	pa_memblock_unref(b2);
	return 0;
}

pa_hook_result_t mumdrc_set_volume_cb(pa_core *c, pa_cvolume *mumdrc_volume, userdata *u)
{
	pa_assert(c);
	pa_assert(mumdrc_volume);
	pa_assert(u);
	u->mumdrc_volume = pa_sw_volume_to_dB(pa_cvolume_avg(mumdrc_volume));
	if (u->mumdrc_volume > -10.1f)
	{
		pa_log_debug("MuMDRC volume changing to %f", u->mumdrc_volume);
		set_drc_volume(u->mumdrc_data, u->mumdrc_volume);
	}
	volume_switch(u);
	return 0;
}

pa_hook_result_t mumdrc_parameter_cb(pa_core *c, meego_parameter_update_args *ua, userdata *u)
{
	pa_assert(ua);
	pa_assert(u);
	pa_bool_t enable;
	switch (ua->status)
	{
	case MEEGO_PARAM_DISABLE:
		enable = FALSE;
		break;
	case MEEGO_PARAM_ENABLE:
		enable = TRUE;
		break;
	case MEEGO_PARAM_UPDATE:
		pa_assert(ua->parameters);
		meego_algorithm_base_set_enabled(u->base, "mumdrc", 0);
		pa_proplist *proplist = pa_proplist_from_string((const char *)ua->parameters);
		IMUMDRC_Status *parameter;
		size_t length;
		if (!pa_proplist_get(proplist, "x-maemo.mumdrc.parameters", (const void **) &parameter, &length))
		{
			write_mumdrc_params(u->mumdrc_data, parameter, length);
		}
		pa_proplist_free(proplist);
		enable = true;
		break;
	default:
		pa_assert_not_reached();
	}
	u->mumdrc_enabled = enable;
	volume_switch(u);
	return 0;
}

pa_hook_result_t limiter_parameter_cb(pa_core *c, meego_parameter_update_args *ua, userdata *u)
{
	pa_assert(ua);
	pa_assert(u);
	if (ua->parameters)
	{
		meego_algorithm_base_set_enabled(u->base, "mumdrc", 0);
		IMUMDRC_Limiter_Status *parameter;
		size_t length;
		if (!pa_proplist_get(proplist, "x-maemo.limiter.parameters", (const void **) &parameter, &length))
		{
			write_limiter_params(u->mumdrc_data, parameter, length);
		}
		pa_proplist_free(proplist);
		volume_switch(u);
	}
	return 0;
}

void volume_switch(userdata *u)
{
	pa_assert(u);
	pa_Assert(u->base);
	meego_algorithm_hook_slot *slot = u->mumdrc_hook_slot;
	if (slot || (slot = meego_algorithm_base_get_hook_slot(u->base, "mumdrc"), (u->mumdrc_hook_slot = slot) != NULL))
	{
		pa_bool_t enabled = meego_algorithm_hook_slot_enabled(slot);
		pa_bool_t enable;
		if (u->mumdrc_enabled && u->mumdrc_volume > -10.1f)
		{
			enable = true;
		}
		else
		{
			enable = false;
		}
		if (enable != enabled)
		{
			meego_algorithm_hook_slot_set_enabled(u->mumdrc_hook_slot, enable);
		}
	}
	else
	{
		pa_log_error("Could not get mumdrc hook slot, unable to change processing enabled state.");
	}
}

meego_algorithm_callback_list algorithms[3] = { { mumdrc, 0, PA_HOOK_NORMAL, mumdrc_stereo_cb }, { volume, 0, PA_HOOK_NORMAL, mumdrc_set_volume_cb }, { 0, 0, 0, 0 } }
meego_algorithm_callback_list parameters[3] = { { mumdrc, 0, PA_HOOK_NORMAL, mumdrc_parameter_cb }, { limiter, 0, PA_HOOK_NORMAL, limiter_parameter_cb }, { 0, 0, 0, 0 } }
void pa__done(pa_module *m)
{
	meego_algorithm_base *b;
	userdata *u;
	b = m->userdata;
	if (b)
	{
		u = b->userdata;
		meego_algorithm_base_done(b);
		if (u)
		{
			if (u->mumdrc_data)
			{
				mumdrc_deinit(u->mumdrc_data);
			}
			pa_xfree(u);
		}
	}
}

int pa__init(pa_module *m)
{
	userdata *u;
	pa_assert(m);
	meego_algorithm_callback_list parameter_list[3];
	meego_algorithm_callback_list algorithm_list[3];
	memcpy(parameter_list, parameters, sizeof(parameter_list));
	memcpy(algorithm_list, algorithms, sizeof(algorithm_list));
	u = (userdata *) pa_xmaloc0(sizeof(userdata));
	u->base = meego_algorithm_base_init(m, 0, parameter_list, algorithm_list, u);
	if (u->base)
	{
		u->mumdrc_data = mumdrc_init(3840, 48000.0f);
		if (u->mumdrc_data)
		{
			u->mumdrc_volume = 0.0f;
			set_drc_volume(u->mumdrc_data, 0.0f);
			meego_algorithm_base_connect(u->base);
			meego_algorithm_base_set_enabled(u->base, "volume", TRUE);
			return 0;
		}
		else
		{
			pa_log_error("Failed to create drc.");
		}
	}
	else
	{
		pa_log_error("Failed to init algorithm base");
	}
	pa_xfree(u);
	return -1;
}
