#include "module-voice-userdata.h"
#include "voice-sidetone.h"
#include "voice-util.h"

#include <stdio.h>
int32_t sidetone_ch0_current_param = -1;
int32_t sidetone_ch1_current_param = -1;
static int32_t sidetone_gain_table[11] = {-4500,-2000,-1000,-500,-500,500,1000,2000,4000,5000,0};

int32_t voice_limit_sidetone(int32_t val)
{
    int st = val & ~(val >> 31);
    if (st >= 0x8000)
    {
        st = 0x8000;
    }
    return st;
}

void voice_update_sidetone_gain(int16_t gain_idx)
{
    int32_t gain = sidetone_gain_table[gain_idx];
    int32_t ch0_gain = voice_limit_sidetone(sidetone_ch0_current_param - gain);
    FILE *ch0_fp = fopen("/sys/devices/platform/omap-mcbsp.2/st_ch0gain", "w");
    if (ch0_fp)
    {
        fprintf(ch0_fp, "%d\n", ch0_gain);
        fclose(ch0_fp);
    }
    int32_t ch1_gain = voice_limit_sidetone(sidetone_ch1_current_param - gain);
    FILE *ch1_fp = fopen("/sys/devices/platform/omap-mcbsp.2/st_ch1gain", "w");
    if (ch1_fp)
    {
        fprintf(ch1_fp, "%d\n", ch1_gain);
        fclose(ch1_fp);
    }
}

void sidetone_write_parameters(struct userdata *u)
{
    assert(0 && "TODO sidetone_write_parameters address 0x0001BC18");
}

void voice_update_shc(struct userdata *u, int32_t tone)
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
