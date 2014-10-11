#ifndef DRC_H
#define DRC_H

struct _mumdrc_userdata_t
{
  unsigned int DRCnMemRecs;
  EAP_MemoryRecord *DRCpMemRecs;
  EAP_MultibandDrcControl control;
  EAP_MultibandDrcInt32Handle drc;
};
typedef struct _mumdrc_userdata_t mumdrc_userdata_t;

void
mudrc_init(mumdrc_userdata_t *mumdrc, const int blockSize,
           const float sampleRate);

#endif // DRC_H
