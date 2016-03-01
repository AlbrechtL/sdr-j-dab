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
#define syncBufferMask	(2 * 32768 - 1)
#define MAXPRESENTCOUNTER 80

	ofdmProcessor::ofdmProcessor	(virtualInput *theRig,
	                                 DabParams	*p,
	                                 RadioInterface *mr,
	                                 mscHandler 	*msc,
	                                 ficHandler 	*fic,
	                                 int16_t	threshold,
#ifdef	HAVE_SPECTRUM
	                                 RingBuffer<DSPCOMPLEX> *spectrumBuffer,
#endif
	                                 RingBuffer<DSPCOMPLEX> *iqBuffer) {
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
	ibits				= new int16_t [2 * params -> K];
	ofdmSymbolCount			= 0;
	tokenCount			= 0;
	sampleCnt			= 0;
	tokenLength			= 0;
	avgTokenLength			= params -> T_F;
	phaseSynchronizer	= new phaseReference (params, threshold);
	my_ofdmDecoder		= new ofdmDecoder (params,
	                                           iqBuffer,
	                                           phaseSynchronizer -> getTable (),
	                                           myRadioInterface);
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
    connect (this, SIGNAL (setPresent (char)),
             myRadioInterface, SLOT (setSignalPresent (char)));

    connect (myRadioInterface, SIGNAL (FineOffsetSliderSignal(int)),
                  this, SLOT (FineOffsetSliderSlot (int)));

    connect (myRadioInterface, SIGNAL (SyncSetter(bool)),
                  this, SLOT (SyncSetter (bool)));

	bufferContent	= 0;
    SliderValue = 0;
    isSync = false;
    isSyncLast = false;
    isTimerIsRunning = false;
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
int32_t		presentCounter;
float		currentStrength;
int32_t		syncBufferIndex;
int32_t		syncBufferSize	= syncBufferMask + 1;
float		envBuffer	[syncBufferMask + 1];
float		signalLevel;

	running		= true;
	fineCorrector	= 0;
	sLevel		= 0;
	try {
//	first, we initialize to get the buffers filled
//
//	we first fill the buffer
initing:
	   syncBufferIndex	= 0;
	   signalLevel		= 0;
	   currentStrength	= 0;
	   for (i = 0; i < 10 * T_s; i ++) {
	      DSPCOMPLEX sample			= getSample (0);
	      envBuffer [syncBufferIndex]	= jan_abs (sample);
	      signalLevel			+= jan_abs (sample);
	   }
//
//	overrule the current sLevel value
	   sLevel	= signalLevel / (10 * T_s);

       DSPCOMPLEX signal_buffer[T_u];
       DSPCOMPLEX idealspectrum_buffer[T_u*2-1];
       DSPCOMPLEX correlation_buffer[T_u*2-1];
       float correlation_buffer_abs[T_u * 2 -1];
       float max_value = 0;
       //FILE * pFile;
       float sprectrum_eff = 0;
       float idealspectrum_eff = 0;
       int carriers = 1536;
       DSPCOMPLEX cmplx_zero(0,0);

       fftwf_plan SpectrumPlan	= fftwf_plan_dft_1d (T_u,
                                   reinterpret_cast <fftwf_complex *>(&signal_buffer),
                                   reinterpret_cast <fftwf_complex *>(&signal_buffer),
                                   FFTW_FORWARD, FFTW_ESTIMATE);

       fftwf_plan Spectrum2nPlan = fftwf_plan_dft_1d(T_u*2-1,
                                   reinterpret_cast <fftwf_complex *>(&correlation_buffer),
                                   reinterpret_cast <fftwf_complex *>(&correlation_buffer),
                                   FFTW_FORWARD, FFTW_ESTIMATE);

       fftwf_plan IdealSpectrum2nPlan = fftwf_plan_dft_1d(T_u*2-1,
                                  reinterpret_cast <fftwf_complex *>(&idealspectrum_buffer),
                                  reinterpret_cast <fftwf_complex *>(&idealspectrum_buffer),
                                  FFTW_FORWARD, FFTW_ESTIMATE);

       fftwf_plan CorrelationPlan = fftwf_plan_dft_1d(T_u*2-1,
                                  reinterpret_cast <fftwf_complex *>(&correlation_buffer),
                                  reinterpret_cast <fftwf_complex *>(&correlation_buffer),
                                  FFTW_BACKWARD, FFTW_ESTIMATE);

checkSignal:
       if(presentCounter!=0)
           fprintf(stderr,"checkSignal\n");

       setSynced (false);
       setPresent(false);

       presentCounter = 0;

       /*setlocale(LC_NUMERIC, "en_US.UTF-8");
       pFile = fopen ("data_singal.m","w");*/

       // Clear buffers
       memset(signal_buffer,0,sizeof(signal_buffer));
       memset(idealspectrum_buffer,0,sizeof(idealspectrum_buffer));
       memset(correlation_buffer,0,sizeof(correlation_buffer));

       //fprintf(pFile,"present=[");

       // Copy samples into fft buffer
       for (i = 0; i < T_u; i ++)
       {
          DSPCOMPLEX sample	= getSample (0);
          signal_buffer[i] = sample;
          //fprintf(pFile, "%f+%fi ...\n", real(sample), imag(sample));
       }
       //fprintf(pFile,"];");

       fftwf_execute(SpectrumPlan);

       // Filter the DC 0 Hz
       for(int i=0;i<10;i++) signal_buffer[i] = cmplx_zero;
       for(int i=T_u-10;i<T_u;i++) signal_buffer[i] = cmplx_zero;

       // Zero padding and reorder if first FFT
       for(int i=0;i<T_u;i++)
       {
          DSPCOMPLEX number(jan_abs(signal_buffer[(i + T_u/2) % (T_u)]),0);
          correlation_buffer[i] = number;
       }

       // Calculate the effective value of the zero padded correlation_buffer
       sprectrum_eff = 0;
       for(int i=0;i<T_u*2-1;i++)
           sprectrum_eff += jan_abs(correlation_buffer[i]) * jan_abs(correlation_buffer[i]);

       // Add an ideal spectrum into the idealspectrum_buffer
       for(int i=0;i<carriers;i++)
       {
          DSPCOMPLEX number(1,0);
          idealspectrum_buffer[(T_u - carriers)/2 + i] = number;
       }

       // Calculate the effective value of the zero padded idealspectrum_eff
       idealspectrum_eff = 0;
       for(int i=0;i<T_u*2-1;i++)
           idealspectrum_eff += jan_abs(idealspectrum_buffer[i]) * jan_abs(idealspectrum_buffer[i]);

     /*  fprintf(pFile,"signal_buffer=[");
       for(int i=0;i<T_u;i++)
       {
           fprintf(pFile, "%f+%fi ...\n", signal_buffer[i].real(), signal_buffer[i].imag());
       }
       fprintf(pFile,"];");

       fprintf(pFile,"idealspectrum_buffer=[");
       for(int i=0;i<T_u*2-1;i++)
       {
           fprintf(pFile, "%f+%fi ...\n", idealspectrum_buffer[i].real(), idealspectrum_buffer[i].imag());
       }
       fprintf(pFile,"];");
       fprintf(pFile,"correlation_buffer=[");
       for(int i=0;i<T_u*2-1;i++)
       {
           fprintf(pFile, "%f+%fi ...\n", correlation_buffer[i].real(), correlation_buffer[i].imag());
       }
       fprintf(pFile,"];");*/

       fftwf_execute(Spectrum2nPlan);
       fftwf_execute(IdealSpectrum2nPlan);

       // Do the cross correlation
       for (int i = 0; i < 2 * T_u - 1; i++)
       {
           correlation_buffer[i] = correlation_buffer[i] * conj(idealspectrum_buffer[i]);
       }

       fftwf_execute(CorrelationPlan);


      /* fprintf(pFile,"correlation_buffer2=[");
       for(int i=0;i<T_u*2-1;i++)
       {
           fprintf(pFile, "%f+%fi ...\n", correlation_buffer[i].real(), correlation_buffer[i].imag());
       }
       fprintf(pFile,"];");*/


       // Reorder data because the 0-frequency is middle after the FFT
       memset(correlation_buffer_abs,0,sizeof(correlation_buffer_abs));
       for (int i = 0; i < 2 * T_u - 1; i++)
       {
            // Make value absolute and reorder it
           correlation_buffer_abs[i] = jan_abs(correlation_buffer[(i + T_u) % (2 * T_u - 1)]);

           // Scale to the power
           correlation_buffer_abs[i]  /= sqrt(sprectrum_eff*idealspectrum_eff) * (T_u*2-1);
       }

      /* fprintf(pFile,"correlation_buffer_abs=[");
       for(int i=0;i<T_u*2-1;i++)
       {
           fprintf(pFile, "%f ...\n", correlation_buffer_abs[i]);
       }
       fprintf(pFile,"];");*/

       // Looking for maximum
       max_value = 0;
       for(int i=0;i<T_u * 2-1;i++)
       {
           if(correlation_buffer_abs[i] >= max_value)
               max_value = correlation_buffer_abs[i];
       }

       //fclose(pFile);

       // Check it peak of the cross corelation. 1 is maximum similarity, 0 is no similarity.
       // Experiments showed that 0.82 simiarity detects a real OFDM spectrum
       if(max_value < 0.82)
       {
           //msleep(250);
           goto checkSignal;
       }
       else
       {
           setPresent(true);
           fprintf(stderr,"max = %f\n",max_value);
       }

notSynced:
//	read in T_s samples for a next attempt;
	   for (i = 0; i < T_s; i ++) {
	      DSPCOMPLEX sample			= getSample (0);
	      envBuffer [syncBufferIndex]	= jan_abs (sample);
	      syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	   }

       currentStrength	= 0;
       for (i = syncBufferIndex - 50; i < syncBufferIndex; i ++)
          currentStrength += envBuffer [i];


//	we now have initial values for currentStrength and signalLevel
//	Then we look for a "dip" in the datastream
SyncOnNull:
	   counter	= 0;
	   setSynced (false);
static	DSPFLOAT	lowest	= 0;
	   while (currentStrength / 50  > 0.40 * sLevel) {
	      DSPCOMPLEX sample	=
	                      getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = jan_abs (sample);
//	update the levels
	      currentStrength += envBuffer [syncBufferIndex] -
	                      envBuffer [(syncBufferIndex + syncBufferSize - 50) & syncBufferMask];
	      syncBufferIndex =
	                      (syncBufferIndex + 1) & syncBufferMask;
	      counter ++;
	      if (counter > 2 * T_s)	// hopeless
          {
              presentCounter ++;
              if(presentCounter >  MAXPRESENTCOUNTER / 2)
                  goto checkSignal;
              else
                  goto notSynced;
          }
	   }
	   lowest	= currentStrength;

//	Now it is waiting for the end of the dip
SyncOnEndNull:
	   while (currentStrength / 50 < 0.75 * sLevel) {
	      DSPCOMPLEX sample = getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = abs (sample);
//	update the levels
	      currentStrength += envBuffer [syncBufferIndex] -
	                        envBuffer [(syncBufferIndex + syncBufferSize - 50) & syncBufferMask];
	      if (currentStrength < lowest)
	         lowest = currentStrength;
	      syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	      counter	++;
	      if (counter > 3 * T_s)	// hopeless
          {
              presentCounter ++;
              if(presentCounter >  MAXPRESENTCOUNTER / 3)
                  goto checkSignal;
              else
                  goto notSynced;
          }
	   }

SyncOnPhase:
//
//	here we also enter after having processed a full frame
//	now read in Tu samples
	   for (i = 0; i <  params -> T_u; i ++) {
	      ofdmBuffer [i] = getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = jan_abs (ofdmBuffer [i]);
//	update the levels
	      currentStrength += envBuffer [syncBufferIndex] -
	                              envBuffer [(syncBufferIndex  + syncBufferSize - 50) & syncBufferMask];
	      syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	      counter ++;
	   }
//
//	and then call upon the phase synchronizer to verify/identify
//	the real "first" sample of the new frame
	   startIndex = phaseSynchronizer ->
	                        findIndex (ofdmBuffer, params -> T_u);
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
	   my_ofdmDecoder  -> processBlock_0 (ofdmBuffer);
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
static int cnt = 0;
	   FreqCorr		= DSPCOMPLEX (0, 0);
       cnt=0;
	   for (ofdmSymbolCount = 2;
	        ofdmSymbolCount <= 4; ofdmSymbolCount ++) {
	      getSamples (ofdmBuffer, T_s, coarseCorrector + fineCorrector);
	      for (i = (int)T_u; i < (int)T_s; i ++) 
          {
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
             cnt++;
          }
	  
	      my_ofdmDecoder -> processToken (ofdmBuffer, ibits, ofdmSymbolCount);
	      my_ficHandler -> process_ficBlock (ibits, ofdmSymbolCount);
	   }

//	and similar for the (params -> L - 4) MSC blocks
//	   FreqCorr	= 0;
	   for (ofdmSymbolCount = 5;
	        ofdmSymbolCount <= params -> L;
	        ofdmSymbolCount ++) {
	      getSamples (ofdmBuffer, T_s, coarseCorrector + fineCorrector);
	      for (i = (int32_t)T_u; i < (int32_t)T_s; i ++) 
          {
             FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
             cnt++;
          }

	      my_ofdmDecoder -> processToken (ofdmBuffer, ibits, ofdmSymbolCount);
          my_mscHandler -> process_mscBlock (ibits, ofdmSymbolCount,isSync);
	   }
//
//	We integrate the error with the correction term
       float argFreqCorr = arg(FreqCorr);

       //if(isSync)
       {
        fineCorrector += 0.1 * argFreqCorr / M_PI * (params -> carrierDiff / 2);
       }
      /* else
       {
        //fineCorrector = SliderValue;
           fineCorrector += 10;
           if(fineCorrector >= 499)
               fineCorrector = -499;
       }*/
//
//	OK, at the end of the frame
//	assume everything went well and shift T_null samples
	   syncBufferIndex	= 0;
	   currentStrength	= 0;
	   for (i = 0; i < T_null - 50; i ++) {
	      DSPCOMPLEX sample	= getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex ++] = jan_abs (sample);
	   }
//
//	and a new  currentLevel
	   for (i = T_null - 50; i < T_null; i ++) {
	      DSPCOMPLEX sample	= getSample (coarseCorrector + fineCorrector);
	      envBuffer [syncBufferIndex] = jan_abs (sample);
	      currentStrength 	+= envBuffer [syncBufferIndex ++];
	   }
//
//	so, we skipped Tnull samples, so the real start should be T_g
//	samples away
	   counter	= 0;

static int waar	= 0;
static	int	corrector	= 0;
static	int	oldCorrection	= 0;

	   if ((++ waar > 6) && f2Correction) {
	      int correction	= my_ofdmDecoder -> coarseCorrector ();
	      if (oldCorrection == correction) {
	         correction += corrector;
	         corrector = -corrector;
	      }
	      oldCorrection = correction;
          coarseCorrector	+= correction * params -> carrierDiff;
	      waar = 0;
	   }

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
    setSynced (false);
    setPresent(false);
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

void	ofdmProcessor::stop		(void) {
	running		= false;
}


void    ofdmProcessor::FineOffsetSliderSlot(int value)
{
    //fineCorrector = value;
}

void    ofdmProcessor::SyncSetter(bool value)
{
    if(value)
    {
        QTimer::singleShot(5000, this, SLOT(Timeout()));
        isSync = true;
        isTimerIsRunning = true;
    }

    if(isTimerIsRunning)
        isSyncLast = value;
    else
        isSync = value;
}

void    ofdmProcessor::Timeout()
{
    isSync = isSyncLast;
    isTimerIsRunning = false;
}
