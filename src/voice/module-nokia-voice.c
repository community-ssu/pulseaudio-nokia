#include "module-voice-userdata.h"
#include "module-nokia-voice-symdef.h"
#include "voice-sidetone.h"
#include "voice-util.h"

#include <pulsecore/modargs.h>
#include <pulsecore/namereg.h>

PA_MODULE_AUTHOR("Jyri Sarha");
PA_MODULE_DESCRIPTION("Nokia voice module");
PA_MODULE_USAGE("voice_sink_name=<name for the voice sink> "
                "voice_source_name=<name for the voice source> "
                "master_sink=<sink to connect to> "
                "master_source=<source to connect to> "
                "raw_sink=<name for raw sink> "
                "raw_source=<name for raw source> "
                "dbus_type=<system|session> "
                "max_hw_frag_size=<maximum fragment size of master sink and source in usecs>");

PA_MODULE_VERSION(PACKAGE_VERSION) ;

void pa__done(pa_module *m)
{
  struct userdata *u;
  pa_modargs *ma;

  u = m->userdata;
  if (u)
  {
    voice_shutdown_aep();
    voice_enable_sidetone(u, 0);
    voice_clear_up(u);
    fir_eq_free(u->wb_ear_iir_eq);
    iir_eq_free(u->nb_ear_iir_eq);
    iir_eq_free(u->wb_mic_iir_eq);
    iir_eq_free(u->nb_mic_iir_eq);
    xprot_free(u->xprot);
    ma = (pa_modargs *)u->modargs;

    if (ma)
      pa_modargs_free(ma);

    pa_xfree(u);
  }
}
