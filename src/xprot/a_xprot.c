#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/log.h>
#include "xprot.h"

void a_xprot_func(XPROT_Variable *var, XPROT_Fixed *fix, int16 *in1, int16 temp_limit, int16 displ_limit)
{
	pa_assert(var);
	pa_assert(fix);
	pa_assert(in1);
}

void a_xprot_func_s(XPROT_Variable *var_left, XPROT_Fixed *fix_left, XPROT_Variable *var_right, XPROT_Fixed *fix_right, int16 *in_left, int16 *in_right, int16 temp_limit, int16 displ_limit)
{
	pa_assert(var_left);
	pa_assert(var_right);
	pa_assert(fix_left);
	pa_assert(fix_right);
	pa_assert(in_left);
	pa_assert(in_right);
}

void a_xprot_init(XPROT_Variable *var, XPROT_Fixed *fix, XPROT_Constant *cns)
{
	pa_assert(var);
	pa_assert(fix);
	pa_assert(cns);
}
