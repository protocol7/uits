/*
 *  uitsAudioFileManager.c
 *  UITS_Tool
 *
 *  This manager provides wrapper calls to the audio file format-specific callbacks.
 *  For each supported file type, the following callbacks must be implemented:
 *       IsValidFile:    Returns TRUE if the audio file is of the requested type
 *       GetMediaHash:   Generates a media hash for the audio data
 *       EmbedPayload:   Embeds a UITS payload into the audio file
 *       ExtractPayload: Extracts a UITS payload from the audio file
 *
 *  Version 1.0 of the tool only supports MP3.
 *  Version 2.0 adds MP4, FLAC, AIFF and WAV
 *
 *  Created by Chris Angelli on 12/7/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

#include "uits.h"

char *audioModuleName = "uitsAudioFileManager.c";

UITS_AUDIO_CALLBACKS uitsAudioCB [] = {
	{ MP3,		mp3IsValidFile,		mp3GetMediaHash,	 mp3EmbedPayload,		mp3ExtractPayload },
	{ MP4,		mp4IsValidFile,		mp4GetMediaHash,	 mp4EmbedPayload,		mp4ExtractPayload },
	{ FLAC,		flacIsValidFile,	flacGetMediaHash,	 flacEmbedPayload,		flacExtractPayload },
	{ AIFF,		aiffIsValidFile,	aiffGetMediaHash,	 aiffEmbedPayload,		aiffExtractPayload },
	{ WAV,		wavIsValidFile,		wavGetMediaHash,	 wavEmbedPayload,		wavExtractPayload },
	{ GENERIC,  genericIsValidFile, genericGetMediaHash, genericEmbedPayload,	genericExtractPayload },
	{ 0, 0, 0, 0, 0}
};

int bigEndianFlag = 1;

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
	UITS_AUDIO_CALLBACKS *currAudioCB;
	
	currAudioCB = uitsAudioGetCB (audioFileName);
	
	err = currAudioCB->uitsAudioEmbedPayload (audioFileName, audioOutFileName, uitsPayloadXML, numPadBytes);
	uitsHandleErrorINT(audioModuleName, "uitsAudioEmbedPayload", err, OK, ERR_EMBED, "Couldn't embed UITS payload into audio file\n");
	
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
	UITS_AUDIO_CALLBACKS *currAudioCB;
	char *uitsPayloadXML;
	
	currAudioCB = uitsAudioGetCB (audioFileName);
		
	/* read the payload XML from the audio file */
	
	uitsPayloadXML = currAudioCB-> uitsAudioExtractPayload (audioFileName);
	uitsHandleErrorPTR(audioModuleName, "uitsExtract", uitsPayloadXML, ERR_EXTRACT, "Couldn't extract payload XML from audio file\n");
	
	
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
	UITS_AUDIO_CALLBACKS *currAudioCB;
	char *mediaHashValue;
	
	
	currAudioCB = uitsAudioGetCB (audioFileName);
	
	mediaHashValue = currAudioCB->uitsAudioGetMediaHash (audioFileName);
	
	return (mediaHashValue);
}
/*
 *
 * Function: uitsAudioGetCB
 * Purpose:	 Determine audio file type and return pointers to callbacks for that type
 * Returns:  Pointer to callbacks
 *
 */

UITS_AUDIO_CALLBACKS *uitsAudioGetCB (char *audioFileName) {
	UITS_AUDIO_CALLBACKS *currAudioCB = uitsAudioCB;
	
	while (currAudioCB->uitsAudioIsValidFile) {
		if (currAudioCB->uitsAudioIsValidFile (audioFileName)) {
			return (currAudioCB);
		}
		currAudioCB++;
	}
	
	/* audio file is not one of our known types */
	uitsHandleErrorPTR(audioModuleName, "uitsAudioGetCB", NULL, ERR_AUD,  "Couldn't find audio file type \n");

}

/*
 *	Function: uitsAudioBufferedCopy
 *	Purpose:  Copy data through a buffer directly from the audio input file to the audio output file.
 *			  Leaves input and output file pointers at end of copied bytes
 *	Returns:  Number of bytes copied
 *
 */

int uitsAudioBufferedCopy (FILE *audioInFP, FILE *audioOutFP, unsigned long numBytes)
{
	unsigned char *ioBuffer = calloc(AUDIO_IO_BUFFER_SIZE, 1);
	unsigned long  bytesLeft;
	unsigned long  bufferSize;		/* size of the buffer to write */
	unsigned long  bytesRead;			/* number of bytes read from the input file */
	unsigned long  bytesWritten;		/* number of bytes written to the output file */
	unsigned long  totalBytesWritten = 0;
	
	// read and process the data in the file in  chunks 
	bytesLeft = numBytes;
	while (bytesLeft) {
		bufferSize = (bytesLeft > AUDIO_IO_BUFFER_SIZE) ? AUDIO_IO_BUFFER_SIZE : bytesLeft;
		bytesRead = fread(ioBuffer, 1, bufferSize, audioInFP);
		if (bytesRead != bufferSize) {
			uitsHandleErrorINT(audioModuleName, "uitsAudioBufferedCopy", ERROR, OK, ERR_FILE,
							   "Incorrect number of bytes read from audio input file\n");
		}
		bytesWritten = fwrite(ioBuffer, 1, bufferSize, audioOutFP);
		uitsHandleErrorINT(audioModuleName, "uitsAudioBufferedCopy",  bytesWritten, bufferSize, ERR_FILE, NULL);
		totalBytesWritten += bytesWritten;
		bytesLeft         -= bufferSize;
	}
	
	fflush(audioOutFP);
	free(ioBuffer);
	
	return (totalBytesWritten);
}

/*
 *  Housekeeping functions - to convert endian-ness of 2, 4, and 8-byte integers, when necessary...
 *
 *	First the 2-byte version...
 *
 */

void SetBigEndianFlag (int flag) {
	bigEndianFlag = flag;
}

int wswap(short *word)
{
	short w1, w2;
	
	
	if (bigEndianFlag) {
		w1 = w2 = *word;
		*word = (w1 << 8) & 0xff00;
		*word |= ((w2 >> 8) & 0x00ff);
	}
	return(0);
}

/*
 *  4-byte version...
 *
 */

int lswap(long *lword)
{
	union {
		long ul;
		struct {
			char b1, b2, b3, b4;
		} bytes ;
	} *pu, lu;
	
	if (bigEndianFlag) {
		dprintf("lswap intel_cpu defined\n");
		lu.ul = *lword;
		pu  = lword;
		pu->bytes.b4 = lu.bytes.b1;
		pu->bytes.b3 = lu.bytes.b2;
		pu->bytes.b2 = lu.bytes.b3;
		pu->bytes.b1 = lu.bytes.b4;
	}
	
	return(0);
}

/*
 *  8-byte version...
 *
 */

int llswap(unsigned long long *llword)
{
	unsigned long long ll1;
	union {
		unsigned long long ul;
		struct {
			char b1, b2, b3, b4, b5, b6, b7,b8;
		} bytes ;
	} *pu, lu;
	
	
	if (bigEndianFlag) {
		lu.ul = *llword;
		pu = llword;
		pu->bytes.b8 = lu.bytes.b1;
		pu->bytes.b7 = lu.bytes.b2;
		pu->bytes.b6 = lu.bytes.b3;
		pu->bytes.b5 = lu.bytes.b4;
		pu->bytes.b4 = lu.bytes.b5;
		pu->bytes.b3 = lu.bytes.b6;
		pu->bytes.b2 = lu.bytes.b7;
		pu->bytes.b1 = lu.bytes.b8;
	}
	
	return(0);
}

// EOF



