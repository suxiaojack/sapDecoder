#ifndef REBERATION_H_
#define REBERATION_H_

#include <vector>
using namespace std;
class Filter;

class Reverb {
 public:
  Reverb(int block_size, int sampling_rate, float _reberationtime);
  float GetQuietPeriod() const;

  void AddReberation(const vector<float>& input,
                     vector<float>* output_left,
                     vector<float>* output_right);

  const vector<float>& GetImpulseResponseLeft() const;
  const vector<float>& GetImpulseResponseRight() const;

 private:
  void RenderImpulseResponse(int block_size, int sampling_rate,
                             float _reberationtime);
  static float FloatRand();
  int _block_size;
  vector<float> impulse_response_left_;
  vector<float> impulse_response_right_;
  float quiet_period_sec_;

  Filter* left__reberationfilter_;
  Filter* right__reberationfilter_;
  vector<float> _reberationinput_;
  vector<float> _reberationoutput_left_;
  vector<float> _reberationoutput_right_;
  int _reberationoutput_read_pos_;

};

#endif

