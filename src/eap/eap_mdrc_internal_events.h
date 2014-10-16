#ifndef EAP_MDRC_INTERNAL_EVENTS_H
#define EAP_MDRC_INTERNAL_EVENTS_H

#include "eap_data_types.h"

typedef int32 EAP_MdrcInternalEventType;

struct _EAP_MdrcInternalEvent
{
  EAP_MdrcInternalEventType type;
};

typedef struct _EAP_MdrcInternalEvent EAP_MdrcInternalEvent;

#endif // EAP_MDRC_INTERNAL_EVENTS_H
