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
//
//	The search range for the coarse frequency offset is
//	It is pretty large, since it must deal with oscillator offsets
//	of up to 34, 35 Khz
#define	SEARCH_RANGE		(2 * 36)
#define	CORRELATION_LENGTH	18

	ofdmDecoder::ofdmDecoder	(DabParams	*p,
	                                 RingBuffer<DSPCOMPLEX> *iqBuffer,
	                                 DSPCOMPLEX	*refTable,
	                                 RadioInterface *mr) {
int16_t	i;
	this	-> params		= p;
	this	-> iqBuffer		= iqBuffer;
	this	-> refTable		= refTable;
	this	-> myRadioInterface	= mr;
	this	-> T_s			= params	-> T_s;
	this	-> T_u			= params	-> T_u;
	this	-> carriers		= params	-> K;
	connect (this, SIGNAL (showIQ (int)),
	         mr, SLOT (showIQ (int)));
	this	-> delta	= T_s - T_u;
	fft_handler		= new common_fft (T_u);
	fft_buffer		= fft_handler -> getVector ();
	phaseReference		= new DSPCOMPLEX [T_u];
	myMapper		= new permVector (params);
	iqCount			= 0;
	displayToken		= 2;
//
	connect (this, SIGNAL (show_snr (int)),
	         mr, SLOT (show_snr (int)));
	snrCount		= 0;
	snr			= 0;	
//
//	and for the correlation:
	refArg			= new float [CORRELATION_LENGTH];
	correlationVector	= new float [SEARCH_RANGE + CORRELATION_LENGTH];
	for (i = 0; i < CORRELATION_LENGTH; i ++)  {
	   refArg [i] = arg (refTable [(T_u + i) % T_u] *
	                        conj (refTable [(T_u + i + 1) % T_u]));
	}
}

	ofdmDecoder::~ofdmDecoder	(void) {
	delete	fft_handler;
	delete	phaseReference;
	delete	myMapper;
	delete[]	correlationVector;
	delete[]	refArg;
}

int16_t	ofdmDecoder::processBlock_0 (DSPCOMPLEX *vi, bool flag) {
int16_t	i, j, index_1 = 100, index_2	= 100;
float	Min	= 1000;

	memcpy (fft_buffer, vi, T_u * sizeof (DSPCOMPLEX));
	fft_handler	-> do_FFT ();

	memcpy (phaseReference, fft_buffer, T_u * sizeof (DSPCOMPLEX));

	snr		= 0.7 * snr + 0.3 * get_snr (fft_buffer);
	if (++snrCount > 10) {
	   show_snr (snr);
	   snrCount = 0;
	}

	if (!flag)		// no need to synchronize further
	   return 0;
//	as a side effect we "compute" an estimate for the
//	coarse offset
#ifdef	SIMPLE_SYNCHRONIZATION
	return getMiddle (fft_buffer);
#endif
//

#ifdef	FULL_CORRELATION
//
//	The phase differences are computed once
	for (i = 0; i < SEARCH_RANGE + CORRELATION_LENGTH; i ++) {
	   int16_t baseIndex = T_u - SEARCH_RANGE / 2 + i;
	   correlationVector [i] =
	                   arg (fft_buffer [baseIndex % T_u] *
	                    conj (fft_buffer [(baseIndex + 1) % T_u]));
	}
	float	MMax	= 0;
	float	oldMMax	= 0;
	for (i = 0; i < SEARCH_RANGE; i ++) {
	   float sum	= 0;
	   for (j = 1; j < CORRELATION_LENGTH; j ++) {
	      sum += abs (refArg [j] * correlationVector [i + j]);
	      if (sum > MMax) {
	         oldMMax	= MMax;
	         MMax = sum;
	         index_1 = i;
	      }
	   }
	}
//
//	Now map the index back to the right carrier
	return T_u - SEARCH_RANGE / 2 + index_1 - T_u;
#else
//
//	An alternative way is to look at a special pattern consisting
//	of zeros in the row of args between successive carriers.
	float Mmin	= 1000;
	float OMmin	= 1000;
	for (i = T_u - SEARCH_RANGE / 2; i < T_u + SEARCH_RANGE / 2; i ++) {
              float a1  =  abs (abs (arg (fft_buffer [(i + 1) % T_u] *
                                conj (fft_buffer [(i + 2) % T_u])) / M_PI) - 1);
	      float a2	= abs (arg (fft_buffer [(i + 1) % T_u] *
	      	                    conj (fft_buffer [(i + 3) % T_u])));
	      float a3	= abs (arg (fft_buffer [(i + 3) % T_u] *
	      	                    conj (fft_buffer [(i + 4) % T_u])));
	      float a4	= abs (arg (fft_buffer [(i + 4) % T_u] *
	      	                    conj (fft_buffer [(i + 5) % T_u])));
	      float a5	= abs (arg (fft_buffer [(i + 5) % T_u] *
	      	                    conj (fft_buffer [(i + 6) % T_u])));
	      float b1	= abs (abs (arg (fft_buffer [(i + 16 + 1) % T_u] *
	      	                    conj (fft_buffer [(i + 16 + 3) % T_u])) / M_PI) - 1);
	      float b2	= abs (arg (fft_buffer [(i + 16 + 3) % T_u] *
	      	                    conj (fft_buffer [(i + 16 + 4) % T_u])));
	      float b3	= abs (arg (fft_buffer [(i + 16 + 4) % T_u] *
	      	                    conj (fft_buffer [(i + 16 + 5) % T_u])));
	      float b4	= abs (arg (fft_buffer [(i + 16 + 5) % T_u] *
	      	                    conj (fft_buffer [(i + 16 + 6) % T_u])));
	      float sum = a1 + a2 + a3 + a4 + a5 + b1 + b2 + b3 + b4;
	      if (sum < Mmin) {
	        OMmin = Mmin;
	         Mmin = sum;
	         index_2 = i;
	      }
	}
	return index_2 - T_u;
#endif
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

	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper -> mapIn (i);
	   if (index < 0) 
	      index += T_u;
	      
	   DSPCOMPLEX	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
	   DSPFLOAT ab1	= jan_abs (r1);
//	Recall:  with this viterbi decoder
//	we have 127 = max pos, -127 = max neg
	   ibits [i]		= - real (r1) / ab1 * 127.0;
	   ibits [carriers + i] = - imag (r1) / ab1 * 127.0;
	}

//	and, from time to time we show some clouds in the "old" GUI
//	Since we are in a different thread, directly calling of the
//	iqBuffer is not done. We use an intermediate buffer
//	Note that we do it in two steps since the
//	fftbuffer contains low and high at the ends
	if (blkno == displayToken) {
	   if (++cnt > 7) {
	      iqBuffer	-> putDataIntoBuffer (&fft_buffer [0],
	                                      carriers / 2);
	      iqBuffer	-> putDataIntoBuffer (&fft_buffer [T_u - 1 - carriers / 2],
	                                      carriers / 2);
	      showIQ	(carriers);
	      cnt = 0;
	   }
	}
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


int16_t	ofdmDecoder::getMiddle (DSPCOMPLEX *v) {
int16_t		i;
DSPFLOAT	sum = 0;
int16_t		maxIndex = 0;
DSPFLOAT	oldMax	= 0;
//
//	basic sum over K carriers that are - most likely -
//	in the range
//	The range in which the carrier should be is
//	T_u / 2 - K / 2 .. T_u / 2 + K / 2
//	We first determine an initial sum over 1536 carriers
	for (i = 40; i < 1536 + 40; i ++)
	   sum += abs (v [(T_u / 2 + i) % T_u]);
//
//	Now a moving sum, look for a maximum within a reasonable
//	range (around (T_u - K) / 2, the start of the useful frequencies)
	for (i = 40; i < T_u - (1536 - 40); i ++) {
	   sum -= abs (v [(T_u / 2 + i) % T_u]);
	   sum += abs (v [(T_u / 2 + i + 1536) % T_u]);
	   if (sum > oldMax) {
	      sum = oldMax;
	      maxIndex = i;
	   }
	}
	return maxIndex - (T_u - 1536) / 2;
}
