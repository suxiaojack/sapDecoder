#include <string.h>
#include <stdlib.h>
#include "equalizer.hpp"

Equalizer::Equalizer() {

	p_eq_param = (eq_param_t *)malloc( sizeof( eq_param_t ) );
	EqzInit();
}

Equalizer::~Equalizer() {
	
	EqzClean();
	free(p_eq_param);
}

int Equalizer::EqzInit( )
{
    eqz_config_t cfg;
    int i, ch;
	int i_rate = 44100;
	

    EqzCoeffs( i_rate, 1.0f, &cfg );

    /* Create the static filter config */
    p_eq_param->i_band = cfg.i_band;
    p_eq_param->f_alpha = (float *)malloc( p_eq_param->i_band * sizeof(float) );
    p_eq_param->f_beta  = (float *)malloc( p_eq_param->i_band * sizeof(float) );
    p_eq_param->f_gamma = (float *)malloc( p_eq_param->i_band * sizeof(float) );
    if( !p_eq_param->f_alpha || !p_eq_param->f_beta || !p_eq_param->f_gamma )
        goto error;

    for( i = 0; i < p_eq_param->i_band; i++ )
    {
        p_eq_param->f_alpha[i] = cfg.band[i].f_alpha;
        p_eq_param->f_beta[i]  = cfg.band[i].f_beta;
        p_eq_param->f_gamma[i] = cfg.band[i].f_gamma;
    }

    /* Filter dyn config */
    p_eq_param->b_2eqz = false;
    p_eq_param->f_gamp =  1.f;
    p_eq_param->f_amp  = (float *)malloc( p_eq_param->i_band * sizeof(float) );
    if( !p_eq_param->f_amp )
        goto error;

    for( i = 0; i < p_eq_param->i_band; i++ )
    {
        p_eq_param->f_amp[i] = 0.0f;
    }

    /* Filter state */
    for( ch = 0; ch < 32; ch++ )
    {
        p_eq_param->x[ch][0]  =
        p_eq_param->x[ch][1]  =
        p_eq_param->x2[ch][0] =
        p_eq_param->x2[ch][1] = 0.0f;

        for( i = 0; i < p_eq_param->i_band; i++ )
        {
            p_eq_param->y[ch][i][0]  =
            p_eq_param->y[ch][i][1]  =
            p_eq_param->y2[ch][i][0] =
            p_eq_param->y2[ch][i][1] = 0.0f;
        }
    }

    p_eq_param->b_2eqz = false;

    return  1;

error:
    free( p_eq_param->f_alpha );
    free( p_eq_param->f_beta );
    free( p_eq_param->f_gamma );

    return 0;
}

void  Equalizer::EqzPreset( int eq_num  )
{
	int i;
	const eqz_preset_t *preset = NULL;

	preset = eqz_preset_10b + eq_num;
    p_eq_param->f_gamp =  preset->f_preamp;
    for( i = 0; i < p_eq_param->i_band; i++ )
    {
        p_eq_param->f_amp[i] = preset->f_amp[i];
    }
}

void Equalizer::EqzFilter( float *out,float *in,
                       int i_samples, int i_channels )
{
    int i, ch, j;

    for( i = 0; i < i_samples; i++ )
    {
        for( ch = 0; ch < i_channels; ch++ )
        {
            const float x = in[ch];
            float o = 0.0f;

            for( j = 0; j < p_eq_param->i_band; j++ )
            {
                float y = p_eq_param->f_alpha[j] * ( x - p_eq_param->x[ch][1] ) +
                          p_eq_param->f_gamma[j] * p_eq_param->y[ch][j][0] -
                          p_eq_param->f_beta[j]  * p_eq_param->y[ch][j][1];

                p_eq_param->y[ch][j][1] = p_eq_param->y[ch][j][0];
                p_eq_param->y[ch][j][0] = y;

                o += y * p_eq_param->f_amp[j];
            }
            p_eq_param->x[ch][1] = p_eq_param->x[ch][0];
            p_eq_param->x[ch][0] = x;

            /* Second filter */
            if( p_eq_param->b_2eqz )
            {
                const float x2 = EQZ_IN_FACTOR * x + o;
                o = 0.0f;
                for( j = 0; j < p_eq_param->i_band; j++ )
                {
                    float y = p_eq_param->f_alpha[j] * ( x2 - p_eq_param->x2[ch][1] ) +
                              p_eq_param->f_gamma[j] * p_eq_param->y2[ch][j][0] -
                              p_eq_param->f_beta[j]  * p_eq_param->y2[ch][j][1];

                    p_eq_param->y2[ch][j][1] = p_eq_param->y2[ch][j][0];
                    p_eq_param->y2[ch][j][0] = y;

                    o += y * p_eq_param->f_amp[j];
                }
                p_eq_param->x2[ch][1] = p_eq_param->x2[ch][0];
                p_eq_param->x2[ch][0] = x2;

                /* We add source PCM + filtered PCM */
                out[ch] = p_eq_param->f_gamp * p_eq_param->f_gamp *( EQZ_IN_FACTOR * x2 + o );
            }
            else
            {
                /* We add source PCM + filtered PCM */
                out[ch] = p_eq_param->f_gamp *( EQZ_IN_FACTOR * x + o );
            }
        }

        in  += i_channels;
        out += i_channels;
    }

}

void Equalizer::EqzClean( )
{

    free( p_eq_param->f_alpha );
    free( p_eq_param->f_beta );
    free( p_eq_param->f_gamma );

    free( p_eq_param->f_amp );
}


/* Equalizer coefficient calculation function based on equ-xmms */
void Equalizer::EqzCoeffs( int i_rate, float f_octave_percent,eqz_config_t *p_eqz_config )
{
    const float *f_freq_table_10b = f_iso_frequency_table_10b;
    float f_rate = (float) i_rate;
    float f_nyquist_freq = 0.5f * f_rate;
    float f_octave_factor = powf( 2.0f, 0.5f * f_octave_percent );
    float f_octave_factor_1 = 0.5f * ( f_octave_factor + 1.0f );
    float f_octave_factor_2 = 0.5f * ( f_octave_factor - 1.0f );

    p_eqz_config->i_band = EQZ_BANDS_MAX;

    for( int i = 0; i < EQZ_BANDS_MAX; i++ )
    {
        float f_freq = f_freq_table_10b[i];

        p_eqz_config->band[i].f_frequency = f_freq;

        if( f_freq <= f_nyquist_freq )
        {
            float f_theta_1 = ( 2.0f * (float) M_PI * f_freq ) / f_rate;
            float f_theta_2 = f_theta_1 / f_octave_factor;
            float f_sin     = sinf( f_theta_2 );
            float f_sin_prd = sinf( f_theta_2 * f_octave_factor_1 )
                            * sinf( f_theta_2 * f_octave_factor_2 );
            float f_sin_hlf = f_sin * 0.5f;
            float f_den     = f_sin_hlf + f_sin_prd;

            p_eqz_config->band[i].f_alpha = f_sin_prd / f_den;
            p_eqz_config->band[i].f_beta  = ( f_sin_hlf - f_sin_prd ) / f_den;
            p_eqz_config->band[i].f_gamma = f_sin * cosf( f_theta_1 ) / f_den;
        }
        else
        {
            /* Any frequency beyond the Nyquist frequency is no good... */
            p_eqz_config->band[i].f_alpha = 0.0f;
            p_eqz_config->band[i].f_beta  = 0.0f;
            p_eqz_config->band[i].f_gamma = 0.0f;
        }
    }
}

float Equalizer::EqzConvertdB( float db )
{
    /* Map it to gain,
     * (we do as if the input of iir is /EQZ_IN_FACTOR, but in fact it's the non iir data that is *EQZ_IN_FACTOR)
     * db = 20*log( out / in ) with out = in + amp*iir(i/EQZ_IN_FACTOR)
     * or iir(i) == i for the center freq so
     * db = 20*log( 1 + amp/EQZ_IN_FACTOR )
     * -> amp = EQZ_IN_FACTOR*(10^(db/20) - 1)
     **/

    if( db < -20.0f )
        db = -20.0f;
    else if(  db > 20.0f )
        db = 20.0f;
    return EQZ_IN_FACTOR * ( powf( 10.0f, db / 20.0f ) - 1.0f );
}