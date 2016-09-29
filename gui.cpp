#
/*
 *    Copyright (C) 2013, 2014, 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  SDR-J (JSDR).
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are acknowledged.
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
#include	<iostream>
#include	<QSettings>
#include	<QMessageBox>
#include	<QFileDialog>
#include	<QDebug>
#include	<QDateTime>
#include	<QFile>
#include	<QStringList>
#include	<QStringListModel>
#include	"dab-constants.h"
#include	"gui.h"
#include	"scope.h"
#include	"audiosink.h"
#include	"ofdm-processor.h"
#include	"fft.h"
#include	"rawfiles.h"
#include	"wavfiles.h"
#ifdef	HAVE_DABSTICK
#include	"dabstick.h"
#endif
#ifdef	HAVE_SDRPLAY
#include	"sdrplay.h"
#endif
#ifdef	HAVE_DONGLE
#include	"mirics-dongle.h"
#endif
#ifdef	HAVE_UHD
#include	"uhd-input.h"
#endif
#ifdef	HAVE_EXTIO
#include	"extio-handler.h"
#endif
#ifdef	HAVE_RTL_TCP
#include	"rtl_tcp_client.h"
#endif
#ifdef	HAVE_AIRSPY
#include	"airspy-handler.h"
#endif
#ifdef	HAVE_SPECTRUM
#include	"spectrum-handler.h"
#endif

#define		BAND_III	0100
#define		L_BAND		0101
static
const char *get_programm_type_string (uint8_t type);
static
const char *get_programm_language_string (uint8_t language);
/*
 *	We use the creation function merely to set up the
 *	user interface and make the connections between the
 *	gui elements and the handling agents. All real action
 *	is embedded in actions, initiated by gui buttons
 */
	RadioInterface::RadioInterface (QSettings	*Si,
	                                uint8_t		freqSyncMethod,
	                                QWidget		*parent):
	                                                   QDialog (parent) {
int16_t	k;

// 	the setup for the generated part of the ui
	setupUi (this);
	dabSettings		= Si;
	this	-> freqSyncMethod	= freqSyncMethod;
//
//	Before printing anything, we set
	setlocale (LC_ALL, "");
//	The default:
	myRig			= new virtualInput ();
	running			= false;
	autoCorrector 		= true;
	scanning		= false;
	threshold		=
	           dabSettings -> value ("threshold", 3). toInt ();
//	Note that the generation of values to be displayed
//	is in a separate thread, we need a buffer for communication
//	and a signal/slot combination for triggering

	isSynced		= UNSYNCED;
	syncedLabel		->
	               setStyleSheet ("QLabel {background-color : red}");
	syncedLabel		-> setToolTip (QString ("green  means time synchronization"));

	iqBuffer		= new RingBuffer<DSPCOMPLEX> (2 * 1536);
	iqDisplaysize		=
	               dabSettings -> value ("iqDisplaysize", 2 * 256). toInt ();
	myIQDisplay		= new IQDisplay (iqDisplay, iqDisplaysize);
	iqDisplay		-> setToolTip (QString ("Constellation of data points"));
//
	TunedFrequency		= MHz (200);	// any value will do
	outRate			= 48000;	// not used
//
//	our_audioSink will initialize the values for the
//	streamOutSelector
	our_audioSink		= new audioSink		(streamOutSelector);

#ifdef	HAVE_SPECTRUM
	spectrumBuffer		= new RingBuffer<DSPCOMPLEX> (2 * 32768);
	spectrumHandler	= new spectrumhandler (this, dabSettings, spectrumBuffer);
	spectrumHandler	-> show ();
	spectrumButton	-> show ();
	spectrumisShown	= true;
#else
	spectrumButton	-> hide ();
#endif

//	We will not bore our users with entries for which there is
//	no device
#ifdef	HAVE_SDRPLAY
	deviceSelector	-> addItem ("sdrplay");
#endif
#ifdef	HAVE_DABSTICK
	deviceSelector	-> addItem ("dabstick");
#endif
#ifdef	HAVE_AIRSPY
	deviceSelector	-> addItem ("airspy");
#endif
#ifdef HAVE_UHD
	deviceSelector	-> addItem ("UHD");
#endif
#ifdef HAVE_EXTIO
	deviceSelector	-> addItem ("extio");
#endif
#ifdef	HAVE_RTL_TCP
	deviceSelector	-> addItem ("rtl_tcp");
#endif
	our_audioSink	-> selectDefaultDevice ();
//	Thanks to David Malone, who discovered while porting to OSX that
//	these initializations should NOT be forgotten.
	mp2File		= NULL;
	mp4File		= NULL;
	ficBlocks	= 0;
	ficSuccess	= 0;

	setModeParameters (1);
	my_mscHandler		= new mscHandler	(this,
	                                                 &dabModeParameters,
	                                                 our_audioSink,
	                                                 true);
	my_ficHandler		= new ficHandler	(this,
	                                                 2 * dabModeParameters. K);
//
//	the default is:
	the_ofdmProcessor = new ofdmProcessor (myRig,
	                                       &dabModeParameters,
	                                       this,
	                                       my_mscHandler,
	                                       my_ficHandler,
	                                       threshold,
#ifdef	HAVE_SPECTRUM
	                                       spectrumBuffer,
#endif
	                                       iqBuffer,
	                                       freqSyncMethod);
//
	ensemble.setStringList (Services);
	ensembleDisplay	-> setModel (&ensemble);
	Services << " ";
	ensemble. setStringList (Services);
	ensembleDisplay	-> setModel (&ensemble);

	dabBand		= BAND_III;
	setupChannels	(channelSelector, dabBand);
//
//	The connects of the GUI to the handlers
	connect (ensembleDisplay, SIGNAL (clicked (QModelIndex)),
	              this, SLOT (selectService (QModelIndex)));
	connect (ensembleDisplay, SIGNAL (activated (QModelIndex)),
	              this, SLOT (selectService (QModelIndex)));
	connect	(modeSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (selectMode (const QString &)));
	connect (startButton, SIGNAL (clicked (void)),
	              this, SLOT (setStart (void)));
	connect (QuitButton, SIGNAL (clicked ()),
	              this, SLOT (TerminateProcess (void)));
	connect (deviceSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (setDevice (const QString &)));
	connect (channelSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (set_channelSelect (const QString &)));
	connect (bandSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (set_bandSelect (const QString &)));
	connect (scopeSlider, SIGNAL (valueChanged (int) ),
		      this, SLOT (setScopeWidth (int) ) );
	connect (dumpButton, SIGNAL (clicked (void)),
	              this, SLOT (set_dumping (void)));
	connect (mp2fileButton, SIGNAL (clicked (void)),
	              this, SLOT (set_mp2File (void)));
	connect (aacfileButton, SIGNAL (clicked (void)),
	              this, SLOT (set_mp4File (void)));
	connect (audioDump, SIGNAL (clicked (void)),
	              this, SLOT (set_audioDump (void)));
	connect (resetButton, SIGNAL (clicked (void)),
	              this, SLOT (hard_Reset (void)));
	connect (scanButton, SIGNAL (clicked (void)),
	         this, SLOT (set_Scanning (void)));
	scanButton	-> setToolTip (QString ("Experimental feature, scans over the signals in the current band.\n Scanning will stop as soon as a signal is detected"));
#ifdef	HAVE_SPECTRUM
	connect (spectrumButton, SIGNAL (clicked (void)),
	              this, SLOT (set_spectrumHandler (void)));
#endif
//
//	Timers
	displayTimer		= new QTimer ();
	displayTimer		-> setInterval (1000);
	connect (displayTimer,
	         SIGNAL (timeout (void)),
	         this,
	         SLOT (updateTimeDisplay (void)));
//
	setScopeWidth		(scopeSlider	 -> value ());
	sourceDumping		= false;
	dumpfilePointer		= NULL;
	audioDumping		= false;
	audiofilePointer	= NULL;
	sampleRateDisplay 	-> display (INPUT_RATE);
//
//	some settings may be influenced by the ini file
//	the default device is the "no device", which happens to
//	be a device
	setDevice 		(deviceSelector 	-> currentText ());
	QString h		=
	           dabSettings -> value ("device", "no device"). toString ();
	k		= deviceSelector -> findText (h);
	if (k != -1) {
	   deviceSelector	-> setCurrentIndex (k);
	   setDevice 		(deviceSelector 	-> currentText ());
	}
	h		= dabSettings -> value ("channel", "12C"). toString ();
	k		= channelSelector -> findText (h);
	if (k != -1) {
	   channelSelector -> setCurrentIndex (k);
	   set_channelSelect (h);
	}
	
//	display the version
	QString v = "sdr-j DAB(+)  " ;
	v. append (CURRENT_VERSION);
	versionName	-> setText (v);
//	and start the timer
	displayTimer		-> start (1000);
//
	pictureLabel		= NULL;
}

	RadioInterface::~RadioInterface () {
}
//
//	at the end, save the values used
void	RadioInterface::dumpControlState (QSettings *s) {
	if (s == NULL)	// cannot happen
	   return;

	s	-> setValue ("band", bandSelector -> currentText ());
	s	-> setValue ("channel",
	                      channelSelector -> currentText ());
	s	-> setValue ("device", deviceSelector -> currentText ());
	s	-> setValue ("iqDisplaysize", iqDisplaysize);
}
//
//	On start, we ensure that the streams are stopped so
//	that they can be restarted again.
void	RadioInterface::setStart	(void) {
bool	r = 0;
	if (running)		// only listen when not running yet
	   return;
//
//	All modules are waiting for input, so let us give that to them.
	r = myRig		-> restartReader ();
	qDebug ("Starting %d\n", r);
	if (!r) {
	   QMessageBox::warning (this, tr ("sdr"),
	                               tr ("Opening  input stream failed\n"));
	   return;
	}

	clearEnsemble ();		// the display
//
//	This does not hurt
	our_audioSink	-> restart ();
	running = true;
}

void	RadioInterface::TerminateProcess (void) {
	displayTimer	-> stop ();
	if (sourceDumping) {
	   the_ofdmProcessor	-> stopDumping ();
	   sf_close (dumpfilePointer);
	}

	if (audioDumping) {
	   our_audioSink	-> stopDumping ();
	   sf_close (audiofilePointer);
	}

	if (mp2File != NULL) {
	   my_mscHandler	-> setFiles (NULL, NULL);
	   fclose (mp2File);
	}

	if (mp4File != NULL) {
	   my_mscHandler	-> setFiles (NULL, NULL);
	   fclose (mp4File);
	}
//

	my_mscHandler	-> stop	();		// might be concurrent
	my_ficHandler	-> stop	();
	the_ofdmProcessor -> stop ();		// definitely concurrent
	myRig		-> stopReader ();	// may contain concurrency
	our_audioSink	-> stop ();		// concurrent
//
//	The whole system should now have come to a kind of comatose state
//
	dumpControlState (dabSettings);
	delete		the_ofdmProcessor;
	delete		my_ficHandler;
	delete		my_mscHandler;
	delete		myRig;
	delete		our_audioSink;
#ifdef	HAVE_SPECTRUM
	delete spectrumHandler;			// may have pending signals
	spectrumHandler	= NULL;
	fprintf (stderr, "the spectrumHandler is gone\n");
#endif
	accept ();
	if (pictureLabel != NULL)
	   delete pictureLabel;			// there might be pending ops
	pictureLabel	= NULL;
	fprintf (stderr, "Termination started");
//	delete		displayTimer;
}

void	RadioInterface::abortSystem (int d) {
	qDebug ("aborting for reason %d\n", d);
	accept ();
}
//
void	RadioInterface::setScopeWidth (int n) {
	scopeWidth		= n;
}
//
void	RadioInterface::setTuner (int32_t n) {
	myRig		-> setVFOFrequency (n);
	vfoFrequency	= n;
}

void	RadioInterface::updateTimeDisplay (void) {
QDateTime	currentTime = QDateTime::currentDateTime ();
	timeDisplay	-> setText (currentTime.
	                            toString (QString ("dd.MM.yy:hh:mm:ss")));
}
//
void	RadioInterface::set_fineCorrectorDisplay (int v) {
	fineCorrectorDisplay	-> display (v);
}

void	RadioInterface::set_coarseCorrectorDisplay (int v) {
	coarseCorrectorDisplay	-> display (v);
}

void	RadioInterface::set_avgTokenLengthDisplay (int n) {
	avgTokenLengthDisplay -> display (n);
}
//

struct dabFrequencies {
	const char	*key;
	int	fKHz;
};

struct dabFrequencies bandIII_frequencies [] = {
{"5A",	174928},
{"5B",	176640},
{"5C",	178352},
{"5D",	180064},
{"6A",	181936},
{"6B",	183648},
{"6C",	185360},
{"6D",	187072},
{"7A",	188928},
{"7B",	190640},
{"7C",	192352},
{"7D",	194064},
{"8A",	195936},
{"8B",	197648},
{"8C",	199360},
{"8D",	201072},
{"9A",	202928},
{"9B",	204640},
{"9C",	206352},
{"9D",	208064},
{"10A",	209936},
{"10B", 211648},
{"10C", 213360},
{"10D", 215072},
{"11A", 216928},
{"11B",	218640},
{"11C",	220352},
{"11D",	222064},
{"12A",	223936},
{"12B",	225648},
{"12C",	227360},
{"12D",	229072},
{"13A",	230748},
{"13B",	232496},
{"13C",	234208},
{"13D",	235776},
{"13E",	237488},
{"13F",	239200},
{NULL, 0}
};

struct dabFrequencies Lband_frequencies [] = {
{"LA", 1452960},
{"LB", 1454672},
{"LC", 1456384},
{"LD", 1458096},
{"LE", 1459808},
{"LF", 1461520},
{"LG", 1463232},
{"LH", 1464944},
{"LI", 1466656},
{"LJ", 1468368},
{"LK", 1470080},
{"LL", 1471792},
{"LM", 1473504},
{"LN", 1475216},
{"LO", 1476928},
{"LP", 1478640},
{NULL, 0}
};

void	RadioInterface::setupChannels (QComboBox *s, uint8_t band) {
struct dabFrequencies *t;
int16_t	i;
int16_t	c	= s -> count ();

	for (i = 0; i < c; i ++)
	   s	-> removeItem (c - (i + 1));

	if (band == BAND_III)
	   t = bandIII_frequencies;
	else
	   t = Lband_frequencies;

	for (i = 0; t [i]. key != NULL; i ++)
	   s -> insertItem (i, t [i]. key, QVariant (i));
}
//
void	RadioInterface::set_bandSelect (QString s) {
	if (running) {
	   running	= false;
	   myRig	-> stopReader ();
	   myRig	-> resetBuffer ();
	   our_audioSink	-> stop ();
	   usleep (100);
	   clearEnsemble ();
	}

	if (s == "BAND III")
	   dabBand	= BAND_III;
	else
	   dabBand	= L_BAND;
	setupChannels (channelSelector, dabBand);
}

void	RadioInterface::set_channelSelect (QString s) {
int16_t	i;
struct dabFrequencies *finger;
bool	localRunning	= running;

	if (scanning)
	   set_Scanning ();	// switch it off

	if (localRunning) {
	   our_audioSink	-> stop ();
	   myRig		-> stopReader ();
	   myRig		-> resetBuffer ();
	}
	Services = QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay		-> setModel (&ensemble);
	my_ficHandler		-> clearEnsemble ();

	ensembleLabel		= QString ();
	ensembleName		-> setText (ensembleLabel);
	
//	Then the various displayed items
	programName		-> setText ("   ");
	ensembleName		-> setText ("   ");
	errorDisplay		-> display (0);
	ficRatioDisplay		-> display (0);
	snrDisplay		-> display (0);
	uepFlagDisplay		-> display (0);
	startAddrDisplay	-> display (0);
	LengthDisplay		-> display (0);
	protLevelDisplay	-> display (0);
	bitRateDisplay		-> display (0);
	ASCTyDisplay		-> display (0);
	dynamicLabel		-> setText ("");
	nameofLanguage	        -> setText ("");
	programType	        -> setText ("");

	TunedFrequency		= 0;
	if (dabBand == BAND_III)
	   finger = bandIII_frequencies;
	else
	   finger = Lband_frequencies;

	for (i = 0; finger [i]. key != NULL; i ++) {
	   if (finger [i]. key == s) {
	      TunedFrequency	= KHz (finger [i]. fKHz);
	      setTuner (TunedFrequency);
	      break;
	   }
	}

	if (TunedFrequency == 0)
	   return;

	setTuner (TunedFrequency);
	if (localRunning) {
	   our_audioSink -> restart ();
	   myRig	 -> restartReader ();
	   the_ofdmProcessor	-> reset ();
	   running	 = true;
	}
	
}
//
//	If the ofdm processor has waited for a period of N frames
//	to get a start of a synchronization,
//	it sends a signal to the GUI handler
//	If "scanning" is "on" we hop to the next frequency on
//	the list
void	RadioInterface::No_Signal_Found (void) {
	if (!scanning)
	   return;
//
//	we stop the thread from running,
//	Increment the frequency
//	and restart
	the_ofdmProcessor -> stop ();
	while (!the_ofdmProcessor -> isFinished ())
	   usleep (10000);
	Increment_Channel ();
	clearEnsemble ();
	the_ofdmProcessor -> start ();
	the_ofdmProcessor	-> coarseCorrectorOn ();
	the_ofdmProcessor	-> reset ();
}
//
//	In case the scanning button was pressed, we
//	set it off as soon as we have a signal found
void	RadioInterface::Yes_Signal_Found (void) {
	if (!scanning)
	   return;
	fprintf (stderr, "entering yes signal with %d\n", scanning);
	set_Scanning ();
}

void	RadioInterface::set_Scanning	(void) {
	if (!running)
	   return;
	scanning	= !scanning;
	if (the_ofdmProcessor != NULL)
	   the_ofdmProcessor -> set_scanMode (scanning);
	if (scanning) {
	   scanButton -> setText ("scanning");
	   Increment_Channel ();
	}
	else
	   scanButton -> setText ("scan");
}
//
//	Increment channel is called during scanning.
//	The approach taken here is to increment the current index
//	in the combobox and select the new frequency.
//	Tp avoid disturbance, we disconnect the combobox
//	temporarily
void	RadioInterface::Increment_Channel (void) {
int16_t	i;
struct dabFrequencies *finger;
int	cc	= channelSelector -> currentIndex ();

	cc	+= 1;
	if (cc >= channelSelector -> count ())
	   cc = 0;
	disconnect (channelSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (set_channelSelect (const QString &)));
	channelSelector -> setCurrentIndex (cc);
//
//	Now setting the frequency
	TunedFrequency		= 0;
	if (dabBand == BAND_III)
	   finger = bandIII_frequencies;
	else
	   finger = Lband_frequencies;

	for (i = 0; finger [i]. key != NULL; i ++) {
	   if (finger [i]. key == channelSelector -> currentText ()) {
	      TunedFrequency	= KHz (finger [i]. fKHz);
	      setTuner (TunedFrequency);
	      break;
	   }
	}

	if (TunedFrequency == 0)
	   return;

	setTuner (TunedFrequency);
	connect    (channelSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (set_channelSelect (const QString &)));
}

//	on changing settings, we clear all things in the gui
//	related to the ensemble
void	RadioInterface::clearEnsemble	(void) {
//
//	first the real stuff
	Services = QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay		-> setModel (&ensemble);
	my_ficHandler		-> clearEnsemble ();
	if (autoCorrector)
	   the_ofdmProcessor	-> coarseCorrectorOn ();
	the_ofdmProcessor	-> reset ();

	ensembleLabel		= QString ();
	ensembleName		-> setText (ensembleLabel);
	
//	Then the various displayed items
	programName		-> setText ("   ");
	ensembleName		-> setText ("   ");
	errorDisplay		-> display (0);
	ficRatioDisplay		-> display (0);
	snrDisplay		-> display (0);
	uepFlagDisplay		-> display (0);
	startAddrDisplay	-> display (0);
	LengthDisplay		-> display (0);
	protLevelDisplay	-> display (0);
	bitRateDisplay		-> display (0);
	ASCTyDisplay		-> display (0);
	dynamicLabel		-> setText ("");
	nameofLanguage	        -> setText (" ");
	programType	        -> setText (" ");
}

void	RadioInterface::addtoEnsemble (const QString &s) {
	Services << s;
	Services. removeDuplicates ();
	ensemble. setStringList (Services);
	ensembleDisplay	-> setModel (&ensemble);
	the_ofdmProcessor	-> coarseCorrectorOff ();
}
//
//	a slot, called by the fib processor
void	RadioInterface::addEnsembleChar (char v, int i) {
	ensembleLabel. insert(i, QChar (v));
}

void	RadioInterface::nameofEnsemble (int id, const QString &v) {
QString s;

	(void)v;
	ensembleId		-> display (id);
	ensembleName		-> setText (v);
	the_ofdmProcessor	-> coarseCorrectorOff ();
	Yes_Signal_Found	();
}

static 
const char *table12 [] = {
"NONE",
"NEWS",
"CURRENT AFFAIRS",
"INFORMATION",
"SPORT",
"EDUCATION",
"DRAMA",
"ARTS",
"SCIENCE",
"TALK",
"POP MUSIC",
"ROCK MUSIC",
"EASY LISTENING",
"LIGHT CLASSICAL",
"CLASSICAL MUSIC",
"OTHER MUSIC",
"WHEATHER",
"FINANCE",
"CHILDREN\'S",
"FACTUAL",
"RELIGION",
"PHONE IN",
"TRAVEL",
"LEUSURE",
"JAZZ AND BLUES",
"COUNTRY MUSIC",
"NATIONAL MUSIC",
"OLDIES MUSIC",
"FOLK MUSIC",
"entry 29 not used",
"entry 30 not used",
"entry 31 not used"
};
static
const char *get_programm_type_string (uint8_t type) {
	if (type > 0x40) {
	   fprintf (stderr, "GUI: programmtype wrong (%d)\n", type);
	   return (table12 [0]);
	}

	return table12 [type];
}

static
const char *table9 [] = {
"unknown",
"Albanian",
"Breton",
"Catalan",
"Croatian",
"Welsh",
"Czech",
"Danish",
"German",
"English",
"Spanish",
"Esperanto",
"Estonian",
"Basque",
"Faroese",
"French",
"Frisian",
"Irish",
"Gaelic",
"Galician",
"Icelandic",
"Italian",
"Lappish",
"Latin",
"Latvian",
"Luxembourgian",
"Lithuanian",
"Hungarian",
"Maltese",
"Dutch",
"Norwegian",
"Occitan",
"Polish",
"Postuguese",
"Romanian",
"Romansh",
"Serbian",
"Slovak",
"Slovene",
"Finnish",
"Swedish",
"Tuskish",
"Flemish",
"Walloon"
};

static
const char *get_programm_language_string (uint8_t language) {
	if (language > 43) {
	   fprintf (stderr, "GUI: wrong language (%d)\n", language);
	   return table9 [0];
	}
	return table9 [language];
}

void	RadioInterface::selectService (QModelIndex s) {
int16_t	type, language;

QString a = ensemble. data (s, Qt::DisplayRole). toString ();
	switch (my_ficHandler -> kindofService (a)) {
	   case AUDIO_SERVICE:
	      { audiodata d;
	        my_ficHandler	-> dataforAudioService (a, &d);
	        my_mscHandler	-> set_audioChannel (&d);
	        showLabel (QString (" "));
	        language	= my_mscHandler	-> getLanguage ();
	        type		= my_mscHandler	-> getType ();
	        programName	-> setText (a);
	        nameofLanguage	-> setText (get_programm_language_string (language));
	        programType	-> setText (get_programm_type_string (type));
		uepFlagDisplay		-> display (d. uepFlag);
		startAddrDisplay	-> display (d. startAddr);
		LengthDisplay		-> display (d. length);
		protLevelDisplay	-> display (d. protLevel);
		bitRateDisplay		-> display (d. bitRate);
		ASCTyDisplay		-> display (d. ASCTy);
	        break;
	      }

	   case PACKET_SERVICE:
	      {  packetdata d;
	          my_ficHandler	-> dataforDataService (a, &d);
	         if ((d.  DSCTy == 0) || (d. bitRate == 0))
	            return;
	         my_mscHandler	-> set_dataChannel (&d);
	         programName	-> setText (" ");
	         nameofLanguage	-> setText (" ");
	         programType		-> setText (" ");
	         switch (d. DSCTy) {
	            default:
	               showLabel (QString ("unimplemented Data"));
	               break;
	            case 5:
	               showLabel (QString ("Transp. Channel not implemented"));
	               break;
	            case 60:
	               showLabel (QString ("MOT partially implemented"));
	               break;
	            case 59:
	               showLabel (QString ("Embedded IP: UDP data to 8888"));
	               break;
	            case 44:
	               showLabel (QString ("Journaline"));
	               break;
	         }
		 uepFlagDisplay		-> display (d. uepFlag);
		 startAddrDisplay	-> display (d. startAddr);
		 LengthDisplay		-> display (d. length);
		 protLevelDisplay	-> display (d. protLevel);
		 bitRateDisplay		-> display (d. bitRate);
		 ASCTyDisplay		-> display (d. DSCTy);
	         break;
	      }
	   default:
	      break;
	}
}

void	RadioInterface::set_dumping (void) {
SF_INFO *sf_info	= (SF_INFO *)alloca (sizeof (SF_INFO));

//	if (!someStick (myRig -> myIdentity ()))
//	   return;

	if (sourceDumping) {
	   the_ofdmProcessor	-> stopDumping ();
	   sf_close (dumpfilePointer);
	   sourceDumping = false;
	   dumpButton	-> setText ("dump");
	   return;
	}

	QString file = QFileDialog::getSaveFileName (this,
	                                     tr ("open file ..."),
	                                     QDir::homePath (),
	                                     tr ("raw data (*.sdr"));
	file	= QDir::toNativeSeparators (file);
	sf_info	-> samplerate	= INPUT_RATE;
	sf_info	-> channels	= 2;
	sf_info	-> format	= SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	dumpfilePointer	= sf_open (file. toLatin1 (). data (),
	                                   SFM_WRITE, sf_info);
	if (dumpfilePointer == NULL) {
	   qDebug () << "cannot open " << file. toLatin1 (). data ();
	   return;
	}
	dumpButton	-> setText ("writing");
	sourceDumping		= true;
	the_ofdmProcessor	-> startDumping (dumpfilePointer);
}

void	RadioInterface::set_mp2File (void) {

	if (mp2File != NULL) {
	   my_mscHandler	-> setFiles (NULL, mp4File);
	   fclose (mp2File);
	   mp2File	= NULL;
	   mp2fileButton	-> setText ("MP2");
	   return;
	}

	if (audioDumping) {
	   our_audioSink	-> stopDumping ();
	   sf_close (audiofilePointer);
	   audioDumping = false;
	   audioDump	-> setText ("audioDump");
	   return;
	}

	QString file = QFileDialog::getSaveFileName (this,
	                                     tr ("open file ..."),
	                                     QDir::homePath (),
	                                     tr ("mp2 data (*.mp2)"));
	file	= QDir::toNativeSeparators (file);
	mp2File	= fopen (file. toLatin1 (). data (), "w");
	if (mp2File == NULL) {
	   qDebug () << "cannot open " << file. toLatin1 (). data ();
	   return;
	}
	my_mscHandler	-> setFiles (mp2File, mp4File);
	mp2fileButton	-> setText ("writing");
}

void	RadioInterface::set_mp4File (void) {

	cout << "this button does not have effect\n";
	return;
	if (mp4File != NULL) {
	   my_mscHandler	-> setFiles (mp2File, NULL);
	   fclose (mp4File);
	   mp4File	= NULL;
	   aacfileButton	-> setText ("MP4");
	   return;
	}

	if (audioDumping) {
	   our_audioSink	-> stopDumping ();
	   sf_close (audiofilePointer);
	   audioDumping = false;
	   audioDump	-> setText ("audioDump");
	   return;
	}
	QString file = QFileDialog::getSaveFileName (this,
	                                     tr ("open file ..."),
	                                     QDir::homePath (),
	                                     tr ("aac data (*.aac)"));
	file	= QDir::toNativeSeparators (file);
	mp4File	= fopen (file. toLatin1 (). data (), "w");
	if (mp4File == NULL) {
	   qDebug () << "cannot open " << file. toLatin1 (). data ();
	   return;
	}

	my_mscHandler	-> setFiles (mp2File, mp4File);
	aacfileButton	-> setText ("writing");
}

void	RadioInterface::set_audioDump (void) {
SF_INFO	*sf_info	= (SF_INFO *)alloca (sizeof (SF_INFO));
	if (!someStick (myRig -> myIdentity ()))
	   return;

	if (!audioDumping && (mp2File != NULL || mp4File != NULL))
	   return;
	
	if (audioDumping) {
	   our_audioSink	-> stopDumping ();
	   sf_close (audiofilePointer);
	   audioDumping = false;
	   audioDump	-> setText ("audioDump");
	   return;
	}

	QString file = QFileDialog::getSaveFileName (this,
	                                        tr ("open file .."),
	                                        QDir::homePath (),
	                                        tr ("Sound (*.sdr)"));
	file		= QDir::toNativeSeparators (file);
	sf_info		-> samplerate	= 48000;
	sf_info		-> channels	= 2;
	sf_info		-> format	= SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	audiofilePointer	= sf_open (file. toLatin1 (). data (),
	                                   SFM_WRITE, sf_info);
	if (audiofilePointer == NULL) {
	   qDebug () << "Cannot open " << file. toLatin1 (). data ();
	   return;
	}

	audioDump		-> setText ("WRITING");
	audioDumping		= true;
	our_audioSink		-> startDumping (audiofilePointer);
}

void	RadioInterface::show_successRate (int s) {
	errorDisplay	-> display (s);
}

void	RadioInterface::show_ficCRC (bool b) {
        if (b)
           ficSuccess ++;
        if (++ficBlocks >= 100) {
           ficRatioDisplay      -> display (ficSuccess);
           ficSuccess   = 0;
           ficBlocks    = 0;
        }
}

void	RadioInterface::show_snr (int s) {
	snrDisplay	-> display (s);
}
//      if so configured, the function might be triggered
//      from the message decoding software. The GUI
//      might decide to ignore the data sent
void    RadioInterface::show_mscErrors  (int er) {
        dataErrors     -> display (er);
}
//
//      a slot, called by the iphandler
void    RadioInterface::show_ipErrors   (int er) {
        ipErrors     -> display (er);
}

//
//	setDevice is called trough a signal from the gui
//	Operation is in three steps: 
//	- first dumping of any kind is stopped
//	- second the previously loaded device is stopped
//	- third, the new device is initiated
void	RadioInterface::setDevice (QString s) {
bool	success;
QString	file;
//
//	first stop dumping
	if (sourceDumping) {
	   the_ofdmProcessor -> stopDumping ();
	   sf_close (dumpfilePointer);
	   sourceDumping = false;
	   dumpButton	-> setText ("dump");
	}

	if (audioDumping) {
	   our_audioSink	-> stopDumping ();
	   sf_close (audiofilePointer);
	   audioDumping	= false;
	   audioDump -> setText ("audioDump");
	}

	running	= false;
	our_audioSink	-> stop ();
//
//
//	select. For all it holds that:
	myRig	-> stopReader ();
	delete	the_ofdmProcessor;
	delete	myRig;
//
#ifdef	HAVE_AIRSPY
	if (s == "airspy") {
	   myRig	= new airspyHandler (dabSettings, &success);
	   if (!success) {
	      delete myRig;
	      QMessageBox::warning (this, tr ("sdr"),
	                               tr ("airspy: no luck\n"));
	      myRig = new virtualInput ();
	      resetSelector ();
	   }
	   else 
	      set_channelSelect	(channelSelector -> currentText ());
	}
	else
#endif
#ifdef HAVE_UHD
//
//	UHD is - at least in its current setting - for Linux
	if (s == "UHD") {
	   myRig = new uhdInput (dabSettings, &success );
	   if (!success) {
	      delete myRig;
	      QMessageBox::warning( this, tr ("sdr"), tr ("UHD: no luck\n") );
	      myRig = new virtualInput();
	      resetSelector ();
	   }
	   else 
	      set_channelSelect (channelSelector->currentText() );
	}
	else
#endif
//	extio is - in its current settings - for Windows, it is a
//	wrap around the dll
#ifdef HAVE_EXTIO
	if (s == "extio") {
	   myRig = new extioHandler (dabSettings, &success );
	   if (!success) {
	      delete myRig;
	      QMessageBox::warning( this, tr ("sdr"), tr ("extio: no luck\n") );
	      myRig = new virtualInput();
	      resetSelector ();
	   }
	   else 
	      set_channelSelect (channelSelector -> currentText() );
	}
	else
#endif
#ifdef HAVE_RTL_TCP
//	RTL_TCP might be working
	if (s == "rtl_tcp") {
	   myRig = new rtl_tcp_client (dabSettings, &success );
	   if (!success) {
	      delete myRig;
	      QMessageBox::warning( this, tr ("sdr"), tr ("UHD: no luck\n") );
	      myRig = new virtualInput();
	      resetSelector ();
	   }
	   else 
	      set_channelSelect (channelSelector->currentText() );
	}
	else
#endif
#ifdef	HAVE_SDRPLAY
	if (s == "sdrplay") {
	   myRig	= new sdrplay (dabSettings, &success);
	   if (!success) {
	      delete myRig;
	      QMessageBox::warning (this, tr ("sdr"),
	                               tr ("Mirics device: no library\n"));
	      myRig = new virtualInput ();
	      resetSelector ();
	   }
	   else 
	      set_channelSelect	(channelSelector -> currentText ());
	}
	else
#endif
#ifdef	HAVE_DABSTICK
	if (s == "dabstick") {
	   myRig	= new dabStick (dabSettings, &success);
	   if (!success) {
	      delete myRig;
	      QMessageBox::warning (this, tr ("sdr"),
	                               tr ("Dabstick: no luck\n"));
	      myRig = new virtualInput ();
	      resetSelector ();
	   }
	   else 
	      set_channelSelect	(channelSelector -> currentText ());
	}
	else
#endif
	if (s == "file input (.raw)") {
	   file		= QFileDialog::getOpenFileName (this,
	                                                tr ("open file ..."),
	                                                QDir::homePath (),
	                                                tr ("raw data (*.raw)"));
	   file		= QDir::toNativeSeparators (file);
	   myRig	= new rawFiles (file, &success);
	   if (!success) {
	      delete myRig;
	      myRig = new virtualInput ();
	      resetSelector ();
	   }
	}
	else
	if (s == "file input (.sdr)") {
	   file		= QFileDialog::getOpenFileName (this,
	                                                tr ("open file ..."),
	                                                QDir::homePath (),
	                                                tr ("raw data (*.sdr)"));
	   file		= QDir::toNativeSeparators (file);
	   myRig	= new wavFiles (file, &success);
	   if (!success) {
	      delete myRig;
	      myRig = new virtualInput ();
	      resetSelector ();
	   }
	}
	else {	// s == "no device" 
	   myRig	= new virtualInput ();
	}

#ifdef	HAVE_SPECTRUM
	spectrumHandler	-> setBitDepth (myRig -> bitDepth ());
#endif
	the_ofdmProcessor	= new ofdmProcessor (myRig,
	                                             &dabModeParameters,
	                                             this,
	                                             my_mscHandler,
	                                             my_ficHandler,
	                                             threshold,
#ifdef	HAVE_SPECTRUM
	                                             spectrumBuffer,
#endif
	                                             iqBuffer,
	                                             freqSyncMethod);
}
//
//
//	In case selection of a device did not work out for whatever
//	reason, the device selector is reset to "no device"
//	Qt will trigger on the chgange of value in the deviceSelector
//	which will cause selectdevice to be called again (while we
//	are in the middle, so we first disconnect the selector
//	from the slot. Obviously, after setting the index of
//	the device selector, we connect again

void	RadioInterface::resetSelector (void) {
	disconnect (deviceSelector, SIGNAL (activated (const QString &)),
	            this, SLOT (setDevice (const QString &)));
int	k	= deviceSelector -> findText (QString ("no device"));
	if (k != -1) { 		// should not happen
	   deviceSelector -> setCurrentIndex (k);
	}
	connect (deviceSelector, SIGNAL (activated (const QString &)),
	         this, SLOT (setDevice (const QString &)));
}

//
//	signal, received from ofdm_decoder that a buffer is filled
//	with amount values ready for display
void	RadioInterface::showIQ	(int amount) {
DSPCOMPLEX *Values	= (DSPCOMPLEX *)alloca (amount * sizeof (DSPCOMPLEX));
int16_t	i;
int16_t	t;
double	avg	= 0;

	t = iqBuffer -> getDataFromBuffer (Values, amount);
	for (i = 0; i < t; i ++) {
	   float x = abs (Values [i]);
	   if (!isnan (x) && !isinf (x))
	      avg += abs (Values [i]);
	}

	avg	/= t;
	myIQDisplay -> DisplayIQ (Values, scopeWidth / avg);
}

#ifdef	HAVE_SPECTRUM
void	RadioInterface::showSpectrum	(int32_t amount) {
	if (spectrumHandler != NULL)
	   spectrumHandler	-> showSpectrum (amount, vfoFrequency);
}
#endif

//
//	This is a copy of the clearEnsemble, with as difference
//	that the autoCorrector is ON. We then need clean settings
void	RadioInterface::hard_Reset (void) {
//	first the real stuff
	Services		= QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay		-> setModel (&ensemble);
	my_ficHandler		-> clearEnsemble ();
	the_ofdmProcessor	-> coarseCorrectorOn ();
	the_ofdmProcessor	-> reset ();
//
//	Then the various displayed items
	programName		-> setText ("   ");
	ensembleName		-> setText ("   ");
	errorDisplay		-> display (0);
	ficRatioDisplay		-> display (0);
	snrDisplay		-> display (0);
	uepFlagDisplay		-> display (0);
	startAddrDisplay	-> display (0);
	LengthDisplay		-> display (0);
	protLevelDisplay	-> display (0);
	bitRateDisplay		-> display (0);
	ASCTyDisplay		-> display (0);
	dynamicLabel		-> setText ("");
	nameofLanguage	        -> setText ("");
	programType	        -> setText ("");
}
//
//	When selecting another mode, first ensure that all kinds
//	of dumping are stopped, or just stop them
void	RadioInterface::selectMode (const QString &s) {
uint8_t	Mode	= s. toInt ();

	if (sourceDumping) {
	   the_ofdmProcessor -> stopDumping ();
	   sf_close (dumpfilePointer);
	   sourceDumping = false;
	   dumpButton	-> setText ("dump");
	}

	if (audioDumping) {
	   our_audioSink	-> stopDumping ();
	   sf_close (audiofilePointer);
	   audioDumping	= false;
	   audioDump -> setText ("audioDump");
	}

	running	= false;
	our_audioSink		-> stop ();
	myRig			-> stopReader ();
	the_ofdmProcessor	-> stop ();
//
//	we have to create a new ofdmprocessor with the correct
//	settings of the parameters.
	delete 	the_ofdmProcessor;
	setModeParameters (Mode);
	delete my_ficHandler;
	my_ficHandler		= new ficHandler	(this,
	                                                 2 * dabModeParameters. K);
	delete	my_mscHandler;
	my_mscHandler		= new mscHandler	(this,
	                                                 &dabModeParameters,
	                                                 our_audioSink,
	                                                 true);
	delete the_ofdmProcessor;
	the_ofdmProcessor	= new ofdmProcessor (myRig,
	                                             &dabModeParameters,
	                                             this,
	                                             my_mscHandler,
	                                             my_ficHandler,
	                                             threshold,
#ifdef	HAVE_SPECTRUM
	                                             spectrumBuffer,
#endif
	                                             iqBuffer,
	                                             freqSyncMethod);
//	and wait for setStart
}
//
//	the values for the different Modes:
void	RadioInterface::setModeParameters (int16_t Mode) {
	if (Mode == 2) {
	   dabModeParameters. dabMode	= 2;
	   dabModeParameters. L		= 76;		// blocks per frame
	   dabModeParameters. K		= 384;		// carriers
	   dabModeParameters. T_null	= 664;		// null length
	   dabModeParameters. T_F	= 49152;	// samples per frame
	   dabModeParameters. T_s	= 638;		// block length
	   dabModeParameters. T_u	= 512;		// useful part
	   dabModeParameters. guardLength	= 126;
	   dabModeParameters. carrierDiff	= 4000;
	} else
	if (Mode == 4) {
	   dabModeParameters. dabMode		= 4;
	   dabModeParameters. L			= 76;
	   dabModeParameters. K			= 768;
	   dabModeParameters. T_F		= 98304;
	   dabModeParameters. T_null		= 1328;
	   dabModeParameters. T_s		= 1276;
	   dabModeParameters. T_u		= 1024;
	   dabModeParameters. guardLength	= 252;
	   dabModeParameters. carrierDiff	= 2000;
	} else 
	if (Mode == 3) {
	   dabModeParameters. dabMode		= 3;
	   dabModeParameters. L			= 153;
	   dabModeParameters. K			= 192;
	   dabModeParameters. T_F		= 49152;
	   dabModeParameters. T_null		= 345;
	   dabModeParameters. T_s		= 319;
	   dabModeParameters. T_u		= 256;
	   dabModeParameters. guardLength	= 63;
	   dabModeParameters. carrierDiff	= 2000;
	} else {	// default = Mode I
	   dabModeParameters. dabMode		= 1;
	   dabModeParameters. L			= 76;
	   dabModeParameters. K			= 1536;
	   dabModeParameters. T_F		= 196608;
	   dabModeParameters. T_null		= 2656;
	   dabModeParameters. T_s		= 2552;
	   dabModeParameters. T_u		= 2048;
	   dabModeParameters. guardLength	= 504;
	   dabModeParameters. carrierDiff	= 1000;
	}
}

void	RadioInterface::setSynced	(char b) {
	if (isSynced == b)
	   return;

	isSynced = b;
	switch (isSynced) {
	   case SYNCED:
	      syncedLabel -> 
	               setStyleSheet ("QLabel {background-color : green}");
	      break;

	   default:
	      syncedLabel ->
	               setStyleSheet ("QLabel {background-color : red}");
	      break;
	}
}

void	RadioInterface::showLabel	(QString s) {
	dynamicLabel	-> setText (s);
}

void	RadioInterface::showMOT		(QByteArray data, int subtype) {
	if (pictureLabel == NULL)
	   pictureLabel	= new QLabel (NULL);

	QPixmap p;
	p. loadFromData (data, subtype == 0 ? "GIF" :
	                       subtype == 1 ? "JPG" :
	                       subtype == 2 ? "BMP" : "PNG");
	pictureLabel ->  setPixmap (p);
	pictureLabel ->  show ();
}

void	RadioInterface::send_datagram	(char *data, int length) {
	DSCTy_59_socket. writeDatagram (data, length,
	                                QHostAddress ("127.0.0.1"),
	                                8888);
}

#ifdef	HAVE_SPECTRUM
void	RadioInterface::set_spectrumHandler (void) {
	spectrumisShown = !spectrumisShown;
	if (spectrumisShown)
	   spectrumHandler -> show ();
	else
	   spectrumHandler -> hide ();
}
#endif
