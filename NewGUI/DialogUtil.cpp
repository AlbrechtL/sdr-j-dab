/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * British Broadcasting Corporation
 * Copyright (c) 2001-2001-2014
 *
 * Author(s):
 *  Volker Fischer, Julian Cable
 *
 * Description:
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include <QMenuBar>
#include <QLabel>
#include <QAction>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QWhatsThis>
#ifdef _WIN32
# include <winsock2.h>
#endif
#include "Util.h"
#include "DialogUtil.h"
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
#endif
#ifdef WITH_QCUSTOMPLOT
# include "qcplevelmeter.h"
# include "qcpsmeter.h"
#else
# include "qwtlevelmeter.h"
# include "qwtsmeter.h"
#endif

/* to extract the library version */
#ifdef USE_ALSA
# include <alsa/version.h>
#endif
#ifdef USE_OSS
# include <sys/soundcard.h>
#endif
#ifdef USE_PORTAUDIO
# include <portaudio.h>
#endif
#ifdef USE_PULSEAUDIO
# include <pulse/version.h>
#endif
#ifdef HAVE_LIBSNDFILE
# include <sndfile.h>
#endif
#ifdef HAVE_LIBPCAP
# include <pcap.h>
#endif
#ifdef HAVE_LIBFREEIMAGE
# include <FreeImage.h>
#endif
#ifdef WITH_QCUSTOMPLOT
# include <qcustomplot.h>
#else
# include <qwt_global.h>
# include <qwt_thermo.h>
#endif
#ifdef USE_OPUS_LIBRARY
# include "../sourcedecoders/opus_codec.h"
#endif
#ifdef USE_FAAD2_LIBRARY
# include <neaacdec.h>
#else

#endif
/* fftw 3.3.2 doesn't export the symbol fftw_version
 * for windows in libfftw3-3.def
 * You can add it regenerate the lib file and it's supposed to work,
 * but for now the version string is disabled for windows. */
#ifndef _WIN32
# include <fftw3.h>
#endif
//#include "ThemeCustomizer.h"





/* System Tray -------------------------------------------------------------- */

CSysTray::CSysTray(QWidget* parent, const char* callbackIcon, const char* callbackTimer, const char* icon)
    : parent(parent), pTimer(NULL), pContextMenu(NULL)
{
    pSystemTrayIcon = new QSystemTrayIcon(QIcon(icon), parent);
    if (callbackIcon != NULL)
        parent->connect(pSystemTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), parent, callbackIcon);
    if (callbackTimer != NULL)
    {
        pTimer = new QTimer(pSystemTrayIcon);
        parent->connect(pTimer, SIGNAL(timeout()), parent, callbackTimer);
    }
    pSystemTrayIcon->show();
}

CSysTray::~CSysTray()
{
    if (pSystemTrayIcon != NULL)
        delete pSystemTrayIcon;
}

void CSysTray::CreateContextMenu()
{
    if (pContextMenu == NULL)
    {
        pContextMenu = new QMenu(parent);
        pSystemTrayIcon->setContextMenu(pContextMenu);
    }
}

CSysTray* CSysTray::Create(QWidget* parent, const char* callbackIcon, const char* callbackTimer, const char* icon)
{
    CSysTray* pSysTray = NULL;
    if (QSystemTrayIcon::isSystemTrayAvailable())
        pSysTray = new CSysTray(parent, callbackIcon, callbackTimer, icon);
    return pSysTray;
}

void CSysTray::Destroy(CSysTray** pSysTray)
{
    if (*pSysTray != NULL)
    {
        delete *pSysTray;
        *pSysTray = NULL;
    }
}

void CSysTray::Start(CSysTray* pSysTray)
{
    /*if (pSysTray == NULL) return;
    if (pSysTray->pTimer != NULL)
        pSysTray->pTimer->start(GUI_CONTROL_UPDATE_TIME);*/
}

void CSysTray::Stop(CSysTray* pSysTray, const QString& Message)
{
    if (pSysTray == NULL) return;
    if (pSysTray->pTimer != NULL)
        pSysTray->pTimer->stop();
    SetToolTip(pSysTray, QString(), Message);
}

QAction* CSysTray::AddAction(CSysTray* pSysTray, const QString& text, const QObject* receiver, const char* member)
{
    if (pSysTray == NULL) return NULL;
    pSysTray->CreateContextMenu();
    return pSysTray->pContextMenu->addAction(text, receiver, member);
}

QAction* CSysTray::AddSeparator(CSysTray* pSysTray)
{
    if (pSysTray == NULL) return NULL;
    pSysTray->CreateContextMenu();
    return pSysTray->pContextMenu->addSeparator();
}

void CSysTray::SetToolTip(CSysTray* pSysTray, const QString& Title, const QString& Message)
{
    if (pSysTray != NULL &&
            (pSysTray->Title != Title || pSysTray->Message != Message))
    {
        pSysTray->Title = Title;
        pSysTray->Message = Message;
        QString ToolTip;
#ifdef _WIN32
        ToolTip = Title;
        if (!Message.isEmpty())
        {
            if (!Title.isEmpty())
                ToolTip += " |  ";
            ToolTip += Message;
        }
        ToolTip.replace(QRegExp("(\r|\n|\v|\t|\b)"), " ");
#else
        if (!Title.isEmpty())
        {
            QString NewTitle(Title);
            NewTitle.replace('&', "&amp;");
            NewTitle.replace(' ', "&nbsp;");
            NewTitle.replace('<', "&lt;");
            NewTitle.replace('>', "&gt;");
            ToolTip = "<b>" + NewTitle + "</b>";
        }
        if (!Message.isEmpty())
        {
            QString NewMessage(Message);
            if (!Title.isEmpty())
                ToolTip += "<br>";
            NewMessage.replace('&', "&amp;");
            NewMessage.replace('<', "&lt;");
            NewMessage.replace('>', "&gt;");
            ToolTip += NewMessage;
        }
        ToolTip.replace(QRegExp("(\r|\n|\v)"), "<br>");
#endif
        pSysTray->pSystemTrayIcon->setToolTip(ToolTip);
    }
}

LevelMeter* LevelMeter::createLevelMeter(QWidget* parent)
{
#ifdef QWT_VERSION
    return new QwtLevelMeter(parent);
#else
    return new QCPLevelMeter(parent);
#endif
}

SMeter* SMeter::createSMeter(QWidget* parent)
{
#ifdef QWT_VERSION
    return new QwtSMeter(parent);
#else
    return new QCPSMeter(parent);
#endif
}
