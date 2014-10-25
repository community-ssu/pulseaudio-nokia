#ifndef EAP_MEMORY_H
#define EAP_MEMORY_H

#include <stdlib.h>
#include "eap_data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum _EAP_MemoryType
{
  EAP_MEMORY_PERSISTENT = 0x0,
  EAP_MEMORY_SCRATCH = 0x1
};
typedef enum _EAP_MemoryType EAP_MemoryType;

struct _EAP_MemoryRecord
{
  size_t size;
  size_t alignment;
  EAP_MemoryType type;
  int location;
  void *base;
  int freeThisBlock;
};

typedef struct _EAP_MemoryRecord EAP_MemoryRecord;

void
EAP_Memory_Free(EAP_MemoryRecord *memRec, int memRecCount);

int
EAP_Memory_Alloc(EAP_MemoryRecord *memRec, int memRecCount,
                     void *scratchBuffer, unsigned int scratchBufferSize);

size_t
EAP_Memory_ScratchNeed(const EAP_MemoryRecord *memRec, int memRecCount);

#ifdef __cplusplus
}
#endif

#endif // EAP_MEMORY_H
