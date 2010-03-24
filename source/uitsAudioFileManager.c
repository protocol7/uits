/*
 *  uitsAudioFileManager.c
 *  UITS_Tool
 *
 *  This manager provides wrapper calls to the audio file format-specific hash generation and validation functions.
 *  Version 1.0 of the tool only supports MP3.
 *
 *  Created by Chris Angelli on 12/7/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date: 2010/02/17 21:44:42 $
 *  $Revision: 1.5 $
 *
 */

#include "uits.h"

char *audioModuleName = "uitsAudioFileManager.c";


/*
 *
 * Function: uitsAudioEmbedPayload
 * Purpose:	 Embed the UITS payload into an audio file and write the output to a new file
 * Returns:  OK or ERROR
 *
 */

int uitsAudioEmbedPayload  (char *audioFileName, 
							char *audioOutFileName, 
							char *uitsPayloadXML,
							int	  numPadBytes)
{
	int audioFileType;
		
	audioFileType = uitsAudioGetAudioFileType (audioFileName);

	
	// For now, only MP3 audio files supported
	err = mp3EmbedPayload(audioFileName, audioOutFileName, uitsPayloadXML, numPadBytes);
	uitsHandleErrorINT(audioModuleName, "uitsAudioEmbedPayload", err, OK, "Couldn't embed UITS payload into audio file\n");
	
	return (OK);
	
}

/*
 *
 * Function: uitsAudioExtractPayload
 * Purpose:	 Extract the UITS payload from an audio file
 * Returns:  Pointer to string containing XML
 *
 */

char *uitsAudioExtractPayload (char *audioFileName)
{
	int audioFileType;
	char *uitsPayloadXML;
	
	
	audioFileType = uitsAudioGetAudioFileType (audioFileName);
	
	/* read the payload XML from the audio file */
	
	/* for now, only MP3 supported */
	uitsPayloadXML = mp3ExtractPayload (audioFileName);
	uitsHandleErrorPTR(audioModuleName, "uitsExtract", uitsPayloadXML, "Couldn't extract payload XML from audio file\n");
	
	
	return (uitsPayloadXML);
	
}

/*
 *
 * Function: uitsAudioGetMediaHash
 * Purpose:	 Open and read audiofile, determine audio file type, calculate hash value for media
 * Returns:  The media hash
 *
 */

char *uitsAudioGetMediaHash (char *audioFileName) 
{
	int audioFileType;
	char *mediaHashValue;
	
	// For now, only MP3 audio files supported
	
	audioFileType = uitsAudioGetAudioFileType (audioFileName);
	
	mediaHashValue = mp3GetMediaHash (audioFileName);
	
	return (mediaHashValue);
}


/*
 *
 * Function: uitsAudioGetAudioFileType
 * Purpose:	 Determine audio file type
 *
 */

int uitsAudioGetAudioFileType (char *audioFileName) 
{

	return (MP3);
}
