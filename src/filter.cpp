#include <assert.h>
#include <cmath>

#include "filter.h"
#include "filter_impl.h"

using namespace std;

Filter::Filter(int filter_len)
    : _filter_impl(new FilterImpl(filter_len)) {
}

Filter::~Filter() {
  delete _filter_impl;
}

void Filter::SetTimeDomainKernel(const vector<float>& kernel) {
  _filter_impl->SetTimeDomainKernel(kernel);
}

void Filter::AddFreqDomainKernel(const vector<float>& kernel) {
  _filter_impl->AddFreqDomainKernel(kernel);
}

void Filter::SetFreqDomain(const vector<float>& kernel) {
  _filter_impl->SetFreqDomain(kernel);
}

void Filter::AddTimeDomainKernel(const vector<float>& kernel) {
  _filter_impl->AddTimeDomainKernel(kernel);
}

void Filter::ForwardTransform(const vector<float>& time_signal,
                                 vector<float>* freq_signal) const {
  _filter_impl->ForwardTransform(time_signal, freq_signal);
}

void Filter::InverseTransform(const vector<float>& freq_signal,
                                 vector<float>* time_signal) const {
  _filter_impl->InverseTransform(freq_signal, time_signal);
}

void Filter::AddSignalBlock(const vector<float>& signal_block) {
  _filter_impl->AddSignalBlock(signal_block);
}

void Filter::GetResult(vector<float>* signal_block) {
  _filter_impl->GetResult(signal_block);
}

