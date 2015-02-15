#include "module-voice-userdata.h"
#include "voice-sidetone.h"

void voice_enable_sidetone(struct userdata *u, bool enable)
{
  FILE *fp;
  const pa_cvolume *volume;

  fp = fopen("/sys/devices/platform/omap-mcbsp.2/st_enable", "w");
  if (fp)
  {
    if (enable)
    {
      volume = pa_sink_get_volume(u->master_sink, 0, 0);
      voice_update_sidetone_gain(
            voice_pa_vol_to_aep_step(u, volume->values[0]));
      fwrite("1\n", 1, 2, fp);
    }
    else
      fwrite("0\n", 1, 2, fp);

    fclose(fp);
  }
  else
    pa_log_error("Cannot open %s",
                 "/sys/devices/platform/omap-mcbsp.2/st_enable");
}
