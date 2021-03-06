#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
#include	"mot-data.h"
#include	"gui.h"
#include	<QDir>
//
//	First attempt to do "something" with the MOT data
//
//	Two cases
//	The "single item" case, where an item is made up of an
//	header together with a body
//	The "directory" case, where a directory of files is maintained
//	to form together a slideshow or a website
//
		motHandler::motHandler (RadioInterface *mr) {
int16_t	i, j;

	for (i = 0; i < 16; i ++) {
	   table [i]. ordernumber = -1;
	   for (j = 0; j < 100; j ++)
	      table [i]. marked [j] = false;
	}
	ordernumber	= 1;
	theDirectory	= NULL;
	old_slide	= NULL;
	connect (this, SIGNAL (the_picture (QByteArray, int)),
	         mr, SLOT (showMOT (QByteArray, int)));
}

	 	motHandler::~motHandler (void) {
}
//
//	Process a regular header, i.e. a type 3
//	This strongly resembles the newEntry method that
//	creates a header for an item in a directory
void	motHandler::processHeader (int16_t	transportId,
	                           uint8_t	*segment,
	                           int16_t	segmentSize,
	                           int16_t	headerSize,
	                           int32_t	bodySize,
	                           bool		lastFlag) {
uint8_t contentType	= ((segment [5] >> 1) & 0x3F);
uint16_t contentsubType = ((segment [5] & 0x01) << 8) | segment [6];
int16_t	pointer	= 7;
QString	name 	= QString ("");;

	while (pointer < headerSize) {
	   uint8_t PLI = (segment [pointer] & 0300) >> 6;
	   uint8_t paramId = (segment [pointer] & 077);
	   uint16_t	length;
//	   fprintf (stderr, "PLI = %d, paramId = %d\n", PLI, paramId);
	   switch (PLI) {
	      case 00:
	         pointer += 1;
	         break;
	      case 01:
//	         if (paramId == 10)
//	            fprintf (stderr, "priority = %d\n",
//	                              segment [pointer + 1]);
	      
	         pointer += 2;
	         break;

	      case 02:
//	         if (paramId == 5) 
//	            fprintf (stderr, "triggertime = %d\n",
//	                             segment [pointer + 1] << 24 |
//	                             segment [pointer + 2] << 16 |
//	                             segment [pointer + 3] <<  8 |
//	                             segment [pointer + 4]);
	         pointer += 5;
	         break;

	      case 03:
	         if ((segment [pointer + 1] & 0200) != 0) {
	            length = (segment [pointer + 1] & 0177) << 8 |
	                      segment [pointer + 2];
	            pointer += 3;
	         }
	         else {
	            length = segment [pointer + 1] & 0177;
	            pointer += 2;
	         }
	         if (paramId == 12) {
	            int16_t i;
	            for (i = 0; i < length - 1; i ++) 
	               name. append (segment [pointer + i + 1]);
	         }
	         pointer += length;
	   } 
	}

	if (getHandle (transportId) != NULL)
	   return;

	if (lastFlag)	{ // single header
	   newEntry (transportId, bodySize,
	             contentType, contentsubType, name);
	   return;
	}
//	header segment contains header + segment data
//	fprintf (stderr, "combined %d\n", bodySize);
	newEntry (transportId,
	          bodySize,
	          contentType,
	          contentsubType,
	          name);
	processSegment (transportId, 
	                &segment [headerSize],
	                0,
	                segmentSize - headerSize,
	                false);
}

void	motHandler::processDirectory (int16_t	transportId,
                                      uint8_t	*segment,
                                      int16_t	segmentSize,
                                      bool	lastFlag) {
uint32_t directorySize	= ((segment [0] & 0x3F) << 24) |
	                  ((segment [1]       ) << 16) |
	                  ((segment [2]       ) <<  8) | segment [3];
uint16_t numObjects	= (segment [4] << 8) | segment [5];
int32_t	period		= (segment [6] << 16) | 
	                  (segment [7] <<  8) | segment [8];
int16_t segSize		= ((segment [9] & 0x1F) << 8) | segment [10];

	if ((theDirectory != NULL) &&
	                (theDirectory -> transportId == transportId)) 
	   return;		// already in!!

	if (theDirectory != NULL)	// other directory now
	   delete theDirectory;

	theDirectory = new MOT_directory (transportId, segmentSize,
	                                  directorySize, numObjects);
	memcpy (theDirectory -> dir_segments, segment, segmentSize);
	theDirectory -> marked [0] = true;
}

void	motHandler::directorySegment (uint16_t	transportId,
                                      uint8_t	*segment,
                                      int16_t	segmentNumber,
                                      int16_t	segmentSize,
                                      bool	lastSegment) {
int16_t	i;

	if (theDirectory == NULL)
	   return;
	if (theDirectory -> transportId != transportId)
	   return;
	if (theDirectory -> marked [segmentNumber])
	   return;
	if (lastSegment)
	   theDirectory -> num_dirSegments = segmentNumber + 1;
	theDirectory	-> marked [segmentNumber] = true;
	uint8_t	*address = &theDirectory -> dir_segments [segmentNumber *
	                                    theDirectory -> dir_segmentSize];
	memcpy (address, segment, segmentSize);
//
//	we are "complete" if we know the number of segments and
//	all segments are "in"
	if (theDirectory -> num_dirSegments != -1) {
	   for (i = 0; i < theDirectory -> num_dirSegments; i ++)
	      if (!theDirectory -> marked [i])
	         return;
	}
//
//	yes we have all data to build up the directory
	analyse_theDirectory ();
}
//
//	The directory
void	motHandler::analyse_theDirectory (void) {
uint16_t	numObjects	= theDirectory -> numObjects;
uint16_t	currentBase	= 11;	// in bytes
uint8_t	*data			= theDirectory -> dir_segments;
uint16_t extensionLength	= (data [currentBase] << 8) |
	                                            data [currentBase + 1];
int16_t	i;

	currentBase += 2 + extensionLength;
	for (i = 0; i < numObjects; i ++)
	   currentBase = get_dirEntry (i, data, currentBase);
}

int16_t	motHandler::get_dirEntry	(int16_t	index,
	                                 uint8_t	*data,
	                                 uint16_t	currentBase) {
QString		name ("");

uint16_t transportId	=  (data [currentBase] << 8) | data [currentBase + 1];
uint32_t bodySize	=  (data [currentBase + 2] << 20) |
	                   (data [currentBase + 3] << 12) |
	                   (data [currentBase + 4] <<  4) |
	                  ((data [currentBase + 5] & 0xF0) >> 4);
uint16_t headerSize	= ((data [currentBase + 5] & 0x0F) << 9) |
	                   (data [currentBase + 6] << 1) |
	                  ((data [currentBase + 7] >> 7) & 0x01);
uint8_t  contentType	=  (data [currentBase + 7] >> 1) & 0x3F;
uint16_t subType	= ((data [currentBase + 7] & 0x1) << 8) |
	                    data [currentBase + 8];
uint16_t theEnd		= currentBase + 2 + headerSize;

	currentBase	+= 7 + 2;
	while (currentBase < theEnd) {
	   uint8_t PLI = (data [currentBase] & 0300) >> 6;
	   uint8_t paramId = (data [currentBase] & 077);
	   uint16_t	length;
//	   fprintf (stderr, "PLI = %d, paramId = %d\n", PLI, paramId);
	   switch (PLI) {
	      case 00:
	         currentBase += 1;
	         break;
	      case 01:
//	         if (paramId == 10)
//	            fprintf (stderr, "priority = %d\n",
//	                              data [pointer + 1]);
	      
	         currentBase += 2;
	         break;

	      case 02:
//	         if (paramId == 5) 
//	            fprintf (stderr, "triggertime = %d\n",
//	                             data [currentBase + 1] << 24 |
//	                             data [currentBase + 2] << 16 |
//	                             data [currentBase + 3] <<  8 |
//	                             data [currentBase + 4]);
	         currentBase += 5;
	         break;

	      case 03:
	         if ((data [currentBase + 1] & 0200) != 0) {
	            length = (data [currentBase + 1] & 0177) << 8 |
	                      data [currentBase + 2];
	            currentBase += 3;
	         }
	         else {
	            length = data [currentBase + 1] & 0177;
	            currentBase += 2;
	         }
	         if (paramId == 12) {
	            int16_t i;
	            for (i = 0; i < length - 1; i ++) 
	               name. append (data [currentBase + i + 1]);
	         }
	         currentBase += length;
	   } 
	}
//
//	creating an entry for an object mentioned in the directory
//	strongly resembles creating a standalone entry, some differences though
	newEntry (index, transportId, bodySize,
	          contentType, subType, name);
	return currentBase;
}

void	motHandler::processSegment	(int16_t	transportId,
	                                 uint8_t	*segment,
	                                 int16_t	segmentNumber,
	                                 int16_t	segmentSize,
	                                 bool		lastFlag) {
int16_t	i;

	motElement *handle = getHandle (transportId);
	if (handle == NULL)
	   return;
	if (handle -> marked [segmentNumber])
	   return;

//	Note that the last segment may have a different size
	if (!lastFlag && (handle -> segmentSize == -1))
	   handle -> segmentSize = segmentSize;

	if (handle -> segmentSize == -1)
	   return;

//	sanity check
	if (segmentNumber * handle -> segmentSize + segmentSize >
	                                                handle -> bodySize)
	   return;

	for (i = 0; i < segmentSize; i ++)
	   handle -> body [segmentNumber *
	                         handle -> segmentSize + i] = segment [i];
	handle -> marked [segmentNumber] = true;
	if (lastFlag) 
	   handle -> numofSegments = segmentNumber + 1;

	if (isComplete (handle)) 
	   handleComplete (handle);
}
//
//	we have data for all directory entries
void	motHandler::handleComplete (motElement *p) {
int16_t i;
	if (p -> contentType == 2) 
	   show_slide (p);
	else
	if (p -> contentType == 7)
	   handle_epgTopElement	(p);
	else {
	   fprintf (stderr, "going to write file %s\n", (p ->  name). toLatin1 (). data ());
	   fprintf (stderr, "contenttype = %d\n", p -> contentType);
	   checkDir (p -> name);
//
	   FILE *x = fopen (((p -> name). toLatin1 (). data ()), "w");
	   if (x == NULL)
	      fprintf (stderr, "cannot write file %s\n",
	                                     (p -> name). toLatin1 (). data ());
	   else {
	      (void)fwrite ((p -> body). data (), 1, p -> bodySize, x);
	      fclose (x);
	   }
	}
}
void	motHandler::show_slide (motElement *p) {
int16_t i;
	if (old_slide != NULL)
	   for (i = 0; i < p ->  numofSegments; i ++)
	      p -> marked [i] = false;
	fprintf (stderr, "going to show picture %s\n",
	                                   (p -> name). toLatin1 (). data ());
	the_picture (p -> body, p -> contentsubType);
	old_slide	= p;
}

void	motHandler::handle_epgTopElement (motElement *p) {
uint8_t	*body	= (uint8_t *)(p -> body. data ());
uint8_t	tag	= body [0];
int32_t	length;
int32_t	base;
int16_t	localBase;

	return;
//
//	get the length
	if (body [1] == 0xFE) {
	   length	= (body [2] << 8) | body [3];
	   base		= 4;
	}
	else 
	if (body [1] == 0xFF) {
	   length	= (((body[2] << 8) | body [3]) << 8) | body [4];
	   base		= 5;
	}
	else {		// short body
	   length	= body [1];
	   base		= 2;
	}

	length	+= base;
	while ((base < length) && ((body [base] & 0x80) == 0x80))
	   base	+= handle_epgAttribute (body, base);

	while ((base < length) && (body [base] == 0x04)) // string token table
	   base += handle_epgStringTokenTable (body, base);

	while ((base < length) && (body [base] == 0x05))	// 
	   base += handle_epgDefaultContentId (body, base);

	while ((base < length) && (body [base] == 0x06))	//
	   base	+= handle_epgDefaultLanguage (body, base);
	
	while ((base < length) && (body [base] != 0x01))
	   base += handle_epgChildElement (body, base);

	while ((base < length) && (body [base] == 0x1))	// CDATA
	      base += handle_epgCData (body, base);
}

int16_t	motHandler::handle_epgAttribute (uint8_t *data, int16_t base) {
int32_t	elementLength;
int16_t	localBase;

	fprintf (stderr, "found attr %x on base %d\n", data [base], base);
	if (data [base + 1] == 0xFE) {
	   elementLength = (data [base + 2] << 8) | data [base + 3];
	   localBase	= 4;
	}
	else
	if (data [base + 1] == 0xFF) {
	   elementLength = (((data [base + 2] << 8) | data [base + 3]) << 8) |
	                                       data [base + 4];
	   localBase	= 5;
	}
	else {		// short segment
	   elementLength	= data [base + 1];
	   localBase		= 2;
	}
//	do something useful here
	fprintf (stderr, "Length of attribute %d\n", localBase + elementLength);
	return localBase + elementLength;
}

int16_t	motHandler::handle_epgStringTokenTable (uint8_t *data, int16_t base) {
int32_t	elementLength;
int16_t	localBase;

	if (data [base + 1] == 0xFE) {
	   elementLength = (data [base + 2] << 8) | data [base + 3];
	   localBase	= 4;
	}
	else
	if (data [base + 1] == 0xFF) {
	   elementLength = (((data [base + 2] << 8) | data [base + 3]) << 8) |
	                     data [base + 4];
	   localBase	= 5;
	}
	else {		// short segment
	   elementLength	= data [base + 1];
	   localBase		= 2;
	}
//	do something useful here
	return localBase + elementLength;
}

int16_t	motHandler::handle_epgDefaultContentId (uint8_t *data, int16_t base) {
int32_t	elementLength;
int16_t	localBase;

	if (data [base + 1] == 0xFE) {
	   elementLength = (data [base + 2] << 8) | data [base + 3];
	   localBase	= 4;
	}
	else
	if (data [1] == 0xFF) {
	   elementLength = (((data [base + 2] << 8) | data [base + 3]) << 8) |
	                                          data [base + 4];
	   localBase	= 5;
	}
	else {		// short segment
	   elementLength	= data [base + 1];
	   localBase		= 2;
	}
//	do something useful here
	return localBase + elementLength;
}

int16_t motHandler::handle_epgDefaultLanguage (uint8_t *data, int16_t base) {
int32_t	elementLength;
int16_t	localBase;
int16_t	bottom;

	if (data [base + 1] == 0xFE) {
	   elementLength = (data [base + 2] << 8) | data [base + 3];
	   localBase	= 4;
	}
	else
	if (data [base + 1] == 0xFF) {
	   elementLength = (((data [base + 2] << 8) | data [base + 3]) << 8) |
	                          data [base + 4];
	   localBase	= 5;
	}
	else {		// short segment
	   elementLength	= data [base + 1];
	   localBase		= 2;
	}
//	do something useful here
	return localBase + elementLength;
}

static int depth	= 0;

int16_t motHandler::handle_epgChildElement (uint8_t *body, int16_t base) {
int32_t	length;
int16_t	localLength;
int16_t	localBase;
int16_t	localEnd;

int	i;
	if ((body [base] == 0) || (body [base] > 40))
	   return 6;
	depth	++;
	if (depth > 20)
	   exit (21);

	for (i = 0; i < depth; i ++)
	   fprintf (stderr, "*");
//	get the length
	if (body [base + 1] == 0xFE) {
	   length	= (body [base + 2] << 8) | body [base + 3];
	   localBase		= 4;
	}
	else 
	if (body [base + 1] == 0xFF) {
	   length	= (((body [base + 2] << 8) | body [base + 3]) << 8) |
	                       body [base + 4];
	   localBase		= 5;
	}
	else {		// short body
	   length	= body [base + 1];
	   localBase		= 2;
	}

	fprintf (stderr, "Handling tag %x (%c) at base %d (up to %d)\n",
	       body [base], body [base], base, base + localBase + length);
	base		+= localBase;
	localEnd	= base + length;

	while ((base < localEnd) && ((body [base] & 0x80) == 0x80))
	   base += handle_epgAttribute (body, base);

	while ((base < localEnd) && (body [base] != 0x01))
	   base += handle_epgChildElement (body, base);

	while ((base < localEnd) && (body [base] == 0x01))
	   base += handle_epgCData (body, base);

	for (i = 0; i < depth; i ++)
	   fprintf (stderr, "*");
	fprintf (stderr, "Leaving level %d with base = %d, size %d\n", depth, base, localBase + length);
	depth --;
	return localBase + length;
}
	
int16_t motHandler::handle_epgCData (uint8_t *data, int16_t base) {
int32_t	elementLength;
int16_t	localBase;
int16_t	i;

	if (data [base + 1] == 0xFE) {
	   elementLength = (data [base + 2] << 8) | data [base + 3];
	   localBase	= 4;
	}
	else
	if (data [base + 1] == 0xFF) {
	   elementLength = (data [base + 2] << 16) | (data [base + 3] << 8) |
	                                    data [base + 4];
	   localBase	= 5;
	}
	else {		// short segment
	   elementLength	= data [base + 1];
	   localBase		= 2;
	}
	for (i = 0; i < elementLength; i ++)
	   fprintf (stderr, "%c", data [base + localBase + i]);
	fprintf (stderr, "\n");
	return localBase + elementLength;
}
	
void	motHandler::checkDir (QString &s) {
int16_t	ind	= s. lastIndexOf (QChar ('/'));
int16_t	i;
QString	dir;
	if (ind == -1)		// no slash, no directory
	   return;

	for (i = 0; i < ind; i ++)
	   dir. append (s [i]);

	if (QDir (dir). exists ())
	   return;
	QDir (). mkpath (dir);
}

	
bool	motHandler::isComplete (motElement *p) {
int16_t	i;

	if (p -> numofSegments == -1)
	   return false;
	for (i = 0; i < p ->  numofSegments; i ++)
	   if (!(p -> marked [i]))
	      return false;

	return true;
}

motElement	*motHandler::getHandle (uint16_t transportId) {
int16_t	i;
//
//	we first look for the "free" MOT slides, then
//	for the carrousel
	for (i = 0; i < 16; i ++)
	   if (table [i]. ordernumber != -1 &&
	                  table [i]. transportId == transportId)
	      return &table [i];
	if (theDirectory == NULL)
	   return NULL;

	for (i = 0; i < theDirectory -> numObjects; i ++) {
	   if (theDirectory -> dir_proper [i]. ordernumber == -1)
	      continue;
	   if (theDirectory -> dir_proper [i]. transportId == transportId)
	      return &(theDirectory -> dir_proper [i]);
	}
	return NULL;
}
//
//	Handling a plain header is by:
void	motHandler::newEntry (uint16_t	transportId,
	                      int16_t	size,
	                      int16_t	contentType,
	                      int16_t	contentsubType,
	                      QString	name) {
int16_t		i;
uint16_t	lowest;
int16_t		lowIndex;

	for (i = 0; i < 16; i ++) {
	   if (table [i]. ordernumber == -1) {
	      table [i]. ordernumber	= ordernumber ++;
	      table [i]. transportId	= transportId;
	      table [i]. body. resize (size);
	      table [i]. bodySize	= size;
	      table [i]. contentType	= contentType;
	      table [i]. contentsubType	= contentsubType;
	      table [i]. segmentSize	= -1;
	      table [i]. numofSegments	= -1;
	      table [i]. name		= QString (name);
	      return;
	   }
	}
//
//	table full, delete the oldest one
//
	lowest		= 65377;
	lowIndex	= 0;
	for (i = 0; i < 16; i ++) {
	   if (table [i]. ordernumber < lowest) {
	      lowIndex = i;
	      lowest = table [i]. ordernumber;
	   }
	}

	table [lowIndex]. ordernumber	= ordernumber ++;
	table [lowIndex]. transportId	= transportId;
	table [lowIndex]. body. resize (size);
	table [lowIndex]. bodySize	= size;
	table [lowIndex]. contentType	= contentType;
	table [lowIndex]. contentsubType	= contentsubType;
	table [lowIndex]. segmentSize	= -1;
	table [lowIndex]. numofSegments	= -1;
	table [lowIndex]. name		= name;
}
//
//	handling an entry in a directory is
void	motHandler::newEntry (int16_t	index,
	                      uint16_t	transportId,
	                      int16_t	size,
	                      int16_t	contentType,
	                      int16_t	contentsubType,
	                      QString	name) {
motElement	*currEntry = &(theDirectory -> dir_proper [index]);

	currEntry -> ordernumber	= ordernumber ++;
	currEntry -> transportId	= transportId;
	currEntry -> body. resize (size);
	currEntry -> bodySize		= size;
	currEntry -> contentType	= contentType;
	currEntry -> contentsubType	= contentsubType;
	currEntry -> segmentSize	= -1;
	currEntry -> numofSegments	= -1;
	currEntry -> name		= QString (name);
//	fprintf (stderr, "dir entry %s\n", currEntry -> name. toLatin1 (). data ());
}

void	motHandler::process_mscGroup (uint8_t	*data,
	                              uint8_t	groupType,
	                              bool	lastSegment,
	                              int16_t	segmentNumber,
	                              uint16_t	transportId) {
uint16_t segmentSize	= ((data [0] & 0x1F) << 8) | data [1];

	if ((segmentNumber == 0) && (groupType == 3)) { // header
	   uint32_t headerSize	= ((data [5] & 0x0F) << 9) |
	                           (data [6])              |
	                           (data [7] >> 7);
	   uint32_t bodySize	= (data [2] << 20) |
	                          (data [3] << 12) |
	                          (data [4] << 4 ) |
	                          ((data [5] & 0xF0) >> 4);
	   processHeader (transportId,
	                  &data [2],
	                  segmentSize,
	                  headerSize,
	                  bodySize,
	                  lastSegment);
	}
	else
	if ((segmentNumber == 0) && (groupType == 6)) 	// MOT directory
	    processDirectory (transportId,
	                      &data [2],
	                      segmentSize,
	                      lastSegment);
	else
	if (groupType == 6) 	// fields for MOT directory
	   directorySegment (transportId,
	                     &data [2],
	                     segmentNumber,
	                     segmentSize,
	                     lastSegment);
	else
	if (groupType == 4) {
//	   fprintf (stderr, "grouptype = %d, Ti = %d, sn = %d, ss = %d\n",
//	                     groupType, transportId, segmentNumber, segmentSize);

	   processSegment  (transportId,
	                    &data [2],
	                    segmentNumber,
	                    segmentSize,
	                    lastSegment);
	}
//	else
//	   fprintf (stderr, "grouptype = %d, Ti = %d, sn = %d, ss = %d\n",
//	                     groupType, transportId, segmentNumber, segmentSize);
}

