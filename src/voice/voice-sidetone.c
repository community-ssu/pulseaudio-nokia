#include "module-voice-userdata.h"
#include "voice-sidetone.h"
#include "voice-util.h"
#include "proplist-nokia.h"
#include "voice-cmtspeech.h"

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
    pa_sink *om_sink = voice_get_original_master_sink(u);
    if (om_sink)
    {
        if (voice_pa_proplist_get_bool(om_sink->proplist,PA_NOKIA_PROP_AUDIO_SIDETONE_ENABLE))
        {
            u->sidetone_enable = TRUE;
            FILE *fp = fopen("/sys/devices/platform/omap-mcbsp.2/st_taps", "w");
            if (fp)
            {
                fwrite(
                  "17181, 10037, 2433, -143, -700, -1200, -1016, -1031, -866, -750, -703, -605, -629, -601, -637, -660, -662, -69"
                  "1, -673, -678, -652, -625, -606, -576, -572, -555, -556, -557, -546, -544, -531, -521, -499, -479, -465, -443,"
                  " -434, -421, -410, -402, -389, -377, -361, -348, -334, -321, -309, -293, -281, -268, -257, -244, -229, -218, -"
                  "208, -200, -191, -180, -169, -157, -145, -133, -124, -118, -112, -104, -97, -90, -81, -73, -66, -59, -53, -48,"
                  " -44, -40, -36, -32, -27, -23, -20, -17, -13, -10, -8, -6, -4, -2, 0, 1, 2, 4, 5, 6, 7, 7, 9, 8, 9, 9, 9, 10, "
                  "9, 11, 10, 9, 11, 8, 11, 8, 9, 9, 6, 12, 4, 11, 6, 4, 14, -7, 22, -7, 1, 62, -206, 518, -611, 275",
                  1,647,fp);
                fclose(fp);
            }
            else
            {
                pa_log_error("Cannot open %s","/sys/devices/platform/omap-mcbsp.2/st_taps");
            }
            int32_t gain = 0;
            if(pa_atoi(voice_pa_proplist_gets(u->master_sink->proplist,PA_NOKIA_PROP_AUDIO_SIDETONE_GAIN_L),&gain))
            {
                sidetone_ch0_current_param = 0;
            }
            else
            {
                voice_limit_sidetone(gain);
                sidetone_ch0_current_param = gain;
            }
            if(pa_atoi(voice_pa_proplist_gets(u->master_sink->proplist,PA_NOKIA_PROP_AUDIO_SIDETONE_GAIN_R),&gain))
            {
                sidetone_ch1_current_param = 0;
            }
            else
            {
                voice_limit_sidetone(gain);
                sidetone_ch1_current_param = gain;
            }
            if (voice_cmt_ul_is_active_iothread(u) || (u->voip_source && 
                PA_SOURCE_IS_LINKED(u->voip_source->state) && 
                pa_source_used_by(u->voip_source)))
            {
                voice_enable_sidetone(u,TRUE);
                return;
            }
        }
        else
        {
            u->sidetone_enable = FALSE;
            voice_enable_sidetone(u,FALSE);
        }
    }
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
