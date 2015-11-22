#ifndef _PROPLIST_NOKIA_H_
#define _PROPLIST_NOKIA_H_

/* Generic */
#define PA_NOKIA_PROP_AUDIO_MODE                    "x-maemo.mode"                          /* "ihf","hp","hs","hf","a2dp","hsp" */
#define PA_NOKIA_PROP_AUDIO_ACCESSORY_HWID          "x-maemo.accessory_hwid"                /* char */

#define PA_NOKIA_PROP_HOOKS_PTR                     "x-maemo.hooks_ptr"                     /* pa_hook* */

/* Voice module */
#define PA_NOKIA_PROP_AUDIO_CMT_UL_TIMING_ADVANCE   "x-maemo.cmt.ul_timing_advance"         /* int:usecs */
#define PA_NOKIA_PROP_AUDIO_ALT_MIXER_COMPENSATION  "x-maemo.alt_mixer_compensation"        /* int:dB */

#define PA_NOKIA_PROP_AUDIO_AEP_mB_STEPS            "x-maemo.audio_aep_mb_steps"            /* "-6000,-2500, ...*/
#define PA_NOKIA_PROP_AUDIO_SIDETONE_ENABLE         "x-maemo.sidetone.enable"               /* "true" , "false" */
#define PA_NOKIA_PROP_AUDIO_SIDETONE_GAIN_L         "x-maemo.sidetone.lgain"                /* int:*/
#define PA_NOKIA_PROP_AUDIO_SIDETONE_GAIN_R         "x-maemo.sidetone.rgain"                /* int:*/
#define PA_NOKIA_PROP_AUDIO_EAR_REF_PADDING         "x-maemo.ear_ref_padding"               /* int:*/

#endif /* _PROPLIST_NOKIA_H_ */
