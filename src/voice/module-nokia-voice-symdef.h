/*
 * This file is part of pulseaudio-nokia
 *
 * Copyright (C) 2008 Nokia Corporation. All rights reserved.
 *
 * Contact: Maemo Multimedia <multimedia@maemo.org>
 *
 * This software, including documentation, is protected by copyright
 * controlled by Nokia Corporation. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating,
 * any or all of this material requires the prior written consent of
 * Nokia Corporation. This material also contains confidential
 * information which may not be disclosed to others without the prior
 * written consent of Nokia.
 */
#ifndef foomodulevoicesymdeffoo
#define foomodulevoicesymdeffoo

#include <pulsecore/core.h>
#include <pulsecore/module.h>

#define pa__init module_nokia_voice_LTX_pa__init
#define pa__done module_nokia_voice_LTX_pa__done
#define pa__get_author module_nokia_voice_LTX_pa__get_author
#define pa__get_description module_nokia_voice_LTX_pa__get_description
#define pa__get_usage module_nokia_voice_LTX_pa__get_usage
#define pa__get_version module_nokia_voice_LTX_pa__get_version

int pa__init(struct pa_module*m);
void pa__done(struct pa_module*m);

const char* pa__get_author(void);
const char* pa__get_description(void);
const char* pa__get_usage(void);
const char* pa__get_version(void);

#endif
