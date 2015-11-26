#include <pulse/rtclock.h>

#include "voice-temperature.h"
#include "voice-mainloop-handler.h"

void voice_temperature_update(struct userdata *u)
{
  u->xprot_watchdog = FALSE;

  if (u->input_task_active)
    xprot_change_ambient_temperature(u->xprot, u->ambient_temp);
  else if (u->master_sink)
  {
    u->input_task_active = TRUE;
    pa_asyncmsgq_post(pa_thread_mq_get()->outq, u->mainloop_handler,
                      VOICE_MAINLOOP_HANDLER_TEMPERATURE_START, 0, 0, 0, 0);
  }
}

#if 0
static void voice_temperature_read_cb(pa_mainloop_api *a, pa_io_event *e,
                                      int fd, pa_io_event_flags_t events,
                                      struct userdata *u)
{
    emsg_battery_info_reply msg;

    a->io_free(e);

    if (bmeipc_recv(request_fd, &msg, sizeof(msg)) == sizeof(msg))
    {
        u->ambient_temperature = msg.temp - 273;
        pa_log_debug("Ambient temperature updated: %d",u->ambient_temperature);
    }
    else
        pa_log_error("Temperature value response read failed");

    bme_disconnect();
    u->time_event = pa_core_rttime_new(u->core, pa_rtclock_now() + 5000000,
                                       (pa_time_event_cb_t)voice_temperature_timer_cb, u);
    pa_log_debug("Temperature read timer started: %d us", 5000000);
    u->xprot_watchdog = TRUE;

}

static void bme_write_ack_cb(pa_mainloop_api *a, pa_io_event *e, int fd,
                             pa_io_event_flags_t events, struct userdata *u)
{
    int status;

    a->io_free(e);

    if (bme_read(&status, 4) == -1)
    {
        pa_log_error("Temperature value request ack read failed");
        bme_disconnect();
        u->input_task_active = FALSE;
    }
    else
        a->io_new(a, bme_fd, PA_IO_EVENT_INPUT,
                  (pa_io_event_cb_t)voice_temperature_read_cb, u);
}
#endif

static void voice_temperature_timer_cb(pa_mainloop_api *a, pa_time_event *e,
                                       pa_usec_t t, void *userdata)
{
    struct userdata *u = userdata;

    /* remove the assert and #if 0 once libbmeipc-dev can be installed in SB */
    pa_assert(0);
    a->time_free(e);
#if 0
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
#endif
}

static void voice_temperature_read_timer_start(struct userdata *u, pa_usec_t t)
{
    pa_mainloop_api *mainloop = u->core->mainloop;

    mainloop->rtclock_time_new(mainloop, pa_rtclock_now() + t,
                               voice_temperature_timer_cb,
                               NULL);
    pa_log_debug("Temperature read timer started: %lld us",t);
}

void voice_temperature_start(struct userdata *u)
{
    voice_temperature_read_timer_start(u, 40000);
    u->xprot_watchdog = TRUE;
}
