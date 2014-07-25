#ifndef FFT_FILTER_H_
#define FFT_FILTER_H_

#include <vector>

using namespace std;

class FilterImpl;

class Filter {
 public:
  Filter(int filter_len);
  virtual ~Filter();

  void SetTimeDomainKernel(const vector<float>& kernel);
  void AddTimeDomainKernel(const vector<float>& kernel);

  void SetFreqDomain(const vector<float>& kernel);
  void AddFreqDomainKernel(const vector<float>& kernel);

  void ForwardTransform(const vector<float>& time_signal,
                        vector<float>* freq_signal) const;
  void InverseTransform(const vector<float>& freq_signal,
                        vector<float>* time_signal) const;

  void AddSignalBlock(const vector<float>& signal_block);

  void GetResult(vector<float>* signal_block);
 private:
  FilterImpl* _filter_impl;

};

#endif
