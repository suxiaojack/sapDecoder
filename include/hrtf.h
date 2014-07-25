#ifndef HRTF_LOOKUP_H_
#define HRTF_LOOKUP_H_

#include <memory>
#include <utility>
#include <vector>

using namespace std;

class FLANNNeighborSearch;

class HRTF {
 public:
  HRTF(int sample_rate, int block_size);
  virtual ~HRTF();

  bool SetDirection(float elevation_deg, float azimuth_deg);
  void GetDirection(float* elevation_deg, float* azimuth_deg) const;

  const vector<float>& GetLeftEarTimeHRTF() const;
  const vector<float>& GetRightEarTimeHRTF() const;

  const vector<float>& GetLeftFreqHRTF() const;
  const vector<float>& GetRightFreqHRTF() const;

  float GetDistance() const;

  int GetFilterSize() const;

 private:
  static void ConvertShortToFloatVector(const short* input_ptr, int input_size,
                                        vector<float>* output);

  void SampleTransformHRTFs();
  void FreqTransformHRTFs();
  void InitNeighborSearch();

  int _sample_rate;
  int _block_size;

  FLANNNeighborSearch* hrtf_nn_search_;

  int hrtf_index_;
  bool left_right_swap_;

  float hrtf__elevation_deg;
  float hrtf__azimuth_deg;

  int filter_size_;

  typedef vector<float> SampledHRTFT;
  typedef pair<SampledHRTFT, SampledHRTFT> SampledHRTFPairT;
  vector<SampledHRTFPairT> hrtf_time_domain_;
  vector<SampledHRTFPairT> hrtf_freq_domain_;

};

#endif  // HRTF_LOOKUP_H_
