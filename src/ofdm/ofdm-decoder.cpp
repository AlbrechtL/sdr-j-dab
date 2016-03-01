#
/*
 *    Copyright (C) 2013 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Once the bits are "in", interpretation and manipulation
 *	should reconstruct the data blocks.
 *	Ofdm_decoder is called once every Ts samples, and
 *	its invocation results in 2 * Tu bits
 */
#include	"ofdm-decoder.h"
#include	"gui.h"
#include	"phasetable.h"

#define	SYNC_LENGTH	15

extern fftwf_complex prs_static[1536];

	ofdmDecoder::ofdmDecoder	(DabParams	*p,
	                                 RingBuffer<DSPCOMPLEX> *iqBuffer,
	                                 DSPCOMPLEX	*refTable,
	                                 RadioInterface *mr) {

	this	-> params		= p;
	this	-> iqBuffer		= iqBuffer;
	this	-> refTable		= refTable;
	this	-> myRadioInterface	= mr;
	this	-> T_s			= params	-> T_s;
	this	-> T_u			= params	-> T_u;
	this	-> carriers		= params	-> K;

	this	-> syncBuffer		= new float [T_u];

	connect (this, SIGNAL (showIQ (int)),
	         mr, SLOT (showIQ (int)));
	this	-> delta	= T_s - T_u;
	fft_handler		= new common_fft (T_u);
	fft_buffer		= fft_handler -> getVector ();
	phaseReference		= new DSPCOMPLEX [T_u];
    syncBuffer2		= new DSPCOMPLEX [T_u];
	myMapper		= new permVector (params);
	iqCount			= 0;
	coarseOffset		= 0;
	displayToken		= 3;
//
	connect (this, SIGNAL (show_snr (int)),
	         mr, SLOT (show_snr (int)));
	snrCount		= 0;
	snr			= 0;	
}

	ofdmDecoder::~ofdmDecoder	(void) {
	delete	fft_handler;
	delete	phaseReference;
	delete	myMapper;
	delete []	syncBuffer;
    delete	syncBuffer2;
}
//	in practive, we use the "incoming" block
//	and use its data to generate the prs
void	ofdmDecoder::processBlock_0 (DSPCOMPLEX *vi) {
DSPCOMPLEX	*v = (DSPCOMPLEX *)alloca (T_u * sizeof (DSPCOMPLEX));
int16_t	i;

	memcpy (fft_buffer, vi, T_u * sizeof (DSPCOMPLEX));
	fft_handler	-> do_FFT ();

	snr		= 0.7 * snr + 0.3 * get_snr (fft_buffer);
	if (++snrCount > 10) {
	   show_snr (snr);
	   snrCount = 0;
	}
	memcpy (phaseReference, fft_buffer, T_u * sizeof (DSPCOMPLEX));
	for (i = 0; i < T_u; i ++)
	   syncBuffer [i] = abs (fft_buffer [i]);
    memcpy (syncBuffer2, fft_buffer, T_u * sizeof (DSPCOMPLEX));
}

//	for the other blocks of data, the first step is to go from
//	time to frequency domain, to get the carriers.
//	This requires an FFT and zero de-padding and rearrangement
void	ofdmDecoder::processToken (DSPCOMPLEX *inv,
	                           int16_t *ibits, int32_t blkno) {
int16_t		i;
static		int cnt	= 0;

	memcpy (fft_buffer, &inv [delta], T_u * sizeof (DSPCOMPLEX));
	fft_handler -> do_FFT ();
//
//	Note that "mapIn" maps to -carriers / 2 .. carriers / 2
//	we did not set the fft output to low .. high
	if (blkno < SYNC_LENGTH)
	   for (i = 0; i < T_u; i ++)
	      syncBuffer [i] +=  abs (fft_buffer [i]);

	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper -> mapIn (i);
	   if (index < 0) 
	      index += T_u;
	      
	   DSPCOMPLEX	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
	   DSPFLOAT ab1	= jan_abs (r1);
//	Recall:  positive = 0, negative = 1
	   ibits [i]		= real (r1) / ab1 * 255.0;
	   ibits [carriers + i] = imag (r1) / ab1 * 255.0;
	}
//
//	and, from time to time we show some clouds.
//	Since we are in a different thread, directly calling of the
//	iqBuffer is not done. We use an intermediate buffer
//	Note that we do it in two steps since the
//	fftbuffer contains low and high at the ends
	if (blkno == displayToken) {
       if (++cnt > 1) {
	      iqBuffer	-> putDataIntoBuffer (&fft_buffer [0],
	                                      carriers / 2);
	      iqBuffer	-> putDataIntoBuffer (&fft_buffer [T_u - 1 - carriers / 2],
	                                      carriers / 2);
          showIQ	(carriers);
	      cnt = 0;
	   }
	}
}

int16_t	ofdmDecoder::coarseCorrector (void) {
    //coarseOffset 	= getMiddle	(syncBuffer);
    coarseOffset 	= getMiddle2();
	return coarseOffset;
}

//
int16_t	ofdmDecoder::getMiddle (float *v) {
int16_t		i;
DSPFLOAT	sum = 0;
int16_t		maxIndex = 0;
DSPFLOAT	oldMax	= 0;
float	s1	= 0;
int16_t		base1	= 0;
	for (i = (T_u - carriers) / 2;
	     i < (T_u + carriers) / 2; i ++)
	   s1 += v [(T_u / 2 + i) % T_u];
	s1 /= carriers;
//
//	we are looking for the (i, i + carriers + 1) combination
//	with power less than threshold
	for (i = (T_u - carriers) / 2 - 50;
	     i < (T_u - carriers) / 2 + 50; i ++)
	   if (v [(T_u / 2 + i) % T_u] +
	                  v [(T_u / 2 + carriers + i + 1) % T_u] < s1 / 2)
	   base1 = i + 1 + (T_u - carriers) / 2;

//	basic sum over K carriers that are - most likely -
//	in the range
//	The range in which the carrier should be is
//	T_u / 2 - K / 2 .. T_u / 2 + K / 2
//	We first determine an initial sum
	for (i = (T_u -  carriers) / 2 - 50;
	                 i < (T_u + carriers) / 2 - 50; i ++)
	   sum += v [(T_u / 2 + i) % T_u];
	
//	Now a moving sum, look for a maximum within a reasonable
//	range (around (T_u - K) / 2, the start of the useful frequencies)
	for (i = (T_u - carriers) / 2 - 50;
	              i < (T_u - carriers) / 2 + 50; i ++) {
	   sum -= abs (v [(T_u / 2 + i) % T_u]);
	   sum += abs (v [(T_u / 2 + i + carriers) % T_u]);
	   if (sum > oldMax) {
	      sum = oldMax;
	      maxIndex = i;
	   }
	}

       int original = (base1 + maxIndex - (T_u - carriers) / 2) / 2;
       return original;
}

int16_t	ofdmDecoder::getMiddle2 (void)
{
    float ideal_dab_spectrum[T_u * 2-1];
       memset(ideal_dab_spectrum,0,sizeof(ideal_dab_spectrum));
       for(int i=0;i<carriers;i++)
       {
           ideal_dab_spectrum[(T_u - carriers)/2 + i] = 1;
           //ideal_dab_spectrum[(T_u - carriers)/2 + i] = prs_static[i];
       }

       float crosscorrelation[T_u * 2-1];
       memset(crosscorrelation,0,sizeof(crosscorrelation));

       // Start of frequency domain cross correlation
       fftwf_complex *in_v, *out_v, *in_spec, *out_spec, *in_kkf, *out_kkf;
       fftwf_plan p_v, p_spec, p_kkf;

       in_v = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * T_u*2-1);
       out_v = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * T_u*2-1);
       in_spec = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * T_u*2-1);
       out_spec = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * T_u*2-1);
       in_kkf = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * T_u*2-1);
       out_kkf = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * T_u*2-1);

       p_v = fftwf_plan_dft_1d(T_u*2-1, in_v, out_v, FFTW_FORWARD, FFTW_ESTIMATE);
       p_spec = fftwf_plan_dft_1d(T_u*2-1, in_spec,out_spec, FFTW_FORWARD, FFTW_ESTIMATE);
       p_kkf = fftwf_plan_dft_1d(T_u*2-1, in_kkf, out_kkf, FFTW_BACKWARD, FFTW_ESTIMATE);

       // Input
       memset(in_v, 0, sizeof(fftwf_complex) * T_u*2-1);
       for(int i=0;i<T_u;i++)
           //in_v[i][0] = v[(i + T_u/2) % T_u];
       {
           // Copy and recorder
           /*in_v[i][0] = syncBuffer2[(i + T_u/2) % T_u].real();
           in_v[i][1] = syncBuffer2[(i + T_u/2) % T_u].imag();*/
           in_v[i][0] = abs(syncBuffer2[(i + T_u/2) % T_u]);
       }

       memset(in_spec, 0, sizeof(fftwf_complex) * T_u*2-1);
       for(int i=0;i<T_u*2-1;i++)
           in_spec[i][0] = ideal_dab_spectrum[(T_u*2-1)-i-1];
       /*for(int i=0;i<carriers;i++)
       {
           in_spec[(T_u-carriers)/2 + i][0] = prs_static[i][0];
           in_spec[(T_u-carriers)/2 + i][1] = prs_static[i][1];
       }*/

       // Transform to frequency domain
       fftwf_execute(p_v);
       fftwf_execute(p_spec);

       // Do the cross correlation
       for(int i=0;i<T_u*2-1;i++)
       {
           // Complex multiplication
           in_kkf[i][0] = (out_v[i][0] * out_spec[i][0]) - (-out_v[i][1] * -out_spec[i][1]);
           in_kkf[i][1] = (-out_v[i][1] * out_spec[i][0]) + (out_v[i][0] * -out_spec[i][1]);

           //in_kkf[i] = out_v[i] * conj(out_spec[i]);
       }

       // Transform to time domain
       fftwf_execute(p_kkf);

       // Reorder data because the 0-frequency is middle after the FFT
       for (int i = 0; i < 2 * T_u - 1; i++)
       {
           // Reorder
           float real = out_kkf[(i + T_u) % (2 * T_u - 1)][0];
           float imag = out_kkf[(i + T_u) % (2 * T_u - 1)][1];

            // Make value absolute
           crosscorrelation[i] = sqrt(real*real + imag*imag);
       }

       // Looking for maximum
       float old_value = 0;
       int index = 0;
       for(int i=0;i<T_u * 2;i++)
       {
           if(crosscorrelation[i] >= old_value)
           {
               old_value = crosscorrelation[i];
               index = i;
           }
       }
       int tau = index - T_u;

       setlocale(LC_NUMERIC, "en_US.UTF-8");
       /*FILE * pFile;
       pFile = fopen ("data_kkf.m","w");
       if (pFile!=NULL)
       {
           fprintf(pFile,"syncBuffer2=[");
           for(int i=0;i<T_u;i++)
           {
               fprintf(pFile, "%f+%fi ...\n", syncBuffer2[i].real(),syncBuffer2[i].imag());
               //fprintf(pFile, "%i ...\n", ideal_dab_spectrum[i]);
           }
           fprintf(pFile,"];");

           fprintf(pFile,"out_kkf=[");
           for(int i=0;i<T_u * 2-1;i++)
           {
               fprintf(pFile, "%f+%fi ...\n", out_kkf[i][0],out_kkf[i][1]);
               //fprintf(pFile, "%i ...\n", ideal_dab_spectrum[i]);
           }
           fprintf(pFile,"];");

           fprintf(pFile,"out_v=[");
           for(int i=0;i<T_u * 2-1;i++)
           {
               fprintf(pFile, "%f+%fi ...\n", out_v[i][0],out_v[i][1]);
               //fprintf(pFile, "%i ...\n", ideal_dab_spectrum[i]);
           }
           fprintf(pFile,"];");

           fprintf(pFile,"in_v=[");
           for(int i=0;i<T_u * 2-1;i++)
           {
               fprintf(pFile, "%f+%fi ...\n", in_v[i][0],in_v[i][1]);
           }
           fprintf(pFile,"];");

           fprintf(pFile,"in_spec=[");
           for(int i=0;i<T_u * 2-1;i++)
           {
               fprintf(pFile, "%f+%fi ...\n", in_spec[i][0],in_spec[i][1]);
           }
           fprintf(pFile,"];");

           fprintf(pFile,"crosscorrelation=[");
           for(int i=0;i<T_u * 2-1;i++)
           {
               fprintf(pFile, "%f ...\n", crosscorrelation[i]);
           }
           fprintf(pFile,"];");

           fprintf(pFile,"prs_static=[");
           for(int i=0;i<1536;i++)
           {
               fprintf(pFile, "%f+%fi ...\n", prs_static[i][0],prs_static[i][1]);
               //fprintf(pFile, "%i ...\n", ideal_dab_spectrum[i]);
           }
           fprintf(pFile,"];");

           fclose(pFile);
       }*/

       fftwf_destroy_plan(p_kkf);
       fftwf_free(in_kkf);
       fftwf_free(out_kkf);

       fftwf_destroy_plan(p_v);
       fftwf_free(in_v);
       fftwf_free(out_v);

       fftwf_destroy_plan(p_spec);
       fftwf_free(in_spec);
       fftwf_free(out_spec);

       //int original = (base1 + maxIndex - (T_u - carriers) / 2) / 2;
       //return diff;
       //fprintf(stderr,"diff %i test %i\n", diff,test);
       //return test;

       tau = -tau;
       //fprintf(stderr,"old_value = %f, tau = %i\n",old_value, tau);
       return tau;
       //return original;
}

//
//
//	for the snr we have a full T_u wide vector, with in the middle
//	K carriers
int16_t	ofdmDecoder::get_snr (DSPCOMPLEX *v) {
int16_t	i;
DSPFLOAT	noise 	= 0;
DSPFLOAT	signal	= 0;
int16_t	low	= T_u / 2 -  carriers / 2;
int16_t	high	= low + carriers;

	for (i = 10; i < low - 20; i ++)
	   noise += abs (v [(T_u / 2 + i) % T_u]);

	for (i = high + 20; i < T_u - 10; i ++)
	   noise += abs (v [(T_u / 2 + i) % T_u]);

	noise	/= (low - 30 + T_u - high - 30);
	for (i = T_u / 2 - carriers / 4;  i < T_u / 2 + carriers / 4; i ++)
	   signal += abs (v [(T_u / 2 + i) % T_u]);

	return get_db (signal / (carriers / 2)) - get_db (noise);
}

