#
/*
 *    Copyright (C) 2013, 2014, 2015
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
 */
#include	"ofdm-processor.h"
#include	"gui.h"
//

	ofdmProcessor::ofdmProcessor	(virtualInput *theRig,
	                                 DabParams	*p,
	                                 RadioInterface *mr,
	                                 mscHandler 	*msc,
	                                 ficHandler 	*fic,
	                                 int16_t	threshold,
#ifdef	HAVE_SPECTRUM
	                                 RingBuffer<DSPCOMPLEX> *spectrumBuffer,
#endif
	                                 RingBuffer<DSPCOMPLEX> *iqBuffer,
	                                 uint8_t	freqSyncMethod) {
int32_t	i;
	this	-> theRig		= theRig;
	params				= p;
	this	-> T_null		= p	-> T_null;
	this	-> T_s			= p 	-> T_s;
	this	-> T_u			= p	-> T_u;
	this	-> T_g			= T_s - T_u;
	this	-> myRadioInterface	= mr;
	this	-> my_mscHandler	= msc;
	this	-> my_ficHandler	= fic;
	dumping				= false;
	dumpIndex			= 0;
//
	ofdmBuffer			= new DSPCOMPLEX [76 * T_s];
	ofdmBufferIndex			= 0;
	ofdmSymbolCount			= 0;
	tokenCount			= 0;
	sampleCnt			= 0;
	tokenLength			= 0;
	avgTokenLength			= params -> T_F;
	phaseSynchronizer	= new phaseReference (params, threshold);
	my_ofdmDecoder		= new ofdmDecoder (params,
	                                           iqBuffer,
	                                           phaseSynchronizer -> getTable (),
	                                           myRadioInterface,
	                                           freqSyncMethod);
	fineCorrector		= 0;	
	coarseCorrector		= 0;
	f2Correction		= false;
	oscillatorTable		= new DSPCOMPLEX [INPUT_RATE];
	localPhase		= 0;

	for (i = 0; i < INPUT_RATE; i ++)
	   oscillatorTable [i] = DSPCOMPLEX (cos (2.0 * M_PI * i / INPUT_RATE),
	                                     sin (2.0 * M_PI * i / INPUT_RATE));

	connect (this, SIGNAL (show_fineCorrector (int)),
	         myRadioInterface, SLOT (set_fineCorrectorDisplay (int)));
	connect (this, SIGNAL (show_coarseCorrector (int)),
	         myRadioInterface, SLOT (set_coarseCorrectorDisplay (int)));
	connect (this, SIGNAL (show_avgTokenLength (int)),
	         myRadioInterface, SLOT (set_avgTokenLengthDisplay (int)));
	connect (this, SIGNAL (setSynced (char)),
	         myRadioInterface, SLOT (setSynced (char)));

	bufferContent	= 0;
#ifdef	HAVE_SPECTRUM
	bufferSize	= 32768;
	this	-> spectrumBuffer	= spectrumBuffer;
	connect (this, SIGNAL (showSpectrum (int)),
	         mr, SLOT (showSpectrum (int)));
	localBuffer	= new DSPCOMPLEX [bufferSize];
	localCounter	= 0;
#endif
//
//	and we go
	start (QThread::TimeCriticalPriority);
}

	ofdmProcessor::~ofdmProcessor	(void) {
	running		= false;	// this will cause an
	                                // exception to be raised
	                        	// through the getSample functions
	while (!isFinished ()) {
	   msleep (1);
	   running = false;
	}
	delete	ofdmBuffer;
	delete	phaseSynchronizer;
	delete	my_ofdmDecoder;
	delete	oscillatorTable;
	disconnect (this, SIGNAL (show_fineCorrector (int)),
	            myRadioInterface, SLOT (set_fineCorrectorDisplay (int)));
	disconnect (this, SIGNAL (show_coarseCorrector (int)),
	            myRadioInterface, SLOT (set_coarseCorrectorDisplay (int)));
	disconnect (this, SIGNAL (show_avgTokenLength (int)),
	            myRadioInterface, SLOT (set_avgTokenLengthDisplay (int)));
}

//	In this implementation, we implement the "states"
//	of the state machine by using positions in the code
//
DSPCOMPLEX ofdmProcessor::getSample (int32_t phase) {
DSPCOMPLEX temp;
	if (!running)
	   throw 21;
	if (bufferContent == 0) {
	   bufferContent = theRig -> Samples ();
	   while ((bufferContent == 0) && running) {
	      usleep (10);
	      bufferContent = theRig -> Samples (); 
	   }
	}
	if (!running)	
	   throw 20;
//
//	so here, bufferContent > 0
	theRig -> getSamples (&temp, 1);
	bufferContent --;
	if (dumping) {
	   dumpBuffer [2 * dumpIndex] = real (temp);
	   dumpBuffer [2 * dumpIndex + 1] = imag (temp);
	   if ( ++dumpIndex >= DUMPSIZE / 2) {
	      sf_writef_float (dumpFile, dumpBuffer, dumpIndex);
	      dumpIndex = 0;
	   }
	}
//
//	OK, we have a sample!!
//	first: adjust frequency. We need Hz accuracy
#ifdef	HAVE_SPECTRUM
	if (localCounter < bufferSize) 
	   localBuffer [localCounter ++]	= temp;
#endif
	localPhase	-= phase;
	localPhase	= (localPhase + INPUT_RATE) % INPUT_RATE;
	temp		*= oscillatorTable [localPhase];
	sLevel		= 0.00001 * jan_abs (temp) + (1 - 0.00001) * sLevel;
#define	N	7
	sampleCnt	++;
	tokenLength	++;
	if (sampleCnt > INPUT_RATE / N) {
	   show_fineCorrector	(fineCorrector);
	   show_coarseCorrector	(coarseCorrector / KHz (1));
	   sampleCnt = 0;
#ifdef	HAVE_SPECTRUM
	   spectrumBuffer -> putDataIntoBuffer (localBuffer, bufferSize);
	   emit showSpectrum (bufferSize);
	   localCounter	= 0;
#endif
	}
	return temp;
}
//

void	ofdmProcessor::getSamples (DSPCOMPLEX *v, int16_t n, int32_t phase) {
int32_t		i;

	if (!running)
	   throw 21;
	if (n > bufferContent) {
	   bufferContent = theRig -> Samples ();
	   while ((bufferContent < n) && running) {
	      usleep (10);
	      bufferContent = theRig -> Samples (); 
	   }
	}
	if (!running)	
	   throw 20;
//
//	so here, bufferContent >= n
	n	= theRig -> getSamples (v, n);
	bufferContent -= n;
	if (dumping) {
	   for (i = 0; i < n; i ++) {
	      dumpBuffer [2 * dumpIndex] = real (v [i]);
	      dumpBuffer [2 * dumpIndex + 1] = imag (v [i]);
	      if (++dumpIndex >= DUMPSIZE / 2) {
	         sf_writef_float (dumpFile, dumpBuffer, dumpIndex);
	         dumpIndex = 0;
	      }
	   }
	}
//
//	OK, we have samples!!
//	first: adjust frequency. We need Hz accuracy
	for (i = 0; i < n; i ++) {
	   localPhase	-= phase;
	   localPhase	= (localPhase + INPUT_RATE) % INPUT_RATE;
#ifdef	HAVE_SPECTRUM
	   if (localCounter < bufferSize) 
	      localBuffer [localCounter ++]	= v [i];
#endif
	   v [i]	*= oscillatorTable [localPhase];
	   sLevel	= 0.00001 * jan_abs (v [i]) + (1 - 0.00001) * sLevel;
	}

	sampleCnt	+= n;
	tokenLength	+= n;
	if (sampleCnt > INPUT_RATE / N) {
	   show_fineCorrector	(fineCorrector);
	   show_coarseCorrector	(coarseCorrector / KHz (1));
	   sampleCnt = 0;
#ifdef	HAVE_SPECTRUM
	   spectrumBuffer -> putDataIntoBuffer (localBuffer, bufferSize);
	   emit showSpectrum (bufferSize);
	   localCounter	= 0;
#endif
	}
}

//	Note for the reader:
//	currentStrength should read (50 * currentStrength)
//
//	The main thread, reading sample and trying to identify
//	the ofdm frames
void	ofdmProcessor::run	(void) {
int32_t		startIndex;
int32_t		i;
DSPCOMPLEX	FreqCorr;
int32_t		counter;
float		currentStrength;
int32_t		syncBufferIndex	= 0;
int32_t		syncBufferSize	= 10 * T_s;
float		envBuffer	[syncBufferSize];
float		signalLevel;
int16_t		previous_1	= 1000;
int16_t		previous_2	= 999;
int16_t		ibits [2 * params -> K];

	running		= true;
	fineCorrector	= 0;
	sLevel		= 0;
	try {
//	first, we initialize to get the buffers filled
//
//	we first fill the buffer
Initing:
	   syncBufferIndex	= 0;
	   currentStrength	= 0;
	   sLevel		= 0;
	   for (i = 0; i < 10 * T_s; i ++) {
	      (void)jan_abs (getSample (0));	// to build up sLevel
	   }
//
notSynced:
//	read in T_s samples for a next attempt;
	   syncBufferIndex	= 0;
	   currentStrength	= 0;
	   for (i = 0; i < 50; i ++) {
              DSPCOMPLEX sample                 = getSample (0);
              envBuffer [syncBufferIndex]       = jan_abs (sample);
              currentStrength                   += envBuffer [syncBufferIndex];
              syncBufferIndex ++;
           }

//	we now have initial values for currentStrength and signalLevel
//	Then we look for a "dip" in the datastream
SyncOnNull:
	   counter	= 0;
	   setSynced (false);
	   while (currentStrength / 50  > 0.40 * sLevel) {
	      DSPCOMPLEX sample	=
	                      getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = jan_abs (sample);
//	update the levels
	      currentStrength += envBuffer [syncBufferIndex] -
	                      envBuffer [syncBufferIndex - 50];
	      syncBufferIndex ++;
	      counter ++;
	      if (counter > 2 * T_s)	// hopeless
	         goto notSynced;
	   }

//	Now it is waiting for the end of the dip
SyncOnEndNull:
	   while (currentStrength / 50 < 0.75 * sLevel) {
	      DSPCOMPLEX sample = getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = abs (sample);
//      update the levels
//      We constrain syncBufferIndex here to
//      a value smaller than 2 * T_s but larger than 50, to a value
//      smaller than 2 * T_s + T_null, which by itself is
//      smaller than the bufferSize;
              currentStrength += envBuffer [syncBufferIndex] -
                                 envBuffer [syncBufferIndex - 50];
              syncBufferIndex ++;
              counter   ++;
//
	      if (syncBufferIndex > 2 * T_s + T_null)	// hopeless
	         goto notSynced;
	   }
/**
  *     The end of the null period is identified, probably about 40
  *     samples earlier.
  */

SyncOnPhase:
//
//	here we also enter after having processed a full frame
//	now read in Tu samples
	   for (i = 0; i <  params -> T_u; i ++) 
	      ofdmBuffer [i] = getSample (coarseCorrector + fineCorrector);
//
//	and then call upon the phase synchronizer to verify/identify
//	the real "first" sample of the new frame
	   startIndex = phaseSynchronizer -> findIndex (ofdmBuffer);
	   if (startIndex < 0) { // no sync, try again
//	      fprintf (stderr, "startindex fout op %d\n", startIndex);
	      goto notSynced;
	   }
//
//	Once here, we know the correlator did its work and found a
//	decent first element.
//	   fprintf (stderr, "counter = %d\n", counter - T_u + startIndex);
//	Once here, we are synchronized, we need to copy the data we
//	used for synchronization for block 0
	   memmove (ofdmBuffer, &ofdmBuffer [startIndex],
	                    (params -> T_u - startIndex) * sizeof (DSPCOMPLEX));
	   ofdmBufferIndex	= params -> T_u - startIndex;

//	We now have a reference for computing the tokenLength
//	
	   tokenLength = tokenLength - (params -> T_u - startIndex);
	   if (0.9 * params -> T_F <= tokenLength &&
	                tokenLength <= 1.1 * params -> T_F)
	      avgTokenLength = 0.8 * avgTokenLength + 0.2 * tokenLength;

	   if (++tokenCount > 10) {
	      tokenCount = 0;
	      show_avgTokenLength (avgTokenLength);
	   }
//	and for the next round
	   tokenLength		= params -> T_u - startIndex;
//	but we are also ready to  actually process block_0

OFDM_PRS:
//	read the missing samples in the ofdmBuffer
	   setSynced (true);
	   getSamples (&ofdmBuffer [ofdmBufferIndex],
	                     T_u - ofdmBufferIndex, coarseCorrector + fineCorrector);
//
//	block0 will set the phase reference for further decoding
//	and - if the flag is set - will compute an estimate of the
//	frequency offset
	int16_t correction =
	          my_ofdmDecoder  -> processBlock_0 (ofdmBuffer, f2Correction);

	if (f2Correction) {
	   if ((correction == 0) && (previous_1 == 0) && (previous_2 == 0))
	      f2Correction = false;
	   else
	   if (correction != 100) {	// finding an offset succeeded
	      coarseCorrector	+= correction * params -> carrierDiff;
	      if (abs (coarseCorrector) > Khz (35))
	         coarseCorrector = 0;
	      previous_2	= previous_1;
	      previous_1	= correction;
	   }
	}
//
//	after block 0, we will just read in the other (params -> L - 1) blocks
//	we do it in two steps:
//	1. the fick blocks, we read them in raw and determine
//	   the frequency offset
//	2. the whole bunch of MSC blocks

OFDM_SYMBOLS:
//	The first ones are the FIC blocks
//	We try to get an estimate of the fine error by looking
//	at the phase difference between prefix and real data
//	we average over the whole frame

	   FreqCorr		= DSPCOMPLEX (0, 0);
	   for (ofdmSymbolCount = 1;
	        ofdmSymbolCount < 4; ofdmSymbolCount ++) {
	      getSamples (ofdmBuffer, T_s, coarseCorrector + fineCorrector);
	      for (i = (int)T_u; i < (int)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	  
	      my_ofdmDecoder -> processToken (ofdmBuffer, ibits, ofdmSymbolCount);
	      my_ficHandler -> process_ficBlock (ibits, ofdmSymbolCount);
	   }

//	and similar for the (params -> L - 4) MSC blocks
//	   FreqCorr	= 0;
	   for (ofdmSymbolCount = 4;
	        (uint16_t)ofdmSymbolCount < (uint16_t)(params -> L);
	        ofdmSymbolCount ++) {
	      getSamples (ofdmBuffer, T_s, coarseCorrector + fineCorrector);
	      for (i = (int32_t)T_u; i < (int32_t)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);

	      my_ofdmDecoder -> processToken (ofdmBuffer, ibits, ofdmSymbolCount);
	      my_mscHandler -> process_mscBlock (ibits, ofdmSymbolCount);
	   }
//
//	We integrate the error with the correction term
	   fineCorrector +=
	         0.1 * arg (FreqCorr) / M_PI * (params -> carrierDiff / 2);
//
//	OK, at the end of the frame
//	assume everything went well and shift T_null samples
	   syncBufferIndex	= 0;
	   currentStrength	= 0;
	   counter		= 0;
	   getSamples (ofdmBuffer, T_null, coarseCorrector + fineCorrector);
//
//	so, we skipped Tnull samples, so the real start should be T_g
//	samples away

	   if (fineCorrector > params -> carrierDiff / 2) {
	      coarseCorrector += params -> carrierDiff;
	      fineCorrector -= params -> carrierDiff;
	   }
	   else
	   if (fineCorrector < -params -> carrierDiff / 2) {
	      coarseCorrector -= params -> carrierDiff;
	      fineCorrector += params -> carrierDiff;
	   }
//	and off we go, up to the next frame
	   goto SyncOnPhase;
	}
	catch (int e) {
	   fprintf (stderr, "ofdmProcessor is terminating\n");
	}
}

void	ofdmProcessor:: reset	(void) {
	fineCorrector = coarseCorrector = 0;
	f2Correction	= true;
}

void	ofdmProcessor::stop		(void) {
	running		= false;
}

void	ofdmProcessor::startDumping	(SNDFILE *f) {
	if (dumping)
	   return;
//	do not change the order here.
	dumpFile 	= f;
	dumping		= true;
	dumpIndex	= 0;
}

void	ofdmProcessor::stopDumping	(void) {
	dumping = false;
}
//

void	ofdmProcessor::coarseCorrectorOn (void) {
	f2Correction 	= true;
	coarseCorrector	= 0;
}

void	ofdmProcessor::coarseCorrectorOff (void) {
	f2Correction	= false;
}

