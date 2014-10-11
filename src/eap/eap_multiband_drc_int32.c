#include "eap_multiband_drc_int32.h"

int EAP_MultibandDrcInt32_MemoryRecordCount(EAP_MultibandDrcInt32_InitInfo *initInfo)
{
  return 4 * initInfo->bandCount + 16;
}

