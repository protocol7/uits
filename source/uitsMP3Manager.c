/*
 *  uitsMP3Manager.c
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 12/8/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

#include "uits.h"


char *mp3ModuleName = "uitsMP3Manager.c";

int   id3v1TagSize = 128L;
char *id3privFrameEmail = "mailto:uits-info@umusic.com";


// int   id3v22Flag; // version 1.0 of tool only supports MP3 ID3 v23

unsigned char header[MP3_HEADER_SIZE]; /* MP3_HEADER_SIZE is the maximum size (10 bytes) used for MP3 2.3 headers */

long bitrates[] =			{	0,
								32000,
								40000, 
								48000, 
								56000, 
								64000, 
								80000, 
								96000, 
								112000, 
								128000, 
								160000, 
								192000, 
								224000, 
								256000, 
								320000, 
								-1};

long samplerates[] =		{	44100, 
								48000, 
								32000, 
								0};

char *channel_modes[] =		{	"        Stereo", 
								"  Joint Stereo", 
								"  Dual Channel", 
								"Single Channel"};

char *mode_extensions[] =	{	"bands  4-31", 
								"bands  8-31", 
								"bands 12-31", 
								"bands 16-31"};

char *emphasis[] =			{	"     none", 
								" 50/15 ms", 
								" reserved", 
								"CCIT J.17"};


/*
 *
 * Function: mp3IsValidFile
 * Purpose:	 Determine if an audio file is an MP3 file
 *
 * Returns:   TRUE if MP3, FALSE otherwise
 */

int mp3IsValidFile (char *audioFileName) 
{
	
	FILE *audioFP;
	MP3_ID3_HEADER		   *mp3ID3Header;
	
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp3ModuleName, "mp3IsValidFile", audioFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	/* If this is an MP3 file, it will start with an ID3 tag header */
	mp3ID3Header = mp3ReadID3Header(audioFP);
	fclose (audioFP);
	
	if (mp3ID3Header) {
		vprintf("Audio file is MP3\n");
		
		/* for the first version of the tool, MP3 audio files must be ID3 version 2.3 */
		mp3CheckFileVersion (audioFileName);
		return (TRUE);
	} else {
		return (FALSE);
	}
	
}

/*
 *
 * Function: mp3GetMediaHash
 * Purpose:	 Calcluate the media hash for an MP3 file
 *				1. Read the size of the ID3 tag and skip over it
 *				2. If the first frame of audio is a xing, info or VBRI frame, skip it.
 *				3. If there is a 128 byte ID3v1 tag at the end of the file, make sure skip it
 *				4. Read and hash the audio frames until EOF or ID3v1 tag
 *
 * Returns:   Pointer to the hashed frame data
 */

char *mp3GetMediaHash (char *audioFileName) 
{
	FILE *audioFP;
	MP3_ID3_HEADER		   *mp3ID3Header;
	MP3_AUDIO_FRAME_HEADER *mp3AudioFrameHeader;
	int audioFrameStart, audioFrameEnd, audioFrameLength;
	UITS_digest *mediaHash;
	char *mediaHashString;
	int foundPadBytes;
	

	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp3ModuleName, "mp3GetMediaHash", audioFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	/* The file should start with an ID3 tag header */
	mp3ID3Header = mp3ReadID3Header(audioFP);
	uitsHandleErrorPTR(mp3ModuleName, "mp3GetMediaHash", audioFP, ERR_MP3, "Couldn't read ID3 Tag header\n");
	
	/* seek to the first audio frame, which will be after the MP3 ID3 tag header (10 bytes) */
	audioFrameStart = MP3_HEADER_SIZE + mp3ID3Header->size;
	fseeko(audioFP, audioFrameStart, SEEK_CUR);
	
	/* skip pad bytes if any */
	mp3SkipPadBytes (audioFP);
	
	foundPadBytes = ftello(audioFP) - audioFrameStart;
	
	if (foundPadBytes) {
		uitsHandleErrorINT(mp3ModuleName, "mp3GetMediaHash", ERROR, OK, ERR_MP3,
						   "Warning: Illegal MP3 file. ID3 frame header size does not include pad bytes\n");
	}
	
	
	mp3AudioFrameHeader = mp3ReadAudioFrameHeader(audioFP);
	uitsHandleErrorPTR(mp3ModuleName, "mp3GetMediaHash", mp3AudioFrameHeader, ERR_MP3, "Coudln't read Audio Frame Header\n");
	// vprintf("Read first Audio Frame: \n");
	// vprintf("\tframe length: %04ld bitrate: %06ld samplerate: %06ld \n", 
	//		mp3AudioFrameHeader->frameLength, mp3AudioFrameHeader->bitrate, mp3AudioFrameHeader->samplerate);	
	
	/* if the first frame of audio is a VBR frame (XING, Info, VBR), skip it */
	if (mp3AudioFrameHeader->vbrHeaderflag) {
		vprintf("Skipping XING Frame\n");
		fseeko(audioFP, mp3AudioFrameHeader->frameLength, SEEK_CUR);
	}
	
	audioFrameStart = ftello(audioFP);
	
	/* calculate how long the audio frame data is by seeking to EOF and saving size  */
	fseeko(audioFP, 0L, SEEK_END);
	audioFrameEnd = ftello(audioFP);	
	fseeko(audioFP, audioFrameStart, SEEK_SET);
	
	/* skip the 128 byte ID3v1 tag at the end of the file, if it's there */
	if (mp3HasID3v1Tag(audioFP)) {
		audioFrameEnd -= id3v1TagSize;
	}
	
	audioFrameLength = audioFrameEnd - audioFrameStart;
	
	mediaHash = uitsCreateDigestBuffered (audioFP, audioFrameLength, "SHA256") ;
	
	mediaHashString = uitsDigestToString(mediaHash);
	
	/* cleanup */
	fclose(audioFP);
	
	/* CMA: these two cleanup calls were coredumping under windows. need to investigate */
	//	free(mp3AudioFrameHeader);
	//	free(mp3ID3Header);
	
	return (mediaHashString);
}

/*
 *
 * Function: mp3EmbedPayload
 * Purpose:	 Embed the UITS payload into an MP3 file
 *				1. Read the ID3 tag and write every frame to output file
 *				2. Remove existing pad bytes, if any
 *				3. When first audio frame is detected, write PRIV tag containing UITS payload
 *				4. Write any user-requested pad bytes
 *				5. Adjust ID3 tag size
 *				6. Read and write audio
 *
 * Returns:   OK or ERROR
 */
 
int mp3EmbedPayload  (char *audioFileName, 
					  char *audioFileNameOut, 
					  char *uitsPayloadXML,
					  int  numPadBytes) 
{
	FILE			*audioInFP, *audioOutFP;
	int				frameType;
	unsigned long	id3TagSize;
	MP3_ID3_HEADER	*id3Header;
	unsigned long	audioFrameStart, audioFrameEnd, audioFrameLength;

	
	vprintf("About to embed payload for %s into %s\n", audioFileName, audioFileNameOut);
	
	/* open the audio input and output files */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp3ModuleName, "mp3EmbedPayload", audioInFP, ERR_FILE, "Couldn't open audio file for reading\n");

	audioOutFP = fopen(audioFileNameOut, "wb");
	uitsHandleErrorPTR(mp3ModuleName, "mp3EmbedPayload", audioOutFP, ERR_FILE, "Couldn't open audio file for writing\n");
	
	/* read the original ID3 header for use later */
	id3Header = mp3ReadID3Header(audioInFP);
	
	/* read and write ID3 Tags and Frames, consume padding until the first audio frame */
	while ((frameType = mp3IdentifyFrame(audioInFP)) != AUDIOFRAME) {
		switch (frameType) {
			case ID3V2TAG:		// this handles the outer metadata boundary, a "tag"
				mp3HandleID3Tag (audioInFP, audioOutFP);	
				break;
				
			case ID3FRAME:		// this handles an inner metadata item, an ID3 "frame"
				mp3HandleID3Frame (audioInFP, audioOutFP);	
				break;
				
			case PADDING:		// remove the padding by skipping
				mp3SkipPadBytes (audioInFP);
				break;
				
			case AUDIOFRAME:		// ID3 v1 tag should not occur until after the first audio frame
				uitsHandleErrorINT(mp3ModuleName, "mp3EmbedPayload", AUDIOFRAME, 0, ERR_MP3,
								"Error: Should insert Priv tag here\n");
				break;
				
			case ID3V1TAG:		// ID3 v1 tag should not occur until after the first audio frame
				uitsHandleErrorINT(mp3ModuleName, "mp3EmbedPayload", ID3V1TAG, 0, ERR_MP3,
								"Error: Found ID3V1 Tag before first Audio Frame\n");
				break;
				
			default:
				vprintf("Unidentified frame at %ld\n", (ftello(audioInFP) - 4));
				break;
		}
		
	}
	
	/* audioOutFP now points to the end of the copied ID3 frames. Write the new PRIV frame */
	err = mp3WritePRIVFrame(audioOutFP, uitsPayloadXML);
	uitsHandleErrorINT(mp3ModuleName, "mp3EmbedPayload", err, OK, ERR_MP3, "Couldn't write PRIV frame to audio output file\n");
	
	/* Write any requested pad bytes */
	err = mp3WritePadBytes (audioOutFP, numPadBytes);
	uitsHandleErrorINT(mp3ModuleName, "mp3EmbedPayload", err, OK, ERR_MP3, "Couldn't add pad bytes to ID3 tag\n");

	/* Update the ID3V2 tag size */
	id3TagSize = ftello(audioOutFP);
	id3Header->size = 	id3TagSize - 10;		/* size does not include 10 header bytes */
	fseeko(audioOutFP, 0, SEEK_SET);				/* seek to start of file for writing */

	err = mp3WriteID3Header(audioOutFP, id3Header); 
	uitsHandleErrorINT(mp3ModuleName, "mp3EmbedPayload", err, OK, ERR_MP3, "Couldn't write MP3 ID3 header\n");
	
	/* reset file pointer to end of ID3 frames */
	fseeko(audioOutFP, id3TagSize, SEEK_SET);				/* seek to start of file for writing */

	/* copy the remaining audio frames from the input file to the output file */	
	/* calculate how long the audio frame data is by seeking to EOF and saving size  */
	audioFrameStart = ftello(audioInFP);
	fseeko(audioInFP, 0L, SEEK_END);
	audioFrameEnd = ftello(audioInFP);		/* note that we're just going to pass the ID3V1TAG through */
	
	fseeko(audioInFP, audioFrameStart, SEEK_SET);
	audioFrameLength = audioFrameEnd - audioFrameStart;
	
	uitsAudioBufferedCopy(audioInFP, audioOutFP, audioFrameLength);
	
	
	/* cleanup */
	fclose(audioInFP);
	fclose(audioOutFP);
	
	return(OK);
}

/*
 *
 * Function: mp3ExtractPayload
 * Purpose:	 Extract the UITS payload from an MP3 file
 *				1. Read the ID3 tag 
 *				2. Skip over frames until PRIV frame is found
 *				3. Seek to <xml> data in PRIV frame
 *				4. return pointer to payload XML
 *
 * Returns: pointer to payload or exit if error
 */

char *mp3ExtractPayload (char *audioFileName) 

{
	FILE			*audioInFP;
	int				frameType;
	char			*uitsPayloadXML = NULL;
	
	
	vprintf("\tAbout to extract payload from %s\n", audioFileName);
	
	/* open the audio input and output files */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp3ModuleName, "mp3ExtractUITSPayload", audioInFP, ERR_FILE, "Couldn't open audio file for reading\n");
	

		/* read ID3 Tags and Frames, consume padding until a PRIV frame containing the UITS payload is found */
	while (!uitsPayloadXML) {
		frameType = mp3IdentifyFrame(audioInFP);
		switch (frameType) {
			case ID3V2TAG:		// this skips the outer metadata boundary, a "tag"
				mp3HandleID3Tag (audioInFP, NULL);	
				break;
				
			case ID3FRAME:		// does this frame conatin the UITS Payload?
				uitsPayloadXML =  mp3FindUITSPayload (audioInFP); 
				break;
				
			case PADDING:		// remove the padding by skipping
				mp3SkipPadBytes (audioInFP);
				break;
				
			case AUDIOFRAME:		// We should not encounter an Audio frame. UITS payload should be found before this.
				uitsHandleErrorINT(mp3ModuleName, "mp3ExtractPayload", AUDIOFRAME, 0, ERR_MP3,  
								"Error: Did not find UITS payload in audio file\n");
				break;
				
			case ID3V1TAG:		// We should not encounter an ID3 V1 tag. UITS payload should be found before this
				uitsHandleErrorINT(mp3ModuleName, "mp3ExtractPayload", ID3V1TAG, 0, ERR_MP3,  
								"Error: Did not find UITS payload in audio file\n");
				break;
				
			default:
				vprintf("Unidentified frame at %ld\n", (ftello(audioInFP) - 4));
				break;
		}
		
	}
	
	return (uitsPayloadXML);
	
}

/*
 *
 * Function: mp3CheckFileVersion
 * Purpose:	 Check the mp3 file's version. This tool currently only supports ID3v2.3
 * Returns:  OK or exit if unsuported version

 *
 */

int mp3CheckFileVersion (char *audioFileName) 
{
	MP3_ID3_HEADER		   *id3Header;
	FILE *audioInFP;
	
	/* open the audio input and output files */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp3ModuleName, "mp3ExtractUITSPayload", audioInFP, ERR_FILE, "Couldn't open audio file for reading\n");

	/* read the original ID3 header for use later */
	id3Header = mp3ReadID3Header(audioInFP);
	uitsHandleErrorPTR(mp3ModuleName, "mp3ExtractUITSPayload", id3Header, ERR_MP3, "Couldn't read ID3 header\n");
	
	if (id3Header->majorVersion != 3) {
		exit(OK);
		snprintf(errStr, ERRSTR_LEN, 
				 "MP3 file ID3 Tag format ID3V2.%d.%d not supported.\n", 
				 id3Header->majorVersion, 
				 id3Header->minorVersion);
		uitsHandleErrorINT(mp3ModuleName, "mp3ExtractUITSPayload", ERROR, OK, ERR_MP3, errStr);

	}
	
	fclose(audioInFP);
	return (OK);
	
}
/*
 *	Function: mp3HandleID3Tag
 *	Purpose:  Read the ID3 Tag from the audio input file 
 *			  If an audio output FP is non-NULL, write it to the audio output file
 *			  Input and output FP are left at end of tag in file
 *  Returns: OK or exit on error
 *
 */

int mp3HandleID3Tag (FILE *audioInFP, FILE *audioOutFP)

{
	
	err = fread(header, 1L, MP3_HEADER_SIZE, audioInFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3HandleID3Tag", err, MP3_HEADER_SIZE, ERR_FILE,
					"Couldn't read MP3 ID3 header from audio input file\n");
	
	
	/* vprintf("Tag format: ID3V2.%d.%d\n", (int) header[3], (int) header[4]); */

	/* version 1.0 of uits tool only supports ID3 v2.3 */
	/* Hack to deal with converting 4-byte frame tags to 3-byte frame tags */
	/*
	if ((int) header[3] < 3)
		id3v22Flag = TRUE;
	else {
		id3v22Flag = FALSE;
	}	
	*/
	
	if (audioOutFP) {
		err = fwrite(header, 1L, MP3_HEADER_SIZE, audioOutFP);
		uitsHandleErrorINT(mp3ModuleName, "mp3HandleID3Tag", err, MP3_HEADER_SIZE, ERR_FILE, 
						"Couldn't write MP3 ID3 header to audio output file\n");
	}
	
	return(OK);
}

/*
 *	Function:	mp3HandleID3Frame
 *	Purpose:	Read an ID3 frame and write it to the audio output file. 
 *					Input and output FP start at beginning of frame and are left at end of Frame
 *  Returns: OK or exit on error
 *
 */

int mp3HandleID3Frame (FILE *audioInFP, FILE *audioOutFP)

{
	
	unsigned long	 size = 0;

	err = fread(header, 1, MP3_HEADER_SIZE, audioInFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3HandleID3Frame", err, MP3_HEADER_SIZE, ERR_FILE,
					"Couldn't read MP3 ID3 header from audio input file\n");
	
	/* this is where we would check the id3v22Flag and do the 3-byte to 4-byte tag conversion */
	/* that includes handling the fact that an ID3v2.2 header is only 6 bytes, not 10 */
	
	size =  (unsigned long) header[7];
	size += (unsigned long) header[6] * 256L;
	size += (unsigned long) header[5] * 256L * 256L;
	size += (unsigned long) header[4] * 256L * 256L * 256L;
	
	/* write the header to the output file */
	//	dprintf("ID3 Frame length: %ld", size);
	err = fwrite(header, 1, MP3_HEADER_SIZE, audioOutFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3HandleID3Frame", err, MP3_HEADER_SIZE, ERR_FILE,
					"Couldn't write MP3 ID3 header to audio output file\n");


	/* write the frame to the output file */

	err = uitsAudioBufferedCopy (audioInFP, audioOutFP, size);
	uitsHandleErrorINT(mp3ModuleName, "mp3HandleID3Frame", err, size, ERR_FILE,
					"Error copying ID3 Frame from input audio file to output audio file\n");
			
	return(OK);
}

/*
 *	Function:	mp3SkipPadBytes
 *	Purpose:	Move the file pointer past any pad bytes in the input file
 *  Returns:    OK
 *
 */


int mp3SkipPadBytes (FILE *audioInFP)

{
	unsigned char c = 0;
	int seekStart, padcount;
	
	seekStart = ftello(audioInFP);
	
	while (!c)
	{
		fread(&c, 1, 1, audioInFP);
		padcount++;
	}
	padcount--;
	fseeko(audioInFP, -1, SEEK_CUR);

	/* dprintf("Zero-padding start: %ld, end: %ld, total: %ld, next non-zero byte %02x.\n", 
	 seekStart, seekStart + padcount, padcount, c); */

	return (OK);
}

/*
 *	Function:	mp3WritePadBytes
 *	Purpose:	Write pad bytes to the output file
 *  Returns:    OK or exit on error
 *
 */

int mp3WritePadBytes (FILE *audioOutFP, int numPadBytes)
{
	int i;
	unsigned char c = 0;
	
	vprintf("Writing %d pad bytes to file\n", numPadBytes);
	for (i=0; i<numPadBytes; i++) {
		err = fwrite(&c, 1, 1, audioOutFP);
		uitsHandleErrorINT(mp3ModuleName, "mp3WritePadBytes", err, 1, ERR_FILE, "Error writing pad bytes to MP3 output file\n");
	}
	
	return (OK);
	
}

/*
 *	Function: mp3WritePRIVFrame
 *	Purpose:  Create a PRIV frame that includes the UITS payload and write it to the output file 
 *			  Leaves input and output file pointers at end of copied bytes
 *  Returns:  OK or exit on error
 *
 */

int mp3WritePRIVFrame (FILE *audioOutFP, char *uitsPayloadXML) 
{
	unsigned char privFrameHeader[MP3_HEADER_SIZE];
	unsigned long privFrameLen;
	unsigned long privFrameMailtoLen;
	unsigned long privFrameuitsLen;
	unsigned long size;
	unsigned char *nullByte = "\0";
		
	/* create the PRIV frame that will be appended to the end if the ID3 frame */
	
	/* the PRIV frame data string: "mailto:uits-info@umusic.com <?xml..." */
	/* note that there are two NULL bytes in the priv frame data. One after the "mailto:.." string
	/* and one after the UITS xml string */
	
	privFrameMailtoLen = strlen(id3privFrameEmail);
	privFrameuitsLen   = strlen(uitsPayloadXML);
	
	privFrameLen = privFrameMailtoLen + 1 + privFrameuitsLen + 1;	
		
	/* the 10 byte PRIV frame header */
	privFrameHeader[0] = 'P';
	privFrameHeader[1] = 'R';
	privFrameHeader[2] = 'I';
	privFrameHeader[3] = 'V';
	
	size = privFrameLen;
	privFrameHeader[7] = size & 0x000000ff;
	size >>= 8;
	privFrameHeader[6] = size & 0x000000ff;
	size >>= 8;
	privFrameHeader[5] = size & 0x000000ff;
	size >>= 8;
	privFrameHeader[4] = size & 0x000000ff;
	
	privFrameHeader[8] = '\0';
	privFrameHeader[9] = '\0';
	
	/* write the frame header */
	err = fwrite(privFrameHeader, 1L, MP3_HEADER_SIZE, audioOutFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3WritePRIVFrame", err, MP3_HEADER_SIZE, ERR_FILE, "Error writing PRIV frame header\n");
	
	/* write the frame data mailto string */
	err = fwrite(id3privFrameEmail, 1L, strlen(id3privFrameEmail), audioOutFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3WritePRIVFrame", err, privFrameMailtoLen, ERR_FILE, "Error writing PRIV frame email\n");
	
	/* write a null terminator for the email */
	err = fwrite(nullByte, 1L,1L, audioOutFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3WritePRIVFrame", err, 1, ERR_FILE, "Error writing PRIV frame email NULL terminator\n");

	/* write the frame data UITS xml string */
	err = fwrite(uitsPayloadXML, 1L, strlen(uitsPayloadXML), audioOutFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3WritePRIVFrame", err, privFrameuitsLen, ERR_FILE, "Error writing PRIV frame UITS xml\n");
	
	/* write a null terminator for the email */
	err = fwrite(nullByte, 1L,1L, audioOutFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3WritePRIVFrame", err, 1, ERR_FILE, "Error writing PRIV frame UITS xml NULL terminator\n");
	
	
//	free(privFrameData);
	
	return (OK);
	
}

/*
 *	Function: mp3FindUITSPayload
 *	Purpose:  Read an ID3 frame and check to see if it is PRIV frame containing the UITS payload
 *			  Leaves input file pointers at end of frame
 *  Returns:  pointer to payload XML if found or NULL
 *
 */

char *mp3FindUITSPayload (FILE *audioInFP) 
{
	unsigned char	header[MP3_HEADER_SIZE];
	unsigned long	size;
	char	*privFrameData;
	char	*strPtr;
	char	*uitsPayloadXML;
	
	/* read the frame header */
	err = fread(header, 1L, MP3_HEADER_SIZE, audioInFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3FindUITSPayload", err, MP3_HEADER_SIZE, ERR_FILE, "Error reading ID3 frame header\n");
	
	/* calculate the frame size */

	size =  (unsigned long) header[7];
	size += (unsigned long) header[6] * 256L;
	size += (unsigned long) header[5] * 256L * 256L;
	size += (unsigned long) header[4] * 256L * 256L * 256L;
	
	/* read the frame data */
	privFrameData = calloc(size, 1);
	err = fread(privFrameData, 1L, size, audioInFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3FindUITSPayload", err, size, ERR_FILE, "Error reading PRIV frame data\n");
	
	/* Is this a PRIV frame? */
	if (header[0] == 'P' && header[1] == 'R' && header[2] == 'I' && header[3] == 'V') {

		/* PRIV frame starts with a null-terminated owner-identifier string. skip it */
		strPtr = privFrameData;
		while (*strPtr) {
			strPtr++;
		}

		strPtr++;

		/* is it a UITS payload? */
		/* look for the the uits:UITS open tag string in the priv frame data */
		if (strstr(strPtr, ":UITS")) {
			uitsPayloadXML = strstr(strPtr, "<?xml");
			return (uitsPayloadXML);
		}
		
	}
	
	/* didn't find the UITS payload. Cleanup and return NULL */
	free(privFrameData);
	
	return (NULL);
	
}


/*
 * Function: mp3IdentifyFrame
 * Purpose:  Determine what we're looking at. We know of ID3 (outer) tags, ID3 metadata frames within them,
 *				(in v2, v3, and v4 variations), MP3 audio frames, and zero-padding. Nothing else.
 *              We should enter this with the file positioned at the first byte of the (presumably unknown) header.
 *              The file pointer is left at it's original position
 *
 * Passed:   file pointer			
 * Returns:  frame type 
 *
 */

int mp3IdentifyFrame (FILE *fpin)

{
	unsigned long saveSeek;
	
	saveSeek = ftello(fpin);
		
	err = fread(header, 1L, 4L, fpin);
	uitsHandleErrorINT(mp3ModuleName, "mp3IdentifyFrame", err, 4L, ERR_FILE, "Couldn't read mp3 Frame header\n");
	
	// return FP to original position
	fseeko(fpin, saveSeek, SEEK_SET);

	if ((header[0] == 0xff) && ((header[1] & 0xe0) == 0xe0))	// Sync bytes - start of an MP3 audio frame, always eleven 1's.
	{
		return(AUDIOFRAME);
	}
	if (header[0] == 'I' && header[1] == 'D' && header[2] == '3')	// outer tag boundary, not audio
		return(ID3V2TAG);
	if (header[0] == 'T' && header[1] == 'A' && header[2] == 'G')	// outer tag boundary, not audio
		return(ID3V1TAG);
	if (header[0] == (char) 0)
		return(PADDING);
	return(ID3FRAME);	// no other choices, must be this
}

/*
 * Function: mp3ReadID3Header
 * Purpose:  Read an ID3 tag and populate a structure with the tag info (outer boundary). 
 *           The file pointer is left at it's original position
 * Passed:   File pointer (should point to start of tag)
 * Returns:  Pointer to header structure or NULL if error
 *
 */

MP3_ID3_HEADER *mp3ReadID3Header(FILE *fpin) 
{
	int saveSeek;
	unsigned long tagsize;
	MP3_ID3_HEADER *mp3Header = calloc(sizeof(MP3_ID3_HEADER), 1);

	saveSeek = ftello(fpin);
	err = fread(header, 1L, MP3_HEADER_SIZE, fpin);
	uitsHandleErrorINT(mp3ModuleName, "mp3ReadID3Header", err, MP3_HEADER_SIZE, ERR_FILE, "Couldn't read mp3 Frame header\n");

	// ID 3 tag header starts with 'I' 'D' '3'
	if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
		// major and minor version number are in next two bytes
		// eg. $03 $00 = ID3V2.3.0
		mp3Header->majorVersion = (int) header[3];
		mp3Header->minorVersion = (int) header[4];
		// vprintf("MP3 Header Tag format: ID3V2.%d.%d\n", mp3Header->majorVersion, mp3Header->minorVersion);
		
		// the next byte is the flags byte
		mp3Header->flags = header[5];
		
		// vprintf("MP3 Header Flags byte: %02x\n", mp3Header->flags);
		
//		if (header[5] & 0x10)
//		{
//			tag_has_footer = 1;
//			if (verbosity)
//				vprintf("ID3 tag footer is present.\n");
//		}
		
		// The next 4 bytes have the size in sync-safe format. Need to convert to integer.
		
		memcpy(&tagsize, &header[6], 4);
		lswap(&tagsize);
		uitsMake28From32(&tagsize); // we have to remove the high bit of each byte in size - moronic.
		mp3Header->size = tagsize;
		// dprintf("Greater Tag size: %ld\n", tagsize);
		fflush(stdout);
		
		// return file pointer to original location
		fseeko(fpin, saveSeek, SEEK_SET);
		
		return(mp3Header);
	}
	
	// Error reading ID3 tag header
	return (NULL);
}

/*
 * Function: mp3WriteID3Header
 * Purpose:  Write an ID3 header to an output file
 *           The file pointer is left at its original position 
 * Passed:   File pointer, header
 * Returns:  OK or exit if error
 *
 */

int	mp3WriteID3Header(FILE *audioOutFP, MP3_ID3_HEADER *mp3Header) 
{
	unsigned long saveSeek;
		
	saveSeek = ftello(audioOutFP);	/* so we can restore fp before return */

	// the first 3 bytes are 'I' 'D' '3' 
	header[0] = 'I';
	header[1] = 'D';
	header[2] = '3';
	
	// major and minor version are in next two bytes 
	header[3] = (unsigned char) mp3Header->majorVersion;
	header[4] = (unsigned char) mp3Header->minorVersion;
			
	// the next byte is the flags byte 
	header[5] = (unsigned char)	mp3Header->flags;
		
	// vprintf("MP3 Header Flags byte: %02x\n", mp3Header->flags);
		
	
	// The next 4 bytes have the size in sync-safe format. Need to convert from integer and handle endianness. 

	uitsMake32From28(&mp3Header->size); // we have to remove the high bit of each byte in size - moronic.
	lswap(&mp3Header->size);
	memcpy(&header[6] ,&mp3Header->size,  4);

	err = fwrite(header, 1, MP3_HEADER_SIZE, audioOutFP);
	uitsHandleErrorINT(mp3ModuleName, "mp3WriteID3Header", err, MP3_HEADER_SIZE, ERR_FILE, 
					"Couldn't write updated MP3 header to audio output file\n");
	
	// return file pointer to original location
	fseeko(audioOutFP, saveSeek, SEEK_SET);
		
	return(OK);
}


/*
 * Function: mp3ReadAudioFrameHeader
 * Purpose:  Read and parse the header for an audio frame, including calculating frame length
 *           The file pointer is left at its original position 
 * Passed:   File pointer
 * Returns:  Pointer to frame header or exit if error
 *
 */


MP3_AUDIO_FRAME_HEADER *mp3ReadAudioFrameHeader (FILE *fpin) 
{
	MP3_AUDIO_FRAME_HEADER *frameHeader = calloc(sizeof(MP3_AUDIO_FRAME_HEADER), 1);
	int saveSeek;		// always leave the file pointer where it was when the function was called
	unsigned char header[3];
	unsigned char bytebuf;

//	unsigned long next_frame, save_seek, aindex = 0L;
	
	saveSeek = ftello(fpin);
	err = fread(header, 1L, 4L, fpin);
	uitsHandleErrorINT(mp3ModuleName, "mp3ReadAudioFrame", err, 4L, ERR_FILE, "Couldn't read mp3 audio frame header\n");
	
	// Make sure this is an audio frame header
	// Sync bytes - start of an MP3 audio frame, always eleven 1's.
	if (!((header[0] == 0xff) && ((header[1] & 0xe0) == 0xe0))){
		uitsHandleErrorINT(mp3ModuleName, "mp3ReadAudioFrameHeader", ERROR, OK, ERR_MP3, "Couldn't read audio frame header\n");
	}

	bytebuf = (header[1] >> 3) & 0x03;
	if (bytebuf == 3)
		frameHeader->mpeg1Flag = 1;
	else
		frameHeader->mpeg1Flag = 0;
		
	bytebuf = (header[1] >> 1) & 0x03;
	frameHeader->layer3Flag = bytebuf;

	bytebuf = header[1] & 0x01;
	frameHeader->crcFlag = bytebuf;

	bytebuf = (header[2] >> 4) & 0x0f;
	frameHeader->bitrate = bitrates[(int) bytebuf];
	
	bytebuf = (header[2] >> 2) & 0x03;
	frameHeader->samplerate = samplerates[(int) bytebuf];
		
	bytebuf = header[2] & 0x02;
	frameHeader->paddedFlag = bytebuf;

	bytebuf = header[2] & 0x01;
	frameHeader->privateFlag = bytebuf;
		
	bytebuf = (header[3] >> 6) & 0x03;
	frameHeader->chanmode = channel_modes[(int) bytebuf];
		
	if ((int) bytebuf < 3)	// not a mono file
		frameHeader->stereoFlag = 1;
	else 
		frameHeader->stereoFlag = 0;

	bytebuf = (header[3] >> 4) & 0x03;
	frameHeader->modeExtension = mode_extensions[(int) bytebuf];
		
	bytebuf = (header[3] >> 3) & 0x01;
	frameHeader->copyrightFlag = bytebuf;
		
	bytebuf = (header[3] >> 2) & 0x01;
	frameHeader->origFlag = bytebuf;
		
	bytebuf = (header[3]& 0x03);
	frameHeader->emphasis = emphasis[(int) bytebuf];
		
	frameHeader->frameLength = ((144L * frameHeader->bitrate) / frameHeader->samplerate) + frameHeader->paddedFlag;
		
//	next_frame = (ftello(fpin)) + frame_length - 4L;
	
	frameHeader->vbrHeaderflag = mp3IsVBRFrame (fpin, frameHeader);
		
	// return file pointer to original location
	fseeko(fpin, saveSeek, SEEK_SET);
	return (frameHeader);

}


/*
 * Function: mp3IsVBRFrame
 * Purpose:  Read the next 3 bytes of data from the audio frame to determine if this audio frame is
 *           a XING, Info, or VBR frame in which case it should be skipped during media hash calculation. 
 *           This will only occur for the first frame of auido.
 *           The file pointer is left at its original position 
 * Passed:   File pointer
 * Returns:  TRUE or FALSE
 *
 */

int mp3IsVBRFrame (FILE *fpin, MP3_AUDIO_FRAME_HEADER *frameHeader)
{
	int saveSeek;	// always leave the file pointer where it was when the function was called
	char *audiobuf;
	int aindex;
//	int xingHeader = 0L;
	int returnValue = FALSE;

	saveSeek = ftello(fpin);
	audiobuf = calloc(frameHeader->frameLength, 1);
	
	fread(audiobuf, 1L, frameHeader->frameLength, fpin);

	if (frameHeader->stereoFlag)
		aindex = 32L;
	else
		aindex = 17L;
	
	if (audiobuf[aindex] == 'X' && audiobuf[aindex + 1] == 'i' && audiobuf[aindex + 2] == 'n' && audiobuf[aindex + 3] == 'g') {
		// vprintf("MP3: Xing header found!\n");
		returnValue = TRUE;
	} else if (audiobuf[aindex] == 'I' && audiobuf[aindex + 1] == 'n' && audiobuf[aindex + 2] == 'f' && audiobuf[aindex + 3] == 'o') {
		// vprintf("MP3: Info header found!\n");
		returnValue = TRUE;
	} else if (audiobuf[aindex] == 'V' && audiobuf[aindex + 1] == 'B' && audiobuf[aindex + 2] == 'R' && audiobuf[aindex + 3] == 'I') {
		// vprintf("MP3: VBRI header found!\n");
		returnValue = TRUE;
	}
	
	/* cleanup */
	free(audiobuf);
	fseeko(fpin, saveSeek, SEEK_SET);	// back up to where we were
	
	return (returnValue);
}


/*
 * Function: mp3HasID3v1Tag
 * Purpose:  Read the last 128 bytes of the file and determine whether or not it's an MP3 ID3v1 tag
  *           The file pointer is left at its original position after writing
 * Passed:   File pointer
 * Returns:  TRUE or FALSE
 *
 */


int mp3HasID3v1Tag (FILE *fpin)
{
	int saveSeek;	// always leave the file pointer where it was when the function was called
	char *audiobuf;
	int returnValue = FALSE;
	
	saveSeek = ftello(fpin);	// save the starting position
	
	fseeko(fpin, -id3v1TagSize, SEEK_END);	// seek back 128 bytes from end of file
	
	audiobuf = calloc(id3v1TagSize, 1);
	
	fread(audiobuf, 1L, id3v1TagSize, fpin);
	
	if (audiobuf[0] == 'T' && audiobuf[1] == 'A' && audiobuf[2] == 'G') {
		// vprintf("MP3: ID3 v1 tag found at end of file\n");
		returnValue = TRUE;
	}
	
	/* cleanup */
	free(audiobuf);
	fseeko(fpin, saveSeek, SEEK_SET);	// back up to where we were
	
	return (returnValue);
}
	

/*
 *  Convert any ID3V2.x header unpacked size to a usable (packed) 28-bit form.
 *
 */

void uitsMake28From32 (long *length)
{
	long l1, l2, l3, l4;
	
	l1 = *length & 0x7f000000;
	l1 >>= 3L;
	l2 = *length & 0x7f0000;
	l2 >>= 2L;	
	l3 = *length & 0x7f00;
	l3 >>= 1L;
	l4 = *length & 0x7f;
	l4 += l1 + l2 + l3;
	*length = l4;
	return;
}

/*
 *  Convert a 28-bit (packed) integer to 32-bit (unpacked) syncsafe form.
 *
 */

void uitsMake32From28 (long *length)
{
	long l1, l2, l3, l4;
	
	l4 = *length & 0x7f;
	l3 = *length & 0x3f80;
	l3 <<= 1L;
	l2 = *length & 0x1fc000;
	l2 <<= 2L;
	l1 = *length & 0x0fe00000;
	l1 <<= 3L;
	l4 += l1 + l2 + l3;
	*length = l4;	
	return;
}
// EOF


