#ifndef EAP_MULTIBAND_DRC_INT32_H
#define EAP_MULTIBAND_DRC_INT32_H

struct _EAP_MultibandDrcInt32_InitInfo
{
  int sampleRate;
  int bandCount;
  int companderLookahead;
  int limiterLookahead;
  int downSamplingFactor;
  int avgShift;
  int maxBlockSize;
};

typedef struct _EAP_MultibandDrcInt32_InitInfo EAP_MultibandDrcInt32_InitInfo;

int EAP_MultibandDrcInt32_MemoryRecordCount(EAP_MultibandDrcInt32_InitInfo *initInfo);

#endif // EAP_MULTIBAND_DRC_INT32_H
