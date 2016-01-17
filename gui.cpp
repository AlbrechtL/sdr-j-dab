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
/*
 *	We use the creation function merely to set up the
 *	user interface and make the connections between the
 *	gui elements and the handling agents. All real action
 *	is embedded in actions, initiated by gui buttons
 */
	RadioInterface::RadioInterface (QSettings	*Si,
	                                QWidget		*parent): QDialog (parent) {
int16_t	i, k;

// 	the setup for the generated part of the ui
	setupUi (this);
	dabSettings		= Si;
//
//	Before printing anything, we set
	setlocale (LC_ALL, "");
//	The default:
	myRig			= new virtualInput ();
	running			= false;
	
	autoCorrector =
	           dabSettings -> value ("autoCorrector", 1). toInt () == 1;

	threshold	=
	           dabSettings -> value ("threshold", 3). toInt ();
//	Note that the generation of values to be displayed
//	is in a separate thread, we need a buffer for communication
//	and a signal/slot combination for triggering

	isSynced		= UNSYNCED;
	syncedLabel		->
	               setStyleSheet ("QLabel {background-color : red}");

	iqBuffer		= new RingBuffer<DSPCOMPLEX> (2 * 1536);
	iqDisplaysize	=
	               dabSettings -> value ("iqDisplaysize", 256). toInt ();
	myIQDisplay		= new IQDisplay (iqDisplay, iqDisplaysize);
//
//	'Concurrent' indicates whether the mp4/mp2 processing part
//	will be in a separate thread or not
	Concurrent		= true;
	TunedFrequency		= MHz (200);	// any value will do
	outRate			= 48000;
	outBuffer		=
	                    dabSettings -> value ("outBuffer", 32768). toInt ();
//
//	Maybe we should move the outTable to the audioSink?
	our_audioSink		= new audioSink		(outRate, outBuffer);
	outTable		= new int16_t
	                             [our_audioSink -> numberofDevices ()];
	for (i = 0; i < our_audioSink -> numberofDevices (); i ++)
	   outTable [i] = -1;

	if (!setupSoundOut (streamOutSelector,
	                    our_audioSink, outRate, outTable)) {
	   fprintf (stderr, "Cannot open any output device\n");
	   exit (22);
	}
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
#ifdef	HAVE_DONGLE
	deviceSelector	-> addItem ("dongle");
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

	setModeParameters (1);
	the_mscHandler		= new mscHandler	(this,
	                                                 our_audioSink,
	                                                 Concurrent);
	the_ficHandler		= new ficHandler	(this,
	                                                 the_mscHandler);
//
//	the default is:
	the_ofdmProcessor = new ofdmProcessor (myRig,
	                                       &dabModeParameters,
	                                       this,
	                                       the_mscHandler,
	                                       the_ficHandler,
	                                       threshold,
#ifdef	HAVE_SPECTRUM
	                                       spectrumBuffer,
#endif
	                                       iqBuffer);
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
	connect	(modeSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (selectMode (const QString &)));
	connect (startButton, SIGNAL (clicked (void)),
	              this, SLOT (setStart (void)));
	connect (QuitButton, SIGNAL (clicked ()),
	              this, SLOT (TerminateProcess (void)));
	connect (deviceSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (setDevice (const QString &)));
	connect (streamOutSelector, SIGNAL (activated (int)),
	              this, SLOT (setStreamOutSelector (int)));
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
	connect (correctorReset, SIGNAL (clicked (void)),
	              this, SLOT (autoCorrector_on (void)));
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
	FreqIncrement		= 0;
	sourceDumping		= false;
	dumpfilePointer		= NULL;
	audioDumping		= false;
	audiofilePointer	= NULL;
	sampleRateDisplay 	-> display (INPUT_RATE);
//
//	some settings may be influenced by the ini file
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
	fprintf (stderr, "we gaan nu echt deleten\n");
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
	s	-> setValue ("Concurrent", Concurrent);
	s	-> setValue ("vfoOffset", vfoOffset);
	s	-> setValue ("iqDisplaysize", iqDisplaysize);
	s	-> setValue ("autoCorrector", autoCorrector ? 1 : 0);
}
//
//	On start, we ensure that the streams are stopped so
//	that they can be restarted again.
void	RadioInterface::setStart	(void) {
bool	r = 0;
	if (running)		// only listen when not running yet
	   return;
//
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
	   the_mscHandler	-> setFiles (NULL, NULL);
	   fclose (mp2File);
	}

	if (mp4File != NULL) {
	   the_mscHandler	-> setFiles (NULL, NULL);
	   fclose (mp4File);
	}

	the_mscHandler	-> stop	();		// might be concurrent
	the_ofdmProcessor -> stop ();		// definitely concurrent
	myRig		-> stopReader ();
	our_audioSink	-> stop ();
	dumpControlState (dabSettings);
	delete		the_ofdmProcessor;
	delete		the_ficHandler;
	delete		the_mscHandler;
	delete		myRig;
	delete		our_audioSink;
#ifdef	HAVE_SPECTRUM
	delete spectrumHandler;
	fprintf (stderr, "the spectrumHandler is gone\n");
#endif
	if (pictureLabel != NULL)
	   delete pictureLabel;
	pictureLabel	= NULL;
	accept ();
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

	if (localRunning) {
	   our_audioSink	-> stop ();
	   myRig		-> stopReader ();
	   myRig		-> resetBuffer ();
	}
	Services = QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay		-> setModel (&ensemble);
	the_ficHandler		-> clearEnsemble ();

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
	dynamicLabel		-> setText	("");

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
//	on changing settings, we clear all things in the gui
//	related to the ensemble
void	RadioInterface::clearEnsemble	(void) {
//
//	first the real stuff
	Services = QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay		-> setModel (&ensemble);
	the_ficHandler		-> clearEnsemble ();
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
}

static 
const char *table12 [] = {
"none",
"news",
"current affairs",
"information",
"sport",
"education",
"drama",
"arts",
"science",
"talk",
"pop music",
"rock music",
"easy listening",
"light classical",
"classical music",
"other music",
"wheather",
"finance",
"children\'s",
"factual",
"religion",
"phone in",
"travel",
"leisure",
"jazz and blues",
"country music",
"national music",
"oldies music",
"folk music",
"entry 29 not used",
"entry 30 not used",
"entry 31 not used"
};

const char *RadioInterface::get_programm_type_string (uint8_t type) {
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

const char *RadioInterface::get_programm_language_string (uint8_t language) {
	if (language > 43) {
	   fprintf (stderr, "GUI: wrong language (%d)\n", language);
	   return table9 [0];
	}
	return table9 [language];
}

void	RadioInterface::selectService (QModelIndex s) {
uint8_t	coding;
int16_t	type, language;
bool	is_audio;

QString a = ensemble. data (s, Qt::DisplayRole). toString ();
	the_ficHandler -> setSelectedService (a);

	the_mscHandler	-> getMode (&is_audio, &coding);
	if (is_audio) {
	   if (coding == 077) 
	      dabMode -> setText ("DAB +");
	   else
	      dabMode -> setText ("DAB");
	   language	= the_mscHandler	-> getLanguage ();
	   type		= the_mscHandler	-> getType ();
	   programName		-> setText (a);
	   nameofLanguage	-> setText (get_programm_language_string (language));
	   programType	-> setText (get_programm_type_string (type));
	   showLabel (" ");
	}
	else {
	   programName		-> setText (" ");
	   nameofLanguage	-> setText (" ");
	   programType		-> setText (" ");
	   switch (coding) {
	      default:
	         showLabel (QString ("unimplemented Data"));
	         break;
	      case 5:
	         showLabel (QString ("Transparent Channel not implemented"));
	         break;
	      case 60:
	         showLabel (QString ("MOT partially implemented"));
	         break;
	      case 59:
	         showLabel (QString ("Embedded IP: UDP data sent to 8888"));
	         break;
	   }
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
	                                     tr ("raw data (*.sdr)"));
	file	= QDir::toNativeSeparators (file);
	sf_info	-> samplerate	= INPUT_RATE;
	sf_info	-> channels	= 2;
	sf_info	-> format	= SF_FORMAT_WAV | SF_FORMAT_PCM_24;

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
	   the_mscHandler	-> setFiles (NULL, mp4File);
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
	the_mscHandler	-> setFiles (mp2File, mp4File);
	mp2fileButton	-> setText ("writing");
}

void	RadioInterface::set_mp4File (void) {

	if (mp4File != NULL) {
	   the_mscHandler	-> setFiles (mp2File, NULL);
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

	the_mscHandler	-> setFiles (mp2File, mp4File);
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
	                                        tr ("Sound (*.wav)"));
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


void	RadioInterface::channelData (int subchId,
	                             int uepFlag,
	                             int startAddr,
	                             int Length,
	                             int protLevel,
	                             int bitRate,
	                             int ASCTy) {
	(void)subchId;
	uepFlagDisplay		-> display (uepFlag);
	startAddrDisplay	-> display (startAddr);
	LengthDisplay		-> display (Length);
	protLevelDisplay	-> display (protLevel);
	bitRateDisplay		-> display (bitRate);
	ASCTyDisplay		-> display (ASCTy);
}

void	RadioInterface::show_successRate (int s) {
	errorDisplay	-> display (s);
}

void	RadioInterface::show_ficRatio (int s) {
	ficRatioDisplay	-> display (s);
}

void	RadioInterface::show_snr (int s) {
	snrDisplay	-> display (s);
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
#ifdef	HAVE_DONGLE
	if (s == "dongle") {
	   myRig	= new miricsDongle (dabSettings, &success);
	   if (!success) {
	      delete myRig;
	      QMessageBox::warning (this, tr ("sdr"),
	                               tr ("miricsDongle: no luck\n"));
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
	                                             the_mscHandler,
	                                             the_ficHandler,
	                                             threshold,
#ifdef	HAVE_SPECTRUM
	                                             spectrumBuffer,
#endif
	                                             iqBuffer);
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
//	   myIQDisplay -> DisplayIQ (Values [i], scopeWidth * 0.0000005);
}

#ifdef	HAVE_SPECTRUM
void	RadioInterface::showSpectrum	(int32_t amount) {
	spectrumHandler	-> showSpectrum (amount, vfoFrequency);
}
#endif

//	do not forget that ocnt starts with 1, due
//	to Qt list conventions
bool	RadioInterface::setupSoundOut (QComboBox	*streamOutSelector,
	                               audioSink	*our_audioSink,
	                               int32_t		cardRate,
	                               int16_t		*table) {
uint16_t	ocnt	= 1;
uint16_t	i;

	for (i = 0; i < our_audioSink -> numberofDevices (); i ++) {
	   const QString so = 
	             our_audioSink -> outputChannelwithRate (i, cardRate);
	   qDebug ("Investigating Device %d\n", i);

	   if (so != QString ("")) {
	      streamOutSelector -> insertItem (ocnt, so, QVariant (i));
	      table [ocnt] = i;
	      qDebug (" (output):item %d wordt stream %d (%s)\n", ocnt , i,
	                      so. toLatin1 ().data ());
	      ocnt ++;
	   }
	}

	qDebug () << "added items to combobox";
	return ocnt > 1;
}

void	RadioInterface::setStreamOutSelector (int idx) {
	if (idx == 0)
	   return;

	outputDevice = outTable [idx];
	if (!our_audioSink -> isValidDevice (outputDevice)) 
	   return;

	our_audioSink	-> stop	();
	if (!our_audioSink -> selectDevice (outputDevice)) {
	   QMessageBox::warning (this, tr ("sdr"),
	                               tr ("Selecting  output stream failed\n"));
	   our_audioSink -> selectDefaultDevice ();
	   return;
	}

	qWarning () << "selected output device " << idx << outputDevice;
}
//
//	This is a copy of the clearEnsemble, with as difference
//	that the autoCorrector is ON. We then need clean settings
void	RadioInterface::autoCorrector_on (void) {
//	first the real stuff
	Services		= QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay		-> setModel (&ensemble);
	the_ficHandler		-> clearEnsemble ();
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
	the_ficHandler		-> setBitsperBlock	(2 * dabModeParameters. K);
	the_mscHandler		-> setMode		(&dabModeParameters);
	delete the_ofdmProcessor;
	the_ofdmProcessor	= new ofdmProcessor (myRig,
	                                             &dabModeParameters,
	                                             this,
	                                             the_mscHandler,
	                                             the_ficHandler,
	                                             threshold,
#ifdef	HAVE_SPECTRUM
	                                             spectrumBuffer,
#endif
	                                             iqBuffer);
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

