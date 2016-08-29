#
/*
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
 */
#
#include	"dab-constants.h"
#include	"msc-handler.h"
#include	"gui.h"
#include	"dab-virtual.h"
#include	"dab-concurrent.h"
#include	"msc-datagroup.h"
//
//	Driver program for processing the MSC.
//	Three operations here (apart from selecting
//	the local frame in the MSC vector)
//	1. deinterleaving
//	2. deconvolution (including depuncturing)
//	3. energy dispersal
//	4. in case of DAB: creating MP2 packets
//	5. in case of DAB+: assembling superframes and creating MP4 packets
//
//	The selected service(s) is (are) to be found in the
//	ficParameters.

#define	CUSize	(4 * 16)
//	Note CIF counts from 0 .. 3
//
		mscHandler::mscHandler	(RadioInterface *mr,
	                                 DabParams	*p,
	                                 audioSink	*sink,
	                                 uint8_t	concurrent) {
	myRadioInterface	= mr;
	our_audioSink		= sink;
	concurrencyOn 		= concurrent != 0;
	cifVector		= new int16_t [55296];
	cifCount		= 0;	// msc blocks in CIF
	blkCount		= 0;
	dabHandler		= new dabVirtual;
	newChannel		= false;
	work_to_be_done		= false;
	dabModus		= 0;
	mp2File			= NULL;	
	mp4File			= NULL;
	BitsperBlock		= 2 * p -> K;
	if (p -> dabMode == 4)	// 2 CIFS per 76 blocks
	   numberofblocksperCIF	= 36;
	else
	if (p -> dabMode == 1)	// 4 CIFS per 76 blocks
	   numberofblocksperCIF	= 18;
	else
	if (p -> dabMode == 2)	// 1 CIF per 76 blocks
	   numberofblocksperCIF	= 72;
	else			// shouldnot/cannot happen
	   numberofblocksperCIF	= 18;

	audioService		= true;		// default
}

		mscHandler::~mscHandler	(void) {
	delete	dabHandler;
//	delete[]  cifVector;
}

void	mscHandler::stop	(void) {
	work_to_be_done	= false;
	dabHandler	-> stop ();
}

//	Note, the set_xxx functions are called from within a
//	different thread than the process_mscBlock method,
//	so, a little bit of locking seems wise while
//	the actual changing of the settings is done in the
//	thread executing process_mscBlock
void	mscHandler::set_audioChannel (audiodata *d) {
	locker. lock ();
	audioService	= true;
	new_uepFlag	= d	-> uepFlag;
	new_startAddr	= d	-> startAddr;
	new_Length	= d	-> length;
	new_protLevel	= d	-> protLevel;
	new_bitRate	= d	-> bitRate;
	new_language	= d	-> language;
	new_type	= d	-> programType;
	new_ASCTy	= d	-> ASCTy;
	new_dabModus	= new_ASCTy == 077 ? DAB_PLUS : DAB;
	newChannel	= true;
	locker. unlock ();
}
//
void	mscHandler::set_dataChannel (packetdata	*d) {
	locker. lock ();
	audioService	= false;
	new_uepFlag	= d	-> uepFlag;
	new_startAddr	= d	-> startAddr;
	new_Length	= d	-> length;
	new_protLevel	= d	-> protLevel;
	new_DGflag	= d	-> DGflag;
	new_bitRate	= d	-> bitRate;
	new_FEC_scheme	= d	-> FEC_scheme;
	new_DSCTy	= d	-> DSCTy;
	new_packetAddress = d	-> packetAddress;
	newChannel	= true;
	locker. unlock ();
}

//
//	add blocks. First is (should be) block 4, last is (should be) 76
void	mscHandler::process_mscBlock	(int16_t *fbits,
	                                 int16_t blkno) { 
int16_t	currentblk;
int16_t	*myBegin;

	if (!work_to_be_done && !newChannel)
	   return;

	currentblk	= (blkno - 4) % numberofblocksperCIF;
//
//	we only change channel at the start of a new frame!!!
	if (newChannel) {
	   locker. lock ();
	   newChannel	= false;
	   dabHandler -> stopRunning ();
	   delete dabHandler;

	   if (audioService) {
	      dabHandler = new dabConcurrent (new_dabModus,
	                                      new_Length * CUSize,
	                                      new_bitRate,
	                                      new_uepFlag,
	                                      new_protLevel,
	                                      myRadioInterface,
	                                      mp2File,
	                                      mp4File,
	                                      our_audioSink);
	   }
	   else	 {	// dealing with data
	      dabHandler = new mscDatagroup (myRadioInterface,
	                                new_DSCTy,
	                                new_packetAddress,
	                                new_Length * CUSize,
	                                new_bitRate,
	                                new_uepFlag,
	                                new_protLevel,
	                                new_DGflag,
	                                new_FEC_scheme,
	                                false);
	      DSCTy		= new_DSCTy;
	      packetAddress	= new_packetAddress;
	      DGflag		= new_DGflag;
	      FEC_scheme	= new_FEC_scheme;
	   }
	          
	   startAddr	= new_startAddr;
	   Length	= new_Length;
	   work_to_be_done	= true;
	   locker. unlock ();
	}
//
//	and the normal operation is:
//	We might optimize this by realizing that the exact segment
//	we take from the CIF is known.
	memcpy (&cifVector [currentblk * BitsperBlock],
	                    fbits, BitsperBlock * sizeof (int16_t));
	if (currentblk < numberofblocksperCIF - 1) 
	   return;
//
//	OK, now we have a full CIF
	blkCount	= 0;
	cifCount	= (cifCount + 1) & 03;
	myBegin		= &cifVector [startAddr * CUSize];
//
//	Here we move the vector to be processed to a
//	separate task or separate function, depending on
//	the settings in the ini file, we might take advantage of multi cores
	(void) dabHandler -> process (myBegin, Length * CUSize);
}
//
//
int16_t	mscHandler::getLanguage (void) {
	return new_language;
}

int16_t	mscHandler::getType (void) {
	return new_type;
}

void	mscHandler::stopProcessing (void) {
	work_to_be_done	= false;
}

void	mscHandler::setFiles (FILE *f1, FILE *f2) {
	mp2File		= f1;
	mp4File		= f2;
	dabHandler	-> setFiles (f1, f2);
}

