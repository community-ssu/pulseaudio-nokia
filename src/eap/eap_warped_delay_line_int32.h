#ifndef EAP_WARPED_DELAY_LINE_INT32_H
#define EAP_WARPED_DELAY_LINE_INT32_H

static void
EAP_WarpedDelayLineInt32_Process(int32 shiftValue, int32 *delayLine,
                                 int32 delayLength, int32 input)
{
  int32 mem1;
  int i;

  mem1 = *delayLine;
  *delayLine = input;

  for (i = 1; i < delayLength; i++ )
  {
    int32 mem2 = delayLine[i];
    int32 s = mem2 - delayLine[i - 1];

    delayLine[i] = s - (s >> shiftValue) + mem1;
    mem1 = mem2;
  }
}

#endif // EAP_WARPED_DELAY_LINE_INT32_H
