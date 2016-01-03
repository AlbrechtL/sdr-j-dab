/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *
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

#include "Util.h"
#include <QRegExp>
#include <QDate>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QStringList>
#ifdef QT_GUI_LIB
# include <QHeaderView>
#endif

/* Ensure that the given filename is secure */
QString VerifyFilename(QString filename)
{
    filename.replace(QRegExp("/"), "_"); /* replace unix-like path separator with '_' */
#ifdef _WIN32
    filename.replace(QRegExp("\\\\"), "_"); /* replace windows path separator with '_' */
    filename.replace(QRegExp(":"), "_"); /* replace ':' with '_' */
#endif
    return filename;
}

/* Ensure that the given html path is secure */
QString VerifyHtmlPath(QString path)
{
    if (path == "..")
        return "_";
#ifdef _WIN32
    path.replace(QRegExp("\\\\"), "_"); /* replace windows path separator with '_' */
    path.replace(QRegExp(":"), "_"); /* replace ':' with '_' */
#endif
    path.replace(QRegExp("^\\.\\./"), "_/"); /* replace '../' at the beginning with '_/' */
    path.replace(QRegExp("/\\.\\.$"), "/_"); /* replace '/..' at the end with '/_' */
    path.replace(QRegExp("/\\.\\./"), "/_/"); /* replace '/../' with '/_/' */
    return path;
}

/* Accept both absolute and relative url, but only return the path component.
   Invalid characters in path are percent-encoded (e.g. space = %20) */
QString UrlEncodePath(QString url)
{
    /* Get path component */
    QString path(QUrl(url, QUrl::TolerantMode).path());
    /* Prepend with '/' if none present */
    if (path.size() == 0 || path.at(0) != QChar('/'))
        path.insert(0, QChar('/'));
    /* Replace multiple '/' by single '/' */
    path.replace(QRegExp("/{1,}"), "/");
    /* Replace all occurrence of '/./' with '/' */
    while (path.indexOf("/./") != -1)
        path.replace(QRegExp("/\\./"), "/");
    /* The Actual percent encoding */
    path = QString(QUrl(path, QUrl::TolerantMode).toEncoded(
                       QUrl::RemoveScheme | QUrl::RemoveAuthority |
                       QUrl::RemoveQuery | QUrl::RemoveFragment));
    return path;
}

/* Determine if the given url is a directory */
bool IsUrlDirectory(QString url)
{
    return url.endsWith(QChar('/'));
}

/* Convert all www. or http:// or email to real
   clickable link, for use with QLabel and such.
   Code by David Flamand */
QString& Linkify(QString& text, QString linkColor)
{
    int i, j, posWWW=-2, posHTTP=-2, posMAIL=-2, posBegin, posEnd, size;
    if (!linkColor.isEmpty())
        linkColor = " style=\"color: " + linkColor + ";\"";
    size = text.size();
    for (i = 0; i < size;)
    {
        if (posWWW != -1 && posWWW < i)
            posWWW  = text.indexOf("www.", i, Qt::CaseInsensitive);
        if (posHTTP != -1 && posHTTP < i)
            posHTTP = text.indexOf("http://", i, Qt::CaseInsensitive);
        if (posMAIL != -1 && posMAIL < i)
            posMAIL = text.indexOf(QRegExp("\\b[0-9a-z._-]+@[0-9a-z.-]+\\.[a-z]{2,4}\\b", Qt::CaseInsensitive), i);
        if (posMAIL>=0 && (posMAIL<=posWWW || posWWW<0) && (posMAIL<posHTTP || posHTTP<0))
            posBegin = posMAIL;
        else if (posWWW>=0 && (posWWW<posHTTP || posHTTP<0))
            posBegin = posWWW;
        else
            posBegin = posHTTP;
        if (posBegin >= 0)
        {
            posEnd = size;
            for (j = posBegin; j < size; j++)
            {
                int chr = text[j].unicode();
                if (!((chr=='@' && posBegin==posMAIL) ||
                        chr=='.' || chr=='/' ||
                        chr=='~' || chr=='-' ||
                        chr=='_' || chr==':' ||
                        (chr>='a' && chr<='z') ||
                        (chr>='A' && chr<='Z') ||
                        (chr>='0' && chr<='9')))
                {
                    posEnd = j;
                    break;
                }
            }
            const int rawLinkSize = posEnd-posBegin;
            QStringRef rawLink(&text, posBegin, rawLinkSize);
            QString newLink;
            if (posBegin == posMAIL)
                newLink = "<a href=\"mailto:%1\"" + linkColor + ">%1</a>";
            else if (posBegin == posWWW)
                newLink = "<a href=\"http://%1\"" + linkColor + ">%1</a>";
            else /* posBegin == posHTTP */
                newLink = "<a href=\"%1\"" + linkColor + ">%1</a>";
            newLink = newLink.arg(rawLink.toString());
            const int newLinkSize = newLink.size();
            text.replace(posBegin, rawLinkSize, newLink);
            const int diff = newLinkSize - rawLinkSize;
            i = posEnd + diff;
            size += diff;
            if (posWWW >= 0)
                posWWW += diff;
            if (posHTTP >= 0)
                posHTTP += diff;
            if (posMAIL >= 0)
                posMAIL += diff;
        }
        else
            break;
    }
    return text;
}

void CreateDirectories(const QString& strFilename)
{
    /*
        This function is for creating a complete directory structure to a given
        file name. If there is a pathname like this:
        /html/files/images/myimage.gif
        this function create all the folders into MOTCache:
        /html
        /html/files
        /html/files/images
        QFileInfo only creates a file if the pathname is valid. If not all folders
        are created, QFileInfo will not save the file. There was no QT function
        or a hint the QT mailing list found in which does the job of this function.
    */
    for (int i = 0;; i++)
    {
#ifdef _WIN32
        int i1 = strFilename.indexOf(QChar('/'), i);
        int i2 = strFilename.indexOf(QChar('\\'), i);
        i = (i1 >= 0 && ((i1 < i2) || (i2<0))) ? i1 : i2;
#else
        i = strFilename.indexOf(QChar('/'), i);
#endif
        if (i < 0)
            break;
        const QString strDirName = strFilename.left(i);
        if (!strDirName.isEmpty() && !QFileInfo(strDirName).exists())
            QDir().mkdir(strDirName);
    }
}



QString getAMScheduleUrl()
{
    QDate d = QDate::currentDate();
    int month = d.month();
    int year = 0;
    char season = 'a';

    // transitions last sunday in March and October
    switch(month) {
    case 1:
    case 2:
        year = d.year()-1;
        season = 'b';
        break;
    case 3: {
        QDate s = d;
        s.setDate(d.year(), month+1, 1);
        s = s.addDays(0-s.dayOfWeek());
        if(d<s) {
            year = d.year()-1;
            season = 'b';
        } else {
            year = d.year();
            season = 'a';
        }
    }
    break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        year = d.year();
        season = 'a';
        break;
    case 10: {
        QDate s = d;
        s.setDate(d.year(), month+1, 1);
        int n = s.dayOfWeek();
        s = s.addDays(0-n);
        if(d<s) {
            year = d.year();
            season = 'a';
        } else {
            year = d.year();
            season = 'b';
        }
    }
    break;
    case 11:
    case 12:
        year = d.year();
        season = 'b';
    }
    return QString("http://eibispace.de/dx/sked-%1%2.csv").arg(season).arg(year-2000,2);
}

#ifdef QT_GUI_LIB


void ColumnParamFromStr(QTreeWidget* treeWidget, const QString& strColumnParam)
{
    QStringList list(strColumnParam.split(QChar('|')));
    const int n = list.count(); /* width and position */
    if (n == 2)
    {
        for (int j = 0; j < n; j++)
        {
            int c = treeWidget->header()->count();
            QStringList values(list[j].split(QChar(',')));
            const int lc = (int)values.count();
            if (lc < c)
                c = lc;
            for (int i = 0; i < c; i++)
            {
                int v = values[i].toInt();
                if (!j) /* width*/
                    treeWidget->header()->resizeSection(i, v);
                else /* position */
                    treeWidget->header()->moveSection(treeWidget->header()->visualIndex(i), v);
            }
        }
    }
    else
    {
        treeWidget->header()->resizeSections(QHeaderView::ResizeToContents);
        treeWidget->header()->resizeSections(QHeaderView::Interactive);
        treeWidget->header()->resizeSection(0, treeWidget->header()->minimumSectionSize());
    }
}

void ColumnParamToStr(QTreeWidget* treeWidget, QString& strColumnParam)
{
    strColumnParam = "";
    const int n = 2; /* width and position */
    for (int j = 0; j < n; j++)
    {
        const int c = treeWidget->header()->count();
        for (int i = 0; i < c; i++)
        {
            int v;
            if (!j) /* width*/
                v = treeWidget->header()->sectionSize(i);
            else /* position */
                v = treeWidget->header()->visualIndex(i);
            QString strValue;
            strValue.setNum(v);
            strColumnParam += strValue;
            if (i < (c-1))
                strColumnParam += ",";
        }
        if (j < (n-1))
            strColumnParam += "|";
    }
}
#endif

