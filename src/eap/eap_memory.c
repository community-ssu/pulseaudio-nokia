#include <assert.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#include <string.h>
#endif

#include "eap_memory.h"

static int allocErrorCounter = -1;

static void Memory_Free(EAP_MemoryRecord *memRec, int memRecCount)
{
  int i;

  for ( i = 0; i < memRecCount; ++i )
  {
    if ( memRec[i].freeThisBlock )
    {
      free(memRec[i].base);
      memRec[i].base = 0;
      memRec[i].freeThisBlock = 0;
    }
  }
}

void EAP_Memory_Free(EAP_MemoryRecord *memRec, int memRecCount)
{
  Memory_Free(memRec, memRecCount);
}

int EAP_Memory_Alloc(EAP_MemoryRecord *memRec, int memRecCount,
                     void *scratchBuffer, unsigned int scratchBufferSize)
{
  unsigned int p, q;
  unsigned int alignment;
  int rv = 0;
  int i;
  int memoryAllocationErrorOccurred;

  p = 0;
  memoryAllocationErrorOccurred = 0;

  for (i = 0; i < memRecCount; i++)
  {
    EAP_MemoryType memRecType = memRec[i].type;
    assert(memRecType == EAP_MEMORY_PERSISTENT || memRecType == EAP_MEMORY_SCRATCH);
  }

  for (i = 0; i < memRecCount; i++)
  {
    if (memRec[i].type && (memRec[i].type != EAP_MEMORY_SCRATCH || scratchBuffer))
    {
      if (memRec[i].alignment)
        alignment = memRec[i].alignment;
      else
        alignment = 32;

      assert(memRec[i].type == EAP_MEMORY_SCRATCH && scratchBuffer != NULL);

      q = p + alignment - 1 - (p + alignment - 1) % alignment;
      memRec[i].base = (char *)scratchBuffer + q;
      p = memRec[i].size + q;

      if (p > scratchBufferSize)
      {
        memRec[i].base = 0;
        memoryAllocationErrorOccurred = 1;
        break;
      }

      memRec[i].freeThisBlock = 0;
    }
    else
    {
      if (allocErrorCounter < 0 || (--allocErrorCounter, allocErrorCounter != -1))
      {
        if (memRec[i].type == EAP_MEMORY_SCRATCH)
          memRec[i].base = malloc(memRec[i].size);
        else
          memRec[i].base = calloc(memRec[i].size, 1u);
      }
      else
        memRec[i].base = 0;

      if (!memRec[i].base)
      {
        memoryAllocationErrorOccurred = 1;
        break;
      }

      memRec[i].freeThisBlock = 1;
    }
  }

  assert(memoryAllocationErrorOccurred == (i < memRecCount));

  while (i < memRecCount)
  {
    memRec[i].freeThisBlock = 1;
    memRec[i++].base = 0;
  }

  if (memoryAllocationErrorOccurred)
  {
    Memory_Free(memRec, memRecCount);
    rv = 1;
  }

  return rv;
}

void
EAP_MemsetBuff_filterbank_Int32(int32 *ptr_left, int32 *ptr_right)
{
#ifdef __ARM_NEON__
  int i = 240;
  int32x4_t zero = {0,};

  for (i = 0; i < 240; i++, ptr_left += 8, ptr_right += 8)
  {
    vst1q_s32(ptr_left, zero);
    vst1q_s32(ptr_right, zero);
    vst1q_s32(ptr_left + 4, zero);
    vst1q_s32(ptr_right + 4, zero);
  }
#else
  memset(ptr_left, 0, 240 * 8 * sizeof(int32));
  memset(ptr_right, 0, 240 * 8 * sizeof(int32))
#endif
}

size_t
EAP_Memory_ScratchNeed(const EAP_MemoryRecord *memRec, int memRecCount)
{
  unsigned int alignment;
  int i;
  size_t scratchOffset;

  scratchOffset = 0;

  for (i = 0; i < memRecCount; i ++)
  {
    if (memRec[i].type == EAP_MEMORY_SCRATCH)
    {
      if (memRec[i].alignment)
        alignment = memRec[i].alignment;
      else
        alignment = 32;

      scratchOffset = memRec[i].size + scratchOffset + alignment - 1 -
                                    (scratchOffset + alignment - 1) % alignment;
    }
  }

  return scratchOffset;
}
