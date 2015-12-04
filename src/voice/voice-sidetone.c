#include "module-voice-userdata.h"
#include "voice-sidetone.h"
#include "voice-util.h"

#include <stdio.h>

int voice_limit_sidetone(int val)
{
    int st = val & ~(val >> 31);
    if (st >= 0x8000)
    {
        st = 0x8000;
    }
    return st;
}

void voice_update_sidetone_gain(int16_t gain)
{
    assert(0 && "TODO voice_update_sidetone_gain address 0x0001BA3C");
}

void sidetone_write_parameters(struct userdata *u)
{
    assert(0 && "TODO sidetone_write_parameters address 0x0001BC18");
}

void voice_update_shc(struct userdata *u, int tone)
{
    assert(0 && "TODO voice_update_shc address 0x0001BF20");
}

void voice_turn_sidetone_down(void)
{
  FILE *fp;

  fp = fopen("/sys/devices/platform/omap-mcbsp.2/st_ch0gain", "w");
  if (fp)
  {
    fprintf(fp, "%d", 0);
    fclose(fp);
  }

  fp = fopen("/sys/devices/platform/omap-mcbsp.2/st_ch1gain", "w");
  if (fp)
  {
    fprintf(fp, "%d", 0);
    fclose(fp);
  }
}

void voice_enable_sidetone(struct userdata *u, pa_bool_t enable)
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
