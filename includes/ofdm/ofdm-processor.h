#
/*
 *
 *    Copyright (C) 2013
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
 */
#
#ifndef	__OFDM_PROCESSOR__
#define	__OFDM_PROCESSOR__
/*
 *
 */
#include	"dab-constants.h"
#include	<QThread>
#include	<QObject>
#include	<sndfile.h>
#include	"stdint.h"
#include	"phasereference.h"
#include	"ofdm-decoder.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"virtual-input.h"
#include	"ringbuffer.h"

#define		DUMPSIZE	8192
class	RadioInterface;
class	common_fft;

class ofdmProcessor: public QThread {
Q_OBJECT
public:
		ofdmProcessor	(virtualInput *,
	                         DabParams	*,
	                         RadioInterface *,
	                         mscHandler *,
	                         ficHandler *,
	                         int16_t,
#ifdef	HAVE_SPECTRUM
	                         RingBuffer<DSPCOMPLEX> *,
#endif
	                         RingBuffer<DSPCOMPLEX> *,
	                         uint8_t);	// passing
		~ofdmProcessor		(void);
	void	reset			(void);
	void	stop			(void);
	void	setOffset		(int32_t);
	void	coarseCorrectorOn	(void);
	void	coarseCorrectorOff	(void);
	void	startDumping		(SNDFILE *);
	void	stopDumping		(void);
private:
	bool		running;
	uint8_t		freqSyncMethod;
	int16_t		gain;
	bool		dumping;
	int16_t		dumpIndex;
	float		dumpBuffer [DUMPSIZE];
	SNDFILE		*dumpFile;
	virtualInput	*theRig;
	DabParams	*params;
	int32_t		T_null;
	int32_t		T_u;
	int32_t		T_s;
	int32_t		T_g;
	float		sLevel;
	RadioInterface	*myRadioInterface;
	DSPCOMPLEX	*oscillatorTable;
	int32_t		localPhase;
	int16_t		fineCorrector;
	int32_t		coarseCorrector;
	bool		f2Correction;
	int32_t		tokenLength;
	int32_t		tokenCount;
	int32_t		avgTokenLength;
	int16_t		*ibits;
	DSPCOMPLEX	*ofdmBuffer;
	uint32_t	ofdmBufferIndex;
	uint32_t	ofdmSymbolCount;
	phaseReference	*phaseSynchronizer;
	ofdmDecoder	*my_ofdmDecoder;
	DSPFLOAT	avgCorr;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscHandler;
	int32_t		sampleCnt;
	int32_t		inputSize;
	int32_t		inputPointer;
	DSPCOMPLEX	getSample	(int32_t);
	void		getSamples	(DSPCOMPLEX *, int16_t, int32_t);
//	void		getSamples	(DSPCOMPLEX *, int16_t);
virtual	void		run		(void);
	int32_t		bufferContent;
#ifdef	HAVE_SPECTRUM
	RingBuffer<DSPCOMPLEX> *spectrumBuffer;
	int32_t		bufferSize;
	int32_t		localCounter;
	DSPCOMPLEX	*localBuffer;
signals:
	void		showSpectrum		(int);
#endif
signals:
	void		show_fineCorrector	(int);
	void		show_coarseCorrector	(int);
	void		show_avgTokenLength	(int);
	void		setSynced		(char);
};
#endif

