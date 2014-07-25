#ifndef _equalizer_
#define _equalizer_

#include <math.h>

#define EQZ_IN_FACTOR (0.25f)
#define EQZ_BANDS_MAX 10

typedef struct
{
    const char psz_name[16];
    int  i_band;
    float f_preamp;
    float f_amp[EQZ_BANDS_MAX];
} eqz_preset_t;

typedef struct 
{
	/* Filter static config */
	int i_band;
	float *f_alpha;
	float *f_beta;
	float *f_gamma;

	/* Filter dyn config */
	float *f_amp;   /* Per band amp */
	float f_gamp;   /* Global preamp */

	/* Filter state */
	float x[32][2];
	float y[32][128][2];

	/* Second filter state */
	float x2[32][2];
	float y2[32][128][2];
	int b_2eqz;
} eq_param_t;

/*****************************************************************************
* Equalizer stuff
*****************************************************************************/
typedef struct
{
	int   i_band;

	struct
	{
		float f_frequency;
		float f_alpha;
		float f_beta;
		float f_gamma;
	} band[EQZ_BANDS_MAX];

} eqz_config_t;

static const float f_iso_frequency_table_10b[EQZ_BANDS_MAX] =
{
    31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000,
};

static const eqz_preset_t eqz_preset_10b[3] =
{
    {
        "music", 10, 1.0f,
        { -1.11022e-15f, -1.11022e-15f, -1.11022e-15f, -5.6f, -1.11022e-15f,
          6.4f, 6.4f, -1.11022e-15f, -1.11022e-15f, -1.11022e-15f }
    },
    {
        "live", 10, 1.1f,
        { -4.8f, -1.11022e-15f, 4.0f, 5.6f, 5.6f, 5.6f, 4.0f, 2.4f,
          2.4f, 2.4f }
    },
    {
        "movie", 10, 1.2f,
        { -8.0f, 9.6f, 9.6f, 5.6f, 1.6f, -4.0f, -8.0f, -10.4f, -11.2f, -11.2f }
    },
};

class Equalizer
{
public:
	Equalizer();
	~Equalizer();

	void EqzFilter( float *, float *, int, int );
	void EqzPreset( int );

private:
	eq_param_t *p_eq_param;

private:
	int  EqzInit(  );
	void EqzClean( );
	float EqzConvertdB( float db );
	void EqzCoeffs( int i_rate, float f_octave_percent,eqz_config_t *p_eqz_config );

};


#endif