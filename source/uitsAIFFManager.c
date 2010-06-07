/*
 *  uitsAIFFManager.c
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 5/4/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 */

#include "uits.h"


char *aiffModuleName = "uitsAIFFManager.c";


/*
 *
 * Function: aiffIsValidFile
 * Purpose:	 Determine if an audio file is an AIFF file
 *			
 *
 * Returns:   TRUE if AIFF, FALSE otherwise
 */

int aiffIsValidFile (char *audioFileName) 
{
	FILE *audioFP;
	AIFF_CHUNK_HEADER *aiffChunkHeader;
	char *formType = calloc(sizeof(char), 4);
	int  isAIFF = FALSE;
	
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(aiffModuleName, "aiffIsValidFile", audioFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	/* Read the first 8 bytes of the file and check to see if they represent an AIFF "FORM" chunk */
	aiffChunkHeader = aiffReadChunkHeader(audioFP);
	
 	if (strncmp(aiffChunkHeader->chunkID, "FORM", 4) == 0) {
		/* seek past header and read form type */
		fseeko(audioFP, AIFF_HEADER_SIZE, SEEK_CUR);
		
		/* FORM type must be AIFF or AIFC */
		err = fread(formType, 1, 4, audioFP);
		uitsHandleErrorINT(aiffModuleName, "aiffIsValidFile", err, 4, ERR_FILE, "Couldn't read AIFF FORM chunk type\n");
		
		if ((strncmp(formType, "AIFF", 4) == 0) ||
			(strncmp(formType, "AIFC", 4) == 0)) {
				vprintf("Audio file is AIFF\n");
				isAIFF = TRUE;
		}
	} 
	fclose (audioFP);	

	return (isAIFF);
}

/*
 *
 * Function: aiffGetMediaHash
 * Purpose:	 Calcluate the media hash for an AIFF file
 *			 The AIFF Media Hash is created using the following algorithm:
 *              1. Find the SSND chunk
 *              2. Hash the data in the SSND chunk
 *
 * Returns:   Pointer to the hashed frame data
 */

char *aiffGetMediaHash (char *audioFileName) 
{
	FILE				*audioFP;
	UITS_digest			*mediaHash = NULL;
	char				*mediaHashString = NULL;
	
	unsigned long	fileLength;
	unsigned long	audioFrameStart, audioFrameEnd;

	AIFF_CHUNK_HEADER *ssndChunk;
	
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(aiffModuleName, "aiffGetMediaHash", audioFP, ERR_FILE, "Couldn't open AIFF audio file for reading\n");
	
	/* seek past the FORM chunk */
	/* FORM chunk is 12 bytes: */
	/*  chunk ID    (4-bytes)
	 *  chunk size  (4-bytes)
	 *  form type   (4-bytes)
	 */
	
	fseeko(audioFP, 12, SEEK_CUR);
	
	/* now skip chunks until 'SSND' chunk */
	/* get file size */
	fileLength = uitsGetFileSize(audioFP);

	ssndChunk = aiffFindChunkHeader(audioFP, "SSND", NULL, fileLength);
	uitsHandleErrorPTR(aiffModuleName, "aiffGetMediaHash", ssndChunk, ERR_AIFF, "Couldn't find 'SSND' chunk in audio file\n");
	
	/* move fp to start of SSND */
	fseeko(audioFP, ssndChunk->saveSeek, SEEK_SET);
	
	/*skip past ID and size */
	fseeko(audioFP, 8L, SEEK_CUR);
	
	/* fp is (hopefully) at start of audio frame data */
	audioFrameStart = ftello(audioFP);
		
	mediaHash = uitsCreateDigestBuffered (audioFP, ssndChunk->chunkSize, "SHA256") ;
	mediaHashString = uitsDigestToString(mediaHash);
	
	fclose(audioFP);
	
	return (mediaHashString);
	
}

/*
 *
 * Function: aiffEmbedPayload
 * Purpose:	 Embed the UITS payload into an AIFF file
 *			 The following algorithm is used to embed the payload:
 *				1. Copy the input file to the output file
 *              2. Append an 'APPL' chunk with an "OSType" string of 'UITS' and the payload
 *				3. Update the size of the FORM chunk to reflect the additional 'APPL' chunk
 *
 *
 * Returns:   OK or ERROR
 */

int aiffEmbedPayload  (char *audioFileName, 
					   char *audioFileNameOut, 
					   char *uitsPayloadXML,
					   int  numPadBytes) 
{
	FILE			*audioInFP, *audioOutFP;
	AIFF_CHUNK_HEADER *formChunk = NULL;
	AIFF_CHUNK_HEADER *applChunk = NULL;
	unsigned long	audioInFileSize;
	
	unsigned long	udtaChunkDataSize;
	unsigned long	payloadXMLSize;
	
	vprintf("About to embed payload for %s into %s\n", audioFileName, audioFileNameOut);
	payloadXMLSize = strlen(uitsPayloadXML);
	
	/* open the audio input and output files */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(aiffModuleName, "aiffEmbedPayload", audioInFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	audioOutFP = fopen(audioFileNameOut, "wb");
	uitsHandleErrorPTR(aiffModuleName, "aiffEmbedPayload", audioOutFP, ERR_FILE, "Couldn't open audio file for writing\n");
	
	/* calculate how long the input audio file is by seeking to EOF and saving size  */
	audioInFileSize = uitsGetFileSize(audioInFP);

	
	/* make sure there isn't an existing UITS payload */
	fseeko(audioInFP, AIFF_HEADER_SIZE + 4, SEEK_CUR);	/* seek past FORM header and type */
	applChunk = aiffFindChunkHeader (audioInFP, "APPL", "UITS", audioInFileSize);
	if (applChunk) {
		uitsHandleErrorPTR(aiffModuleName, "aiffEmbedPayload", NULL, ERR_AIFF, "Audio file already contains a UITS payload\n");
	}
	
	/* no existing payload, rewind and create new file */
	rewind(audioInFP);

	/* copy the entire file */
	uitsAudioBufferedCopy(audioInFP, audioOutFP, audioInFileSize);

	/* update the FORM chunk to include the size of the new APPL chunk */
	udtaChunkDataSize = 4 + payloadXMLSize;	/* chunk data size is payload size + 4 bytes of OSType */
	
	rewind(audioOutFP);
	rewind(audioInFP);
	formChunk = aiffReadChunkHeader(audioInFP);
	formChunk->chunkSize += AIFF_HEADER_SIZE + udtaChunkDataSize ;	/* APPL chunk size includes header + data */
	fseeko(audioOutFP, 4, SEEK_CUR);
	lswap(&formChunk->chunkSize);
	fwrite(&formChunk->chunkSize, 1, 4, audioOutFP);
	
	
	/* add an APPL chunk to the end of the output file*/	
	fseeko(audioOutFP, 0, SEEK_END);
	fwrite("APPL", 1, 4, audioOutFP);						/* 4-bytes ID */
	lswap(&udtaChunkDataSize);								/* handle big vs little endianness */
	fwrite(&udtaChunkDataSize, 1, 4, audioOutFP);			/* 4-bytes size */
	fwrite("UITS", 1, 4, audioOutFP);						/* 4-bytes OSType */
	fwrite(uitsPayloadXML, 1, payloadXMLSize, audioOutFP);	/* UITS payload */
	
	/* the pad byte if necessary */
	if (payloadXMLSize & 1) {
		fwrite("\0", 1, 1, audioOutFP);
	}
	
	return(OK);
}

/*
 *
 * Function: aiffExtractPayload
 * Purpose:	 Extract the UITS payload from an AIFF file
 *
 * Returns: pointer to payload, NULL if payload not found or exit if error
 */

char *aiffExtractPayload (char *audioFileName) 
{
	
	AIFF_CHUNK_HEADER *applChunkHeader = NULL;
	FILE			*audioInFP;
	unsigned long	audioInFileSize;
	char			*payloadXML;
	int				payloadXMLSize;
	
	/* open the audio input file */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(aiffModuleName, "aiffExtractPayload", audioInFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	audioInFileSize = uitsGetFileSize(audioInFP);
	
	/* seek to start of 'FORM' chunk header and type (4 bytes)*/
	
	fseeko(audioInFP, AIFF_HEADER_SIZE + 4, SEEK_CUR);
	
	/* find an 'APPL' chunk with an OSType of "UITS" */	
	applChunkHeader = aiffFindChunkHeader (audioInFP, "APPL", "UITS", audioInFileSize);
	uitsHandleErrorPTR(aiffModuleName, "aiffExtractPayload", applChunkHeader, ERR_AIFF, "Coudln't find UITS payload in AIFF file\n");
	
	fseeko(audioInFP, applChunkHeader->saveSeek, SEEK_SET);	/* seek to start of chunk */
	fseeko(audioInFP, AIFF_HEADER_SIZE, SEEK_CUR);				/* seek past ID and Size in header */
	fseeko(audioInFP, 4, SEEK_CUR);								/* seek past "UITS" OSType */
		
	/* will be null-terminated when it's read from the file because size includes 4 bytes of OSType */
	payloadXML = calloc(applChunkHeader->chunkSize, 1);	
	payloadXMLSize = applChunkHeader->chunkSize - 4;
	
	err = fread(payloadXML, 1L, payloadXMLSize, audioInFP);
	uitsHandleErrorINT(aiffModuleName, "aiffExtractPayload", err, payloadXMLSize, ERR_AIFF, "Couldn't read UITS payload\n");
	
	return (payloadXML);
}

/*
 * Function: aiffReadChunkHeader
 * Purpose:  Read the 4-byte AIFF chunk ID and  4-byte AIFF chunk size
 * Passed:   File pointer (should point to start of tag)
 * Returns:  Pointer to header structure or NULL if error
 *
 */

AIFF_CHUNK_HEADER *aiffReadChunkHeader (FILE *fpin)
{
	AIFF_CHUNK_HEADER *chunkHeader = calloc(sizeof(AIFF_CHUNK_HEADER), 1);
	unsigned char	  header[AIFF_HEADER_SIZE];
	unsigned long     chunkSize;
	
	
	chunkHeader->saveSeek = ftello(fpin);
	err = fread(header, 1L, AIFF_HEADER_SIZE, fpin);
	uitsHandleErrorINT(aiffModuleName, "aiffReadChunkHeader", err, AIFF_HEADER_SIZE, ERR_FILE,  "Couldn't read aiff chunk header\n");
	
	
	//	vprintf("%c%c%c%c\n", header[4], header[5], header[6], header[7]);
	
	memcpy (chunkHeader->chunkID, &header[0], 4); 
	
	memcpy(&chunkSize, &header[4], 4);
	lswap(&chunkSize);
	chunkHeader->chunkSize = chunkSize;	/* size does NOT include 8 header bytes */
	
 	
	/* return file pointer to original position */
	
	fseeko(fpin, chunkHeader->saveSeek, SEEK_SET);
	
    return (chunkHeader);
}

/*
 *
 * Function: aiffFindChunkHeader
 * Purpose:	 Find an AIFF chunk header within an AIFF file
 *           Chunk header format is :
 *               id:   4-bytes
 *               size: 4-bytes (if size is odd, data is padded to even size)
 *
 *				 some chunks have a 4-byte type as their first bit of data (FORM, APPL) 
 *				if a chunkType is passed to this function, only return a chunk header of the requested type 
 *           The file pointer is returned to it's original position
 * Passed:   File pointer (should point to file location to start search)
 *			 ID of chunk to find
 *           Chunk Type (NULL if don't care)
 *			 Location to end searching
 * Returns:  Pointer to header structure or NULL if error
 */


AIFF_CHUNK_HEADER *aiffFindChunkHeader (FILE *fpin, char *chunkID, char *chunkType, unsigned long endSeek)
{
	AIFF_CHUNK_HEADER *chunkHeader = calloc(sizeof(AIFF_CHUNK_HEADER), 1);
	unsigned long	saveSeek, bytesLeft;
	unsigned long sizeWithPad;
	int	chunkTypeLen = 0;
	char *currChunkType = NULL;
	int chunkFound = FALSE;
	
	saveSeek = ftello(fpin);
	
	bytesLeft = endSeek - saveSeek;
	
	if (chunkType) {
		chunkTypeLen = strlen(chunkType);
		currChunkType = calloc(chunkTypeLen + 1, 1);  /* calloc space for type and null terminator */

	}
	
	/* read chunks until finding the chunk ID (and possibly type) or end of search */
	while (!chunkFound && bytesLeft) {
		chunkHeader = aiffReadChunkHeader(fpin);
		/* seek past header */
		fseeko(fpin, AIFF_HEADER_SIZE, SEEK_CUR);
		bytesLeft -= AIFF_HEADER_SIZE;

		if (strncmp(chunkHeader->chunkID, chunkID, 4) == 0) {
			/* if the chunk header needs to be of a particular type, check this chunk's type */
			if (chunkType) {
				fread(currChunkType, chunkTypeLen, 1, fpin);
				if (strncmp(currChunkType, chunkType, chunkTypeLen) == 0) {
					chunkFound = TRUE;
				}
				fseeko(fpin, -chunkTypeLen, SEEK_CUR);
			} else {
				chunkFound = TRUE;
			}
		}		
		/* seek past data and pad byte (if necessary) */
		sizeWithPad = chunkHeader->chunkSize;
		sizeWithPad += (chunkHeader->chunkSize & 0x01);
		fseeko(fpin, sizeWithPad, SEEK_CUR);
		bytesLeft -= sizeWithPad;
	}
	
	fseeko(fpin, saveSeek, SEEK_SET);
    return (chunkFound ? chunkHeader : NULL);
	
}

