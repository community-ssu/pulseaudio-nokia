#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>
#include <pulsecore/core.h>
#include <pulsecore/core-util.h>
#include <pulse/rtclock.h>
#include <pulse/mainloop.h>
#if 0
#include <bmemsg.h>
#include "xprot.h"
void voice_temperature_start(pa_mainloop_api *a, pa_defer_event *e, nokia_xprot_temperature *u);
void voice_temperature_timer_cb(pa_mainloop_api *a, pa_time_event *e, const timeval *t, nokia_xprot_temperature *u);
void bme_write_ack_cb(pa_mainloop_api *a, pa_io_event *e, int fd, pa_io_event_flags_t events, nokia_xprot_temperature *u);
void voice_temperature_read_cb(pa_mainloop_api *a, pa_io_event *e, int fd, pa_io_event_flags_t events, nokia_xprot_temperature *u);
int bme_fd;

void nokia_xprot_temperature_cancel_read_timer(nokia_xprot_temperature *u)
{
	if (u->time_event)
	{
		u->core->mainloop->time_free(u->time_event);
		pa_log_debug("Temperature read timer cancelled");
	}
	u->input_task_active = 0;
}

void nokia_xprot_temperature_init(nokia_xprot_temperature *t, pa_core *c, xprot *xprot_handle)
{
	pa_assert(t);
	pa_assert(c);
	t->core = c;
	t->xprot_handle = xprot_handle;
	t->input_task_active = 0;
	t->xprot_watchdog = 1;
}

void nokia_xprot_temperature_update(nokia_xprot_temperature *u)
{
	pa_assert(u);
	u->xprot_watchdog = 0;
	if (u ->input_task_active)
	{
		xprot_change_ambient_temperature(u->xprot_handle, u->ambient_temperature);
	}
	else
	{
		u->input_task_active = 1;
		u->core->mainloop->defer_new(u->core->mainloop, (pa_defer_event_cb_t)voice_temperature_start, u);
	}
}

void voice_temperature_start(pa_mainloop_api *a, pa_defer_event *e, nokia_xprot_temperature *u)
{
	pa_assert(u);
	a->defer_free(e);
	u->time_event = pa_core_rttime_new(u->core, pa_rtclock_now() + 40000, (pa_time_event_cb_t)voice_temperature_timer_cb, u);
	pa_log_debug("Temperature read timer started: %d us",40000);
	u->xprot_watchdog = 1;
}

void voice_temperature_timer_cb(pa_mainloop_api *a, pa_time_event *e, const timeval *t, nokia_xprot_temperature *u)
{
	u->time_event = NULL;
	a->time_free(e);
	if (u->xprot_watchdog)
	{
		u->input_task_active = 0;
		pa_log_debug("Xprot inactive, shutting down.");
	}
	else
	{
		bme_fd = bme_connect();
		if (bme_fd >= 0)
		{
			pa_make_fd_nonblock(bme_fd);
			a->io_new(a, bme_fd, PA_IO_EVENT_INPUT, (pa_io_event_cb_t)bme_write_ack_cb, u);
			emsg_battery_info_req msg;
			msg.type = EM_BATTERY_INFO_REQ;
			msg.subtype = 0;
			msg.flags = EM_BATTERY_TEMP;
			if (bme_write(bme_fd, &req, sizeof(msg)) != sizeof(msg))
			{
				pa_log_error("Temperature value request write failed");
				bme_disconnect();
				u->input_task_active = 0;
			}
		}
	}
}

void bme_write_ack_cb(pa_mainloop_api *a, pa_io_event *e, int fd, pa_io_event_flags_t events, nokia_xprot_temperature *u)
{
	a->io_free(e);
	int status;
	if (bme_read(&status, 4) == -1)
	{
		pa_log_error("Temperature value request ack read failed");
		bme_disconnect();
		u->input_task_active = 0;
	}
	else
	{
		a->io_new(a, bme_fd, PA_IO_EVENT_INPUT, (pa_io_event_cb_t)voice_temperature_read_cb, u);
	}
}

void voice_temperature_read_cb(pa_mainloop_api *a, pa_io_event *e, int fd, pa_io_event_flags_t events, nokia_xprot_temperature *u)
{
	a->io_free(e);
	emsg_battery_info_reply msg;
	if (bmeipc_recv(request_fd, &msg, sizeof(msg)) == sizeof(msg))
	{
		u->ambient_temperature = msg.temp - 273;
		pa_log_debug("Ambient temperature updated: %d",u->ambient_temperature);
	}
	else
	{
		pa_log_error("Temperature value response read failed");
	}
	bme_disconnect();
	u->time_event = pa_core_rttime_new(u->core, pa_rtclock_now() + 5000000, (pa_time_event_cb_t)voice_temperature_timer_cb, u);
	pa_log_debug("Temperature read timer started: %d us",5000000);
	u->xprot_watchdog = 1;
}
#endif
