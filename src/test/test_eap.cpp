#include <assert.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <QByteArray>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#define PACKAGE pulseaudio

#include "../eap/eap_memory.h"
#include "../eap/eap_multiband_drc_int32.h"
#include "../eap/eap_multiband_drc_control_int32.h"

#include "../drc/drc.h"

#define timing(a) \
{ \
  clock_t start=clock(), diff; \
  int msec; \
  do { \
    a; \
  } \
  while(0); \
  diff = clock() - start; \
  msec = diff * 1000 / CLOCKS_PER_SEC; \
  printf("msecs: %d\n",msec); \
}

#define DEFAULT_CHANNELS     (1)
#define DEFAULT_SAMPLELENGTH (20) //ms
#define DEFAULT_SAMPLERATE   (48000)
int32 SAMPLES;
int32 *src;
int32 *dst;
int32 *dstn;

typedef void (*mudrc_init_f)(mumdrc_userdata_t *mumdrc, const int blockSize, const float sampleRate);
typedef void (*mudrc_process_f)(mumdrc_userdata_t *mudrc, int32 *dst_left, int32 *dst_right, int32 *src_left, int32 *src_right, const int samples);
typedef void (*mumdrc_write_parameters_f)(EAP_MultibandDrcInt32Handle handle,const void *data, size_t size);

#define check_equal_u(member) assert(stock->member == foss->member)
#define check_equal_control(member) assert(stock->control.member == foss->control.member)

mumdrc_userdata_t drc, drcn;

void compare(mumdrc_userdata_t *stock, mumdrc_userdata_t* foss)
{
  int i, j;

  /* mumdrc */
  check_equal_u(DRCnMemRecs);
  for (i = 0; i < (int)stock->DRCnMemRecs; i++)
  {
    check_equal_u(DRCpMemRecs[i].size);
    check_equal_u(DRCpMemRecs[i].alignment);
    check_equal_u(DRCpMemRecs[i].type);
    /*check_equal_u(DRCpMemRecs[i].location);*/
    check_equal_u(DRCpMemRecs[i].freeThisBlock);
  }

  /* control */
  check_equal_control(m_sampleRate);
  check_equal_control(m_bandCount);
  check_equal_control(m_downSamplingFactor);
  check_equal_control(m_companderLookahead);
  check_equal_control(m_limiterLookahead);
  check_equal_control(m_maxBlockSize);
  check_equal_control(m_oneOverFactor);

  assert(!memcmp(&stock->control.m_volume, &foss->control.m_volume,
                 sizeof(foss->control.m_volume)));

  for(i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i++)
    check_equal_control(m_curveSet[i].curveCount);

  for(i = 0; i < EAP_MDRC_MAX_BAND_COUNT; i++)
  {
    for (j = 0; j < stock->control.m_curveSet[i].curveCount; j++)
    {
      assert(!memcmp(&stock->control.m_curveSet[i].curves[j],
                     &foss->control.m_curveSet[i].curves[j],
                     sizeof(EAP_MdrcCompressionCurve)));
    }
  }

  check_equal_control(m_eqCount);
  for(i = 0; i < stock->control.m_eqCount; i++)
    assert(!memcmp(stock->control.m_eqCurves[i],
                   foss->control.m_eqCurves[i],
                   sizeof(foss->control.m_eqCurves[0]) * stock->control.m_bandCount));
}

int comp_lookahead_size()
{
  return drcn.DRCpMemRecs[MEM_COMPANDER_LOOKAHEAD1].size;
}

int filterbank_size()
{
  return drcn.DRCpMemRecs[MEM_FILTERBANK].size;
}

void compare_drc(EAP_MultibandDrcInt32 *stock, EAP_MultibandDrcInt32* foss)
{
  int i;

  for (i = 0; i < stock->bandCount; i ++)
    assert(!memcmp(&stock->attRelFilters[i], &foss->attRelFilters[i], sizeof(EAP_AttRelFilterInt32)));

  for (i = 0; i < stock->bandCount + 1; i ++)
    assert(!memcmp(&stock->avgFilters[i], &foss->avgFilters[i], sizeof(EAP_AverageAmplitudeInt32)));

  for (i = 0; i < stock->bandCount; i ++)
    assert(!memcmp(&stock->compressionCurves[i], &foss->compressionCurves[i], sizeof(EAP_CompressionCurveImplDataInt32)));

  assert(!memcmp(stock->filterbank, foss->filterbank, filterbank_size()));

  assert(!memcmp(&stock->gains, &foss->gains, sizeof(EAP_MdrcDelaysAndGainsInt32) - sizeof(stock->gains.m_memBuffers)));

  for (i = 0; i < 2 * (stock->bandCount + 1); i ++)
    assert(!memcmp(stock->gains.m_memBuffers[i], foss->gains.m_memBuffers[i], comp_lookahead_size()));

  assert(!memcmp(&stock->qmf, &foss->qmf, sizeof(EAP_QmfStereoInt32)));
}

extern "C" void _compare()
{
  int i;
  for (i = 0; i < SAMPLES; i ++)
  {
    if(dst[i] != dstn[i])
    {
      printf("left %d - [%d] [%d]\n", i, dst[i], dstn[i]);
      break;
    }

    if(dst[i + SAMPLES] != dstn[i + SAMPLES])
    {
      printf("right %d - [%d] [%d]\n", i, dst[i + SAMPLES], dstn[i + SAMPLES]);
      break;
    }
  }
  assert(!memcmp(dst, dstn, SAMPLES * 2));

  compare(&drcn, &drc);
  compare_drc((EAP_MultibandDrcInt32 *)drcn.drc, (EAP_MultibandDrcInt32 *)drc.drc);
}

int main(int argc, char *argv[])
{
  int i;
  int samplerate = DEFAULT_SAMPLERATE;
  int samplelength = DEFAULT_SAMPLELENGTH;
  int stereo = 1;
  int maxblocksize = samplerate * (stereo ? 2:1) * 2 * samplelength / 1000 ;

  FILE* fp = fopen("/opt/eap/bin/test.raw", "r"), *fpout, *fpoutn;
  assert(fp);
  fseek (fp, 0, SEEK_END);
  SAMPLES = ftell(fp);
  fseek (fp, 0, SEEK_SET);

  src = (int32 *)malloc(sizeof(int32) * SAMPLES);
  dst = (int32 *)calloc(sizeof(int32), SAMPLES);
  dstn = (int32 *)calloc(sizeof(int32), SAMPLES);

  SAMPLES /= 2 * sizeof(int32);

  printf("Reading %d samples\n", SAMPLES);

  for (i = 0; i < SAMPLES; i ++)
  {
    int32 sample;
    fread(&sample, 4, 1, fp);

    src[i] = sample;
    fread(&sample, 4, 1, fp);

    src[i + SAMPLES] = sample;

  }

  memset(&drc, 0, sizeof(drc));
  memset(&drcn, 0, sizeof(drcn));

  void *h = dlopen("/usr/lib/pulse-0.9.15/modules/module-nokia-record.so", RTLD_LAZY);

  mudrc_init_f init = (mudrc_init_f)dlsym(h,"mudrc_init");
  mudrc_process_f process = (mudrc_process_f)dlsym(h,"mudrc_process");
  mumdrc_write_parameters_f write_parameters = (mumdrc_write_parameters_f)dlsym(h,"mumdrc_write_parameters");

  QByteArray param = QByteArray::fromHex("00400000e9500000e9500000e95000000000000000000000880200003e0600003e06000000000000000000005d0a0000c520000018370300000080009932e408058efe085d0a0000c520000018370300000080009932e408058efe085d0a0000c520000018370300000080009932e408058efe08000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009a59000093c4ffff01b8ffff0180ffff0180ffff0080ffff000000009a59000093c4ffff01b8ffff0180ffff0180ffff0080ffff000000009a59000093c4ffff01b8ffff0180ffff0180ffff0080ffff000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000b00ffffffffffffffffffff02000b00ffffffffffffffffffff02000b00ffffffffffffffffffff000000000000000000000000000000000000000000000000000000002f7eec0b3d384e004e004e004e002f7eec0b3d384e004e004e004e002f7eec0b3d384e004e004e004e00000000000000000000000000000000000000000000000000000000000300000000000000000000000000000000000000000000000000000000000000000000000000000000000000");

  init(&drcn, maxblocksize,(float)samplerate);
  mudrc_init(&drc, maxblocksize,(float)samplerate);

  EAP_MultibandDrcInt32 *d = (EAP_MultibandDrcInt32 *)drc.drc;
  EAP_MultibandDrcInt32 *dn = (EAP_MultibandDrcInt32 *)drcn.drc;

  _compare();

  write_parameters(drcn.drc, param.constData(), param.length());
  mumdrc_write_parameters(drc.drc, param.constData(), param.length());

  _compare();


  printf("stock\n");
  timing(
        for (i = 0;i < 1; i ++)
            process(&drcn, dstn, dstn + SAMPLES, src, src + SAMPLES, 2 * SAMPLES);
      )
  printf("foss\n");
  timing(
        for (i = 0;i < 1; i ++)
            mudrc_process(&drc, dst, dst + SAMPLES, src, src + SAMPLES, 2 * SAMPLES);
      )

  _compare();


  fpout = fopen("/opt/eap/bin/testout.raw", "w+");
  fpoutn = fopen("/opt/eap/bin/testoutn.raw", "w+");

  assert(fpout);
  assert(fpoutn);

  for (i = 0; i < SAMPLES; i ++)
  {
    fwrite(&dst[i], 4, 1, fpout);
    fwrite(&dst[i + SAMPLES], 4, 1, fpout);

    fwrite(&dstn[i], 4, 1, fpoutn);
    fwrite(&dstn[i + SAMPLES], 4, 1, fpoutn);

  }

}
