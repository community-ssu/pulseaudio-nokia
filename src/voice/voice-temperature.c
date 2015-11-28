#include <em_isi.h>
#include <bmemsg.h>
#include <bmeipc.h>
#include <pulse/rtclock.h>

#include "voice-temperature.h"
#include "voice-mainloop-handler.h"
#include "xprot.h"
static void voice_temperature_timer_cb(pa_mainloop_api *a, pa_time_event *e,
                                       pa_usec_t t, void *userdata);
int32_t bme_fd;

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

static void voice_temperature_read_cb(pa_mainloop_api *a, pa_io_event *e,
                                      int fd, pa_io_event_flags_t events,
                                      struct userdata *u)
{
    struct emsg_battery_info_reply msg;

    u->core->mainloop->io_free(e);

    if (bme_read(&msg, sizeof(msg)) == sizeof(msg))
    {
        u->ambient_temp = msg.temp - 273;
        pa_log_debug("Ambient temperature updated: %d",u->ambient_temp);
    }
    else
        pa_log_error("Temperature value response read failed");

    bme_disconnect();
    u->core->mainloop->rtclock_time_new(u->core->mainloop, pa_rtclock_now() + 5000000,
                                       (pa_rtclock_event_cb_t)voice_temperature_timer_cb, u);
    pa_log_debug("Temperature read timer started: %d us", 5000000);
    u->xprot_watchdog = TRUE;

}

static void bme_write_ack_cb(pa_mainloop_api *a, pa_io_event *e, int fd,
                             pa_io_event_flags_t events, struct userdata *u)
{
    int status;

    u->core->mainloop->io_free(e);

    if (bme_read(&status, 4) == -1)
    {
        pa_log_error("Temperature value request ack read failed");
        bme_disconnect();
        u->input_task_active = FALSE;
    }
    else
        u->core->mainloop->io_new(u->core->mainloop, bme_fd, PA_IO_EVENT_INPUT,
                  (pa_io_event_cb_t)voice_temperature_read_cb, u);
}

static void voice_temperature_timer_cb(pa_mainloop_api *a, pa_time_event *e,
                                       pa_usec_t t, void *userdata)
{
    struct userdata *u = userdata;

    u->core->mainloop->time_free(e);
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
            u->core->mainloop->io_new(u->core->mainloop, bme_fd, PA_IO_EVENT_INPUT, (pa_io_event_cb_t)bme_write_ack_cb, u);
            struct emsg_battery_info_req msg;
            msg.type = EM_BATTERY_INFO_REQ;
            msg.subtype = 0;
            msg.flags = EM_BATTERY_TEMP;

            if (bme_write(&msg, sizeof(msg)) != sizeof(msg))
            {
                pa_log_error("Temperature value request write failed");
                bme_disconnect();
                u->input_task_active = 0;
            }
        }
    }
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
