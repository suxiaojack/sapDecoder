#include <assert.h>
#include <cmath>
#include <stdlib.h>
#include "reberation.h"
#include "filter.h"

Reverb::Reverb(int block_size, int sampling_rate, float _reberationtime)
    : _block_size(block_size),
      _reberationoutput_read_pos_(0) {
  RenderImpulseResponse(block_size, sampling_rate, _reberationtime);

  left__reberationfilter_ = new Filter(_block_size);
  left__reberationfilter_->SetTimeDomainKernel(GetImpulseResponseLeft());
  right__reberationfilter_ = new Filter(_block_size);

  right__reberationfilter_->SetTimeDomainKernel(GetImpulseResponseRight());

  _reberationinput_.reserve(_block_size);
  _reberationoutput_left_.resize(_block_size, 0.0f);
  _reberationoutput_right_.resize(_block_size, 0.0f);
}

void Reverb::RenderImpulseResponse(int block_size, int sampling_rate,
                                       float _reberationtime) {
  impulse_response_left_.resize(block_size, 0.0f);
  impulse_response_right_.resize(block_size, 0.0f);

  int quiet_period = block_size;  // filter_delay = block_size
  quiet_period_sec_ = static_cast<float>(quiet_period)
      / static_cast<float>(sampling_rate);
  const float exp_decay = -13.8155;

  srand(0);
  for (int i = 0; i < block_size; ++i) {
    float envelope = exp(
        exp_decay * (i + quiet_period) / sampling_rate / _reberationtime);
    assert(envelope >= 0 && envelope <= 1.0);
    impulse_response_left_[i] = FloatRand() * envelope;
    impulse_response_right_[i] = FloatRand() * envelope;
  }
}
float Reverb::GetQuietPeriod() const {
  return quiet_period_sec_;
}

void Reverb::AddReberation(const vector<float>& input,
                               vector<float>* output_left,
                               vector<float>* output_right) {
  assert(output_left && output_right);
  assert(output_left->size() == output_right->size());
  assert(input.size() == output_right->size());

  for (int i = 0; i < input.size(); ++i) {
    (*output_left)[i] += _reberationoutput_left_[_reberationoutput_read_pos_];
    (*output_right)[i] += _reberationoutput_right_[_reberationoutput_read_pos_];
    ++_reberationoutput_read_pos_;
  }
  _reberationinput_.insert(_reberationinput_.end(), input.begin(), input.end());

  if (_reberationoutput_read_pos_ == _block_size) {
    left__reberationfilter_->AddSignalBlock(_reberationinput_);
    right__reberationfilter_->AddSignalBlock(_reberationinput_);
    _reberationinput_.clear();
    left__reberationfilter_->GetResult(&_reberationoutput_left_);
    right__reberationfilter_->GetResult(&_reberationoutput_right_);
    _reberationoutput_read_pos_ = 0;
  }
  assert(_reberationoutput_read_pos_ < _block_size);
}

float Reverb::FloatRand() {
  return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

const vector<float>& Reverb::GetImpulseResponseLeft() const {
  return impulse_response_left_;
}

const vector<float>& Reverb::GetImpulseResponseRight() const {
  return impulse_response_right_;
}

