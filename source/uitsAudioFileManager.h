/*
 *  uitsAudioFileManager.h
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 12/7/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date: 2010/02/17 21:44:42 $
 *  $Revision: 1.4 $
 *
 */


/*
 * Prevent multiple inclusion...
 */

#ifndef _uitsaudiofileManager_h
#  define _uitsaudiofileManager_h

/*
 * UITS Supported audio file types
 */ 
enum uitsAudioFileTypes {
	MP3
};

/*
 * Functions
 *
 */

char	*uitsAudioExtractPayload	(char *audioFileName);

int		uitsAudioEmbedPayload		(char *audioFileName, 
									 char *audioOutFileName, 
									 char *uitsPayloadXML,
									 int  numPadBytes);

char	*uitsAudioGetMediaHash		(char *audioFileName); 
int		uitsAudioGetAudioFileType	(char *audioFileName); 


#endif