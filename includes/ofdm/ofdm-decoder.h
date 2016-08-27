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
#ifndef	__OFDM_DECODER
#define	__OFDM_DECODER

#include	<stdint.h>
#include	"dab-constants.h"
#include	"fft.h"
#include	"mapper.h"
#include	"phasetable.h"
#include	<QObject>
#include	"ringbuffer.h"

class	RadioInterface;

class	ofdmDecoder: public QObject{
Q_OBJECT
public:
		ofdmDecoder		(DabParams *,
	                                 RingBuffer<DSPCOMPLEX> *,
	                                 DSPCOMPLEX	*,
	                                 RadioInterface *,
	                                 uint8_t);
		~ofdmDecoder		(void);
	int16_t	processBlock_0		(DSPCOMPLEX *, bool);
	void	processToken		(DSPCOMPLEX *, int16_t *, int32_t n);
	int16_t	get_snr			(DSPCOMPLEX *);
	int16_t	getStrength		(void);
	void	set_displayToken	(int16_t);
private:
	int16_t	getMiddle		(DSPCOMPLEX *);
	DabParams	*params;
	RingBuffer<DSPCOMPLEX> *iqBuffer;
	DSPCOMPLEX	*refTable;
	uint8_t		freqSyncMethod;
	RadioInterface	*myRadioInterface;
	int32_t		T_s;
	int32_t		T_u;
	int32_t		carriers;
	int32_t	delta;
	DSPCOMPLEX	*phaseReference;
	common_fft	*fft_handler;
	DSPCOMPLEX	*fft_buffer;
	permVector	*myMapper;
	phaseTable	*phasetable;
	int16_t		iqCount;
	int16_t		displayToken;
	int16_t		snrCount;
	int16_t		snr;
	int16_t		strength;
	int16_t		newStrength	(DSPCOMPLEX *);
	float		*correlationVector;
	float		*refArg;
	int32_t		blockIndex;
signals:
	void		show_snr	(int);
	void		showIQ		(int);
};

#endif


