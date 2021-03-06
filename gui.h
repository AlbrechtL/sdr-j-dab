#
/*
 *    Copyright (C)  2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
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

#ifndef _GUI
#define _GUI

#include	"dab-constants.h"
#include	<QDialog>
#include	<QHostAddress>
#include	<QUdpSocket>
#include	"ui_sdrgui.h"
#include	<QTimer>
#include	<sndfile.h>
#include	<QWheelEvent>
#include	<QStringList>
#include	<QStringListModel>
#include	"scope.h"
#include	"ofdm-processor.h"
#include	"iqdisplay.h"
#include	"ringbuffer.h"
#ifdef	HAVE_SPECTRUM
#include	"spectrum-handler.h"
#endif
class	QSettings;
class	Scope;
class	virtualInput;
class	audioSink;

class	mscHandler;
class	ficHandler;

class	common_fft;

#define	HFSPECTRUM	0200
#define	LFSPECTRUM	0201
/*
 *	GThe main gui object. It inherits from
 *	QDialog and the generated form
 */
class RadioInterface: public QDialog,
		      private Ui_sdr_j_dab {
Q_OBJECT
public:
		RadioInterface		(QSettings	*,
	                                 uint8_t,
	                                 QWidget *parent = NULL);
		~RadioInterface		(void);

private:
	QSettings		*dabSettings;
	uint8_t			freqSyncMethod;
	int16_t			outputDevice;
	void			dumpControlState	(QSettings *);
	bool			sourceDumping;
	SNDFILE			*dumpfilePointer;
	bool			audioDumping;
	SNDFILE			*audiofilePointer;
	int16_t			threshold;
	DabParams		dabModeParameters;
	void			setModeParameters	(int16_t);
	QStringListModel	ensemble;
	QStringList		Services;
#ifdef	HAVE_SPECTRUM
	RingBuffer<DSPCOMPLEX>	*spectrumBuffer;
	spectrumhandler		*spectrumHandler;
#endif
	uint8_t			isSynced;
	int32_t			outRate;
	int32_t			iqDisplaysize;
	IQDisplay		*myIQDisplay;
	RingBuffer<DSPCOMPLEX> *iqBuffer;

	void			setupChannels		(QComboBox *, uint8_t);
	uint8_t			dabBand;
	bool			running;

	QString			ensembleLabel;
	virtualInput		*myRig;

	void			setTuner		(int32_t);
	QTimer			*displayTimer;
	void			resetSelector		(void);
	int16_t			ficBlocks;
	int16_t			ficSuccess;

	bool			spectrumWaterfall;
	ofdmProcessor		*the_ofdmProcessor;
	ficHandler		*my_ficHandler;
	mscHandler		*my_mscHandler;
	audioSink		*our_audioSink;
	int32_t			TunedFrequency;
	int32_t			vfoFrequency;
	bool			autoCorrector;
	FILE			*mp2File;
	FILE			*mp4File;
	int16_t			scopeWidth;
	QLabel			*pictureLabel;
	QUdpSocket		DSCTy_59_socket;

	void			Increment_Channel	(void);
	bool			scanning;
private slots:
	void			set_Scanning		(void);
	void			setStart	(void);
	void			updateTimeDisplay	(void);
	void			setScopeWidth		(int);

	void			selectMode		(const QString &);
	void			hard_Reset		(void);

	void			abortSystem		(int);
	void			TerminateProcess	(void);
	void			set_bandSelect		(QString);
	void			set_channelSelect	(QString);
	void			setDevice		(QString);
	void			selectService		(QModelIndex);
	void			set_dumping		(void);
	void			set_mp2File		(void);
	void			set_mp4File		(void);
	void			set_audioDump		(void);
public slots:
	void			set_fineCorrectorDisplay	(int);
	void			set_coarseCorrectorDisplay	(int);
	void			set_avgTokenLengthDisplay	(int);
	void			clearEnsemble		(void);
	void			addtoEnsemble		(const QString &);
	void			nameofEnsemble		(int, const QString &);
	void			addEnsembleChar		(char, int);
	void			show_successRate	(int);
	void			show_ficCRC		(bool);
	void			show_snr		(int);
	void			showIQ			(int);
	void			setSynced		(char);
	void			No_Signal_Found		(void);
	void			Yes_Signal_Found	(void);
	void			showLabel		(QString);
	void			showMOT			(QByteArray, int);
	void			send_datagram		(char *, int);
        void			show_mscErrors          (int);
        void			show_ipErrors           (int);
#ifdef	HAVE_SPECTRUM
	void			showSpectrum		(int);
	void			set_spectrumHandler	(void);
private:
	bool			spectrumisShown;
#endif
};

#endif

