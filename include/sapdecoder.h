#ifndef SAP_DECODER_SOURCE_H_
#define SAP_DECODER_SOURCE_H_

#include "AFsp/libmtsp.h"
#include "equalizer.hpp"
#include <vector>

using namespace std;

#define BUF_MAX_LEN 4096
#define REVERB_TIME	.1
#define SAMPLE_RATE 44100.0
#define BLOCK_SIZE 512	
#define IN_CHANNEL 2
#define OUT_CHANNEL 6

class Filter;
class HRTF;
class Reverb;

class SAPDecoder {
public:
	SAPDecoder();
	virtual ~SAPDecoder();

	void OpenDecoder();
	void CloseDecoder();
	void SetDecodeParam(int outChannel,int outEffect);
	void ProcessDecoding(const char *infile,const char *outfile);

private:

	float _elevation_deg;
	float _azimuth_deg;
	float _distance;
	float _damping;

	int _channel;
	int _effect;

	float filter[2];

	vector<float> _xfade_window;
	vector<float> _signal_block;

	HRTF* _hrtf;
	Filter* _left_hrtf_filter;
	Filter* _right_hrtf_filter;
	Reverb* _reverb;
	Equalizer* _eq_t;

	/////////////file
	FILE *inputFile;
	AFILE *ouputFile;

private:
	void DecodeBuffer(const vector<float>&input,vector<float>* output_left,vector<float>* output_right);
	void SetDamping(float _dampingfactor, vector<float>* block);
	void SetEqualizer( float *out, float *in,int i_samples, int i_channels );
	void InitXFadeWindow();
	void SetXFadeWindow(const vector<float>& block_a,const vector<float>& block_b,vector<float>* output);
	void SetDirection(float elevation_deg, float azimuth_deg, float distance);
	void ApplyVirtualSurround(float *outbuf,float *inbuf,int samples);
	void ApplyRealSurround(float *outbuf,float *inbuf,int samples);

};

#endif
