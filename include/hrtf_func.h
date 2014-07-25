#ifndef HRTF_FUNC_H_
#define HRTF_FUNC_H_

#include <string>
#include "hrtf_data.h"

using namespace std;

struct HRTFDataT {
  const string identifier;
  const int num_hrtfs;
  const int fir_length;
  const int sample_rate;
  const int (*direction)[2];
  const float distance;
  const short (*data)[2][kHRTFFilterLen];
};

static const HRTFDataT kHRTFDataSet = { kHRTFDataIdentifier, kHRTFNum,
    kHRTFFilterLen, kHRTFSampleRate, kHRTFDirection, kDistanceMeter, kHRTFData };

#endif
