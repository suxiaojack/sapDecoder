#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "sapdecoder.h"
#include "hrtf.h"
#include "filter.h"
#include "reberation.h"
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "convert.h"
#ifdef _WIN32
#include "stddef.h"
#endif

SAPDecoder::SAPDecoder()
{
	_hrtf = NULL;
	_left_hrtf_filter= NULL;
	_right_hrtf_filter= NULL;
	_reverb= NULL;
	_eq_t = NULL;

	inputFile = NULL;
	ouputFile = NULL;

	_elevation_deg = 0.0f;
	_azimuth_deg = 0.0f;
	_distance = 0.0f;

	filter[0] = 0;
	filter[1] = 0;
}

SAPDecoder::~SAPDecoder() 
{
	_hrtf = NULL;
	_left_hrtf_filter= NULL;
	_right_hrtf_filter= NULL;
	_reverb= NULL;
	_eq_t = NULL;
}

void SAPDecoder::OpenDecoder()
{
	_signal_block.resize(BLOCK_SIZE, 0.0f);

	InitXFadeWindow();

	_hrtf = new HRTF(SAMPLE_RATE, BLOCK_SIZE);
	_left_hrtf_filter = new Filter(BLOCK_SIZE);
	_right_hrtf_filter = new Filter(BLOCK_SIZE);

	_left_hrtf_filter->SetFreqDomain(_hrtf->GetLeftFreqHRTF());
	_right_hrtf_filter->SetFreqDomain(_hrtf->GetRightFreqHRTF());

	_reverb = new Reverb(BUF_MAX_LEN, SAMPLE_RATE,REVERB_TIME);

	_eq_t = new Equalizer();

}

void SAPDecoder::SetDecodeParam(int outChannel,int outEffect)
{
	_channel = outChannel;
	_effect = outEffect;

	_eq_t->EqzPreset(_effect);

	SetDirection(10,10,1.4);
}

void SAPDecoder::SetEqualizer( float *out, float *in,int i_samples, int i_channels )
{
	int each =  i_samples / i_channels;
	for( int i = 0; i < each; i++ )
    {
		_eq_t->EqzFilter(out, in , 1, i_channels);

        in  += i_channels;
        out += i_channels;
    }
}

void SAPDecoder::CloseDecoder()
{
	if(_hrtf) delete _hrtf;
	if(_left_hrtf_filter) delete _left_hrtf_filter;
	if(_right_hrtf_filter) delete _right_hrtf_filter;
	if(_reverb) delete _reverb;
	if(_eq_t) delete _eq_t;
}

void SAPDecoder::SetDirection(float elevation_deg, float azimuth_deg,float distance) 
{
	_elevation_deg = elevation_deg;
	_azimuth_deg = azimuth_deg;

	float hrft_distance = _hrtf->GetDistance();
	_distance = distance > hrft_distance ? distance : hrft_distance;
	_damping = hrft_distance / _distance;
	assert(_damping >= 0 && _damping <= 1.0f);
}

void SAPDecoder::InitXFadeWindow() {
	_xfade_window.resize(BLOCK_SIZE);
	double phase_step = M_PI / 2.0 / (BLOCK_SIZE - 1);
	for (int i = 0; i < BLOCK_SIZE; ++i) {
		_xfade_window[i] = sin(i * phase_step);
		_xfade_window[i] *= _xfade_window[i];
	}
}

void SAPDecoder::DecodeBuffer(const vector<float>&input,	vector<float>* output_left,	vector<float>* output_right) 
{
	assert(output_left != 0 && output_right != 0);
	_left_hrtf_filter->AddSignalBlock(input);
	vector<float> current_hrtf_output_left;
	_left_hrtf_filter->GetResult(&current_hrtf_output_left);
	_right_hrtf_filter->AddSignalBlock(input);

	vector<float> current_hrtf_output_right;
	_right_hrtf_filter->GetResult(&current_hrtf_output_right);

	bool new_hrtf_selected = _hrtf->SetDirection(_elevation_deg, _azimuth_deg);
	if (!new_hrtf_selected) {
		output_left->swap(current_hrtf_output_left);
		output_right->swap(current_hrtf_output_right);
	} else {
		_left_hrtf_filter->SetFreqDomain(_hrtf->GetLeftFreqHRTF());
		_right_hrtf_filter->SetFreqDomain(_hrtf->GetRightFreqHRTF());

		_left_hrtf_filter->AddSignalBlock(_signal_block);
		_right_hrtf_filter->AddSignalBlock(_signal_block);

		_left_hrtf_filter->AddSignalBlock(input);
		_right_hrtf_filter->AddSignalBlock(input);

		vector<float> updated_hrtf_output_left;
		_left_hrtf_filter->GetResult(&updated_hrtf_output_left);
		vector<float> updated_hrtf_output_right;
		_right_hrtf_filter->GetResult(&updated_hrtf_output_right);

		output_left->resize(BLOCK_SIZE);
		SetXFadeWindow(current_hrtf_output_left, updated_hrtf_output_left,output_left);
		output_right->resize(BLOCK_SIZE);
		SetXFadeWindow(current_hrtf_output_right, updated_hrtf_output_right,output_right);
	}

	_signal_block = input;

	SetDamping(_damping, output_left);
	SetDamping(_damping, output_right);

	_reverb->AddReberation(input, output_left, output_right);
}

void SAPDecoder::SetXFadeWindow(const vector<float>& block_a,	const vector<float>& block_b,vector<float>* output) 
{
	assert(output != 0);
	assert(block_a.size() == block_b.size());
	assert(block_a.size() == _xfade_window.size());
	assert(output->size() == _xfade_window.size());

	for (int i = 0; i < block_a.size(); ++i) {
		(*output)[i] = block_a[i] * _xfade_window[_xfade_window.size() - 1 - i]
		+ block_b[i] * _xfade_window[i];
	}
}

void SAPDecoder::SetDamping(float _dampingfactor,	vector<float>* block) 
{
	assert(block != 0);
	for (int i = 0; i < block->size(); ++i) {
		(*block)[i] *= _dampingfactor;
	}
}

void SAPDecoder::ProcessDecoding(const char *infilename,const char *outfilename)
{
	long int sampleNum, channelNum;
	double sampleFreq;
	int done = 0;
	int offset = 0;
	int samplesRead ;
	int frameCounter = 0;
	int current_section ;
	int samplesToReadPerCallByte = BLOCK_SIZE*sizeof(short)*IN_CHANNEL;
	char **ptr;
	char inputBuffer[BLOCK_SIZE*sizeof(short)*IN_CHANNEL];
	float mixBuffer[BLOCK_SIZE*IN_CHANNEL];
	float virBuffer[BLOCK_SIZE*IN_CHANNEL];
	float reaBuffer[BLOCK_SIZE*OUT_CHANNEL];
	vorbis_info *vi;
	OggVorbis_File vf;
	bool bStdOut= false;

	if( infilename == NULL ) return;
	if( outfilename == NULL ) bStdOut = true;

	////vorbis file open
	if((inputFile = fopen(infilename,"rb")) == NULL )
	{
		printf("Could not open input as an SapVorbis file.\n");
		return;
	}

	if(ov_open_callbacks(inputFile,&vf,NULL,0,OV_CALLBACKS_NOCLOSE)<0){
		printf("Could not open input as an SapVorbis file.\n");
		return;
	}

	ptr=ov_comment(&vf,-1)->user_comments;
	vi=ov_info(&vf,-1);
	while(*ptr){
		fprintf(stderr,"%s\n",*ptr);
		++ptr;
	}
	fprintf(stderr,"Bitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
	fprintf(stderr,"Decoded length: %ld samples\n",	(long)ov_pcm_total(&vf,-1));
	//fprintf(stderr,"Encoded by: %s\n",ov_comment(&vf,-1)->vendor);

	sampleFreq = vi->rate;
	channelNum = vi->channels;
	sampleNum = ov_pcm_total(&vf,-1);

	if(channelNum != IN_CHANNEL) return;

	ouputFile = AFopnWrite(outfilename, 2, 5, _channel, sampleFreq, 0);

	while (!done) {
		while(offset < samplesToReadPerCallByte)
		{
			samplesRead = ov_read(&vf,inputBuffer+offset,samplesToReadPerCallByte-offset,0,2,1,&current_section);
			offset +=  samplesRead;
			if( samplesRead < 0) 
			{
				printf("Could not read vorbis file: %d\n",current_section);	
				break; 
			}
			if( samplesRead == 0) 
			{
				printf("Vorbis file ended: %d\n",current_section);	
				break; 
			}
		}

		samplesRead = offset;
		offset = 0;

		if (samplesRead != samplesToReadPerCallByte)  done =1;

		if ((!done)) 
		{
			int sampleCount = Char2Float(mixBuffer,inputBuffer,samplesToReadPerCallByte/sizeof(short));

			if( _channel == IN_CHANNEL && sampleCount >0 )
			{
				ApplyVirtualSurround(virBuffer,mixBuffer,sampleCount);
				AFfWriteData(ouputFile, virBuffer,BLOCK_SIZE*IN_CHANNEL); 
			}

			if( _channel == OUT_CHANNEL && sampleCount >0 )
			{
				ApplyRealSurround(reaBuffer,mixBuffer,sampleCount);
				AFfWriteData(ouputFile, reaBuffer,BLOCK_SIZE*OUT_CHANNEL); 
			}

			printf("[%3d%%]\r",(25*samplesRead*frameCounter)/sampleNum);
			frameCounter++;
		}

	}

	ov_clear(&vf);

	if(inputFile != NULL) {
		fclose(inputFile);
	}
	if(ouputFile != NULL) {
		AFclose(ouputFile);
	}
}


void SAPDecoder::ApplyVirtualSurround(float *outbuf,float *inbuf,int samples)
{
	vector<float> input(BLOCK_SIZE, 0.0f);
	vector<float> output_left(BLOCK_SIZE, 0.0f);
	vector<float> output_right(BLOCK_SIZE, 0.0f);

	const float *in = (const float*)inbuf;

	for(int i=0; i<BLOCK_SIZE; i++ )
	{
		input[i] = (*in + *(in+1)) / 2.0;  
		in += IN_CHANNEL;
	}

	DecodeBuffer(input, &output_left, &output_right);

	float *tmp = (float*)inbuf;
	for (int i=0; i<BLOCK_SIZE; ++i) {
		(*tmp++) = output_left[i];
		(*tmp++) = output_right[i];
	}

	SetEqualizer(outbuf,inbuf,samples,IN_CHANNEL);
}

void SAPDecoder::ApplyRealSurround(float *outbuf,float *inbuf,int samples)
{
	vector<float> output_left(BLOCK_SIZE, 0.0f);
	vector<float> output_right(BLOCK_SIZE, 0.0f);
	vector<float> output_center(BLOCK_SIZE, 0.0f);
	vector<float> output_lfe(BLOCK_SIZE, 0.0f);
	vector<float> output_left_rear(BLOCK_SIZE, 0.0f);
	vector<float> output_right_rear(BLOCK_SIZE, 0.0f);
	double biquad_in[BLOCK_SIZE+2];
	double biquad_out[BLOCK_SIZE+2];

	const double h[5] = {0.1,0,0,0,0};
	const float *in = (const float*)inbuf;

	for(int i=0; i<BLOCK_SIZE; i++ )
	{
		output_center[i] = 0.707 * (*in + *(in+1)) / 2.0;  
		output_left_rear[i] = 0.707 * (*in);
		output_right_rear[i] = 0.707 * (*(in+1));
		output_lfe[i] = (*in + *(in+1)) / 2.0;
		biquad_in[i+2] = output_lfe[i];
		biquad_out[i+2] = 0;
		in += IN_CHANNEL;
	}

	/*////
	biquad_in[0] = filter[0];
	biquad_in[1] = filter[1];
	///get lfe channel
	FIdBiquad(biquad_in,biquad_out,BLOCK_SIZE,h);
	filter[0] = biquad_out[0];
	filter[1] = biquad_out[1];
	*/////

	DecodeBuffer(output_center, &output_left, &output_right);

	for (int i=0; i<BLOCK_SIZE; ++i) {
		(*outbuf++) = output_left[i];
		(*outbuf++) = output_right[i];
		(*outbuf++) = output_center[i];
		(*outbuf++) = biquad_out[i+2];
		(*outbuf++) = output_left_rear[i];
		(*outbuf++) = output_right_rear[i];
	}

}