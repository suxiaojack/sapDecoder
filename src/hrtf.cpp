#include "hrtf_func.h"
#include "filter.h"
#include "hrtf.h"
#include "nn_search.hpp"

HRTF::HRTF(int sample_rate, int block_size)
{
	_sample_rate = sample_rate;
	_block_size = block_size;
	hrtf_index_ = -1;
	hrtf__elevation_deg = -1.0;
	hrtf__azimuth_deg = -1.0;
	left_right_swap_ = false;
	filter_size_ = -1;

	hrtf_nn_search_ = new FLANNNeighborSearch();

	InitNeighborSearch();
	SampleTransformHRTFs();
	FreqTransformHRTFs();
	SetDirection(0.0f, 0.0f);
}

HRTF::~HRTF() {
  delete hrtf_nn_search_;
}

void HRTF::InitNeighborSearch() {
  // Add orientations of right hemisphere
  for (int i = 0; i < kHRTFDataSet.num_hrtfs; ++i) {
    float elevation_deg = kHRTFDataSet.direction[i][0];
    float azimuth_deg = kHRTFDataSet.direction[i][1];
    hrtf_nn_search_->AddHRTFDirection(elevation_deg, azimuth_deg, i);
  }
  hrtf_nn_search_->BuildIndex();
}

bool HRTF::SetDirection(float elevation_deg, float azimuth_deg) {
  if (elevation_deg == hrtf__elevation_deg
      && azimuth_deg == hrtf__azimuth_deg) {
    return false;
  }

  int new_elevation_deg = elevation_deg;
  int new_azimuth_deg = azimuth_deg;
  while (new_azimuth_deg < -180) {
    new_azimuth_deg += 360;
  }
  while (new_azimuth_deg > 180) {
    new_azimuth_deg -= 360;
  }

  int hrtf_index = hrtf_nn_search_->FindNearestHRTF(new_elevation_deg,
                                                    fabs((float)new_azimuth_deg));
  assert(hrtf_index >= 0 && hrtf_index < kHRTFDataSet.num_hrtfs);

  if (hrtf_index_ == hrtf_index) {
    return false;
  }
  hrtf_index_ = hrtf_index;

  // Right hemisphere
  hrtf__elevation_deg = kHRTFDataSet.direction[hrtf_index][0];
  hrtf__azimuth_deg = kHRTFDataSet.direction[hrtf_index][1];
  left_right_swap_ = (new_azimuth_deg < 0.0f);

  if (left_right_swap_) {
    // Left hemisphere corrections
    hrtf__azimuth_deg = kHRTFDataSet.direction[hrtf_index][1] * -1;
  }

  return true;
}

void HRTF::GetDirection(float* elevation_deg, float* azimuth_deg) const {
  assert(elevation_deg && azimuth_deg);
  *elevation_deg = hrtf__elevation_deg;
  *azimuth_deg = hrtf__azimuth_deg;
}

const vector<float>& HRTF::GetLeftEarTimeHRTF() const {
  assert(hrtf_index_ >= 0 && hrtf_index_ < kHRTFDataSet.num_hrtfs);
  return
      left_right_swap_ ?
          hrtf_time_domain_[hrtf_index_].second :
          hrtf_time_domain_[hrtf_index_].first;
}
const vector<float>& HRTF::GetRightEarTimeHRTF() const {
  assert(hrtf_index_ >= 0 && hrtf_index_ < kHRTFDataSet.num_hrtfs);
  return
      left_right_swap_ ?
          hrtf_time_domain_[hrtf_index_].first :
          hrtf_time_domain_[hrtf_index_].second;
}

const vector<float>& HRTF::GetLeftFreqHRTF() const {
  assert(hrtf_index_ >= 0 && hrtf_index_ < kHRTFDataSet.num_hrtfs);
  return
      left_right_swap_ ?
          hrtf_freq_domain_[hrtf_index_].second :
          hrtf_freq_domain_[hrtf_index_].first;
}
const vector<float>& HRTF::GetRightFreqHRTF() const {
  assert(hrtf_index_ >= 0 && hrtf_index_ < kHRTFDataSet.num_hrtfs);
  return
      left_right_swap_ ?
          hrtf_freq_domain_[hrtf_index_].first :
          hrtf_freq_domain_[hrtf_index_].second;
}

float HRTF::GetDistance() const {
  return kHRTFDataSet.distance;
}

int HRTF::GetFilterSize() const {
  return filter_size_;
}

void HRTF::SampleTransformHRTFs() {
  //only support 44100Hz sample rate
  if( _sample_rate!= kHRTFDataSet.sample_rate) return;

  vector<float> left_hrtf_float(kHRTFDataSet.fir_length);
  vector<float> right_hrtf_float(kHRTFDataSet.fir_length);

  hrtf_time_domain_.resize(kHRTFDataSet.num_hrtfs);
  for (int hrtf_itr = 0; hrtf_itr < kHRTFDataSet.num_hrtfs; ++hrtf_itr) {

    ConvertShortToFloatVector(&kHRTFDataSet.data[hrtf_itr][0][0],
                              kHRTFDataSet.fir_length, &left_hrtf_float);
    ConvertShortToFloatVector(&kHRTFDataSet.data[hrtf_itr][1][0],
                              kHRTFDataSet.fir_length, &right_hrtf_float);

    hrtf_time_domain_[hrtf_itr].first.swap(left_hrtf_float);
    hrtf_time_domain_[hrtf_itr].second.swap(
        right_hrtf_float);
  }
}

void HRTF::FreqTransformHRTFs() {
  Filter _filter(_block_size);
  hrtf_freq_domain_.resize(kHRTFDataSet.num_hrtfs);
  for (int hrtf_itr = 0; hrtf_itr < kHRTFDataSet.num_hrtfs; ++hrtf_itr) {
    _filter.ForwardTransform(hrtf_time_domain_[hrtf_itr].first,
                                &hrtf_freq_domain_[hrtf_itr].first);
    _filter.ForwardTransform(hrtf_time_domain_[hrtf_itr].second,
                                &hrtf_freq_domain_[hrtf_itr].second);
  }
}

void HRTF::ConvertShortToFloatVector(const short* input_ptr, int input_size,
                                     vector<float>* output) {
  assert(output != 0);
  assert(input_ptr != 0);
  output->resize(input_size);
  const short* input_raw_ptr = input_ptr;
  for (int i = 0; i < input_size; ++i, ++input_raw_ptr) {
    (*output)[i] = (*input_raw_ptr) / static_cast<float>(0x7FFF);
  }
}

