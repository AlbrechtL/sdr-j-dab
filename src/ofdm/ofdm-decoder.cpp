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

	connect (this, SIGNAL (showIQ (int)),
	         mr, SLOT (showIQ (int)));
	this	-> delta	= T_s - T_u;
	fft_handler		= new common_fft (T_u);
	fft_buffer		= fft_handler -> getVector ();
	phaseReference		= new DSPCOMPLEX [T_u];
	myMapper		= new permVector (params);
	iqCount			= 0;
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
}

#define	RANGE	36
int16_t	ofdmDecoder::processBlock_0 (DSPCOMPLEX *vi) {
DSPCOMPLEX	*v = (DSPCOMPLEX *)alloca (T_u * sizeof (DSPCOMPLEX));
int16_t	i, j, index = 100;
float	Min	= 1000;
int16_t	ranges [2 * RANGE];

	memcpy (fft_buffer, vi, T_u * sizeof (DSPCOMPLEX));
	fft_handler	-> do_FFT ();

	memcpy (phaseReference, fft_buffer, T_u * sizeof (DSPCOMPLEX));
//
//	as a side effect we "compute" an estimate for the
//	coarse offset
	for (i = 0; i < RANGE; i ++) {
	   ranges [2 * i] = i;
	   ranges [2 * i + 1] = -i;
	}

	for (j = 0; j < 2 * RANGE; j ++) {
	   i = T_u + ranges [j];
	   if (abs (fft_buffer [i % T_u]) < Min) {
              float a1  = arg (fft_buffer [(i + 1) % T_u] *
                               conj (fft_buffer [(i + 2) % T_u]));
	      float a3	= abs (arg (fft_buffer [(i + 1) % T_u] *
	      	                    conj (fft_buffer [(i + 3) % T_u])));
	      float a4	= abs (arg (fft_buffer [(i + 3) % T_u] *
	      	                    conj (fft_buffer [(i + 4) % T_u])));
	      float a5	= abs (arg (fft_buffer [(i + 4) % T_u] *
	      	                    conj (fft_buffer [(i + 5) % T_u])));
	      float a6	= abs (arg (fft_buffer [(i + 5) % T_u] *
	      	                    conj (fft_buffer [(i + 6) % T_u])));
	  
	      if ((abs (abs (a1 / M_PI) - 1) < 0.15) &&
	          (abs (a3) < 0.45) && (abs (a4) < 0.45) &&
	          (abs (a5) < 0.45) && (abs (a6) < 0.45)) {
                 index  = i;
                 Min    = abs (fft_buffer [i % T_u]);
              }
           }
	}

//	if (index != 100) {	// check on reasonability
//	   float a1	= arg (fft_buffer [(index + 1) % T_u] *
//	                      conj (fft_buffer [(index + 3) % T_u])) / M_PI;
//	   float a2	= arg (fft_buffer [(index + 3) % T_u] *
//	                      conj (fft_buffer [(index + 4) % T_u])) / M_PI;
//	   float a3	= arg (fft_buffer [(index + 4) % T_u] *
//	                      conj (fft_buffer [(index + 5) % T_u])) / M_PI;
//	   float a4	= arg (fft_buffer [(index + 5) % T_u] *
//	                      conj (fft_buffer [(index + 6) % T_u])) / M_PI;
//	   fprintf (stderr, " %d\t%f\t%f\t%f\t%f (%f %f)\n",
//	                    index < RANGE ? index : index - T_u,
//	                    abs (a1), abs (a2), abs (a3), abs (a4),
//	                    abs (arg (fft_buffer [(index + 1) % T_u] *
//	                         conj (fft_buffer [(index + 2) % T_u])) / M_PI),
//	                    abs (arg (fft_buffer [(index + 16 + 1) % T_u] *
//	                         conj (fft_buffer [(index + 16 + 2) % T_u])) / M_PI));
//	}

	snr		= 0.7 * snr + 0.3 * get_snr (fft_buffer);
	if (++snrCount > 10) {
	   show_snr (snr);
	   snrCount = 0;
	}

	if (index == 100)
	   return 100;
	return index - T_u;
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
//
//	and, from time to time we show some clouds.
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

