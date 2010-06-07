/*
 *  uitsWAVManager.c
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 5/17/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 */

#include "uits.h"



char *wavModuleName = "uitsWAVManager.c";


/*
 *
 * Function: wavIsValidFile
 * Purpose:	 Determine if an audio file is a WAV file
 *			
 *
 * Returns:   TRUE if WAV, FALSE otherwise
 */

int wavIsValidFile (char *audioFileName) 
{
	FILE *audioFP;
	WAV_CHUNK_HEADER *wavChunkHeader;
	char *formType = calloc(sizeof(char), 4);
	int  isWAV = FALSE;
	
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(wavModuleName, "wavIsValidFile", audioFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	/* Read the first 8 bytes of the file and check to see if they represent an AIFF "FORM" chunk */
	wavChunkHeader = wavReadChunkHeader(audioFP);
	
 	if (strncmp(wavChunkHeader->chunkID, "RIFF", 4) == 0) {
		/* seek past header and read form type */
		fseeko(audioFP, WAV_HEADER_SIZE, SEEK_CUR);
		
		/* form type must be WAVE */
		err = fread(formType, 1, 4, audioFP);
		uitsHandleErrorINT(wavModuleName, "wavIsValidFile", err, 4, ERR_WAV, "Couldn't read WAV FORM chunk type\n");
		
		if (strncmp(formType, "WAVE", 4) == 0){
			vprintf("Audio file is WAV\n");
			SetBigEndianFlag(FALSE);	/* wav files are always little-endian */
			isWAV = TRUE;
		}
	} 
	fclose (audioFP);	
	
	return (isWAV);
}

/*
 *
 * Function: wavGetMediaHash
 * Purpose:	 Calcluate the media hash for a WAV file
 *			 The AIFF Media Hash is created using the following algorithm:
 *              1. Find the data chunk
 *              2. Hash the audio frames in the data chunk, not including pad byte if it exists
 *
 * Returns:   Pointer to the hashed frame data
 */

char *wavGetMediaHash (char *audioFileName) 
{
	FILE				*audioFP;
	UITS_digest			*mediaHash = NULL;
	char				*mediaHashString = NULL;
	
	unsigned long	fileLength;
	unsigned long	audioFrameStart, audioFrameEnd;
	
	WAV_CHUNK_HEADER *dataChunk;
	
	
//	intelCPUFlag = 0;	/* all WAV files are always little-endian */

	
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(wavModuleName, "wavGetMediaHash", audioFP, ERR_FILE, "Couldn't open WAV audio file for reading\n");
	
	/* seek past the RIFF chunk */
	/* RIFF chunk is 12 bytes: */
	/*  chunk ID    (4-bytes)
	 *  chunk size  (4-bytes)
	 *  wave ID     (4-bytes)
	 */
	
	fseeko(audioFP, 12, SEEK_CUR);
	
	/* now skip chunks until 'data' chunk */
	/* get file size */
	fileLength = uitsGetFileSize(audioFP);
	
	dataChunk = wavFindChunkHeader(audioFP, "data", fileLength);
	uitsHandleErrorPTR(wavModuleName, "wavGetMediaHash", dataChunk, ERR_WAV, 
					   "Couldn't find 'data' chunk in audio file\n");
	
	/* move fp to start of data */
	fseeko(audioFP, dataChunk->saveSeek, SEEK_SET);
	
	/*skip past ID and size */
	fseeko(audioFP, 8L, SEEK_CUR);
	
	/* fp is (hopefully) at start of audio frame data */
	audioFrameStart = ftello(audioFP);
	
	mediaHash = uitsCreateDigestBuffered (audioFP, dataChunk->chunkSize, "SHA256") ;
	mediaHashString = uitsDigestToString(mediaHash);
	
	fclose(audioFP);
	
	return (mediaHashString);
	
}

/*
 *
 * Function: wavEmbedPayload
 * Purpose:	 Embed the UITS payload into an WAV file
 *			 The following algorithm is used to embed the payload:
 *				1. Copy the input file to the output file
 *              2. Append an 'UITS' chunk with the payload
 *				3. Update the size of the RIFF chunk to reflect the additional 'UITS' chunk
 *
 *
 * Returns:   OK or ERROR
 */

int wavEmbedPayload  (char *audioFileName, 
					   char *audioFileNameOut, 
					   char *uitsPayloadXML,
					   int  numPadBytes) 
{
	FILE			*audioInFP, *audioOutFP;
	WAV_CHUNK_HEADER *riffChunk = NULL;
	WAV_CHUNK_HEADER *uitsChunk = NULL;
	unsigned long	audioInFileSize;
	
	unsigned long	udtaChunkDataSize;
	unsigned long	payloadXMLSize;
	
//	intelCPUFlag = 0;	/* all WAV files are always little-endian */

	vprintf("About to embed payload for %s into %s\n", audioFileName, audioFileNameOut);
	payloadXMLSize = strlen(uitsPayloadXML);
	
	/* open the audio input and output files */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(wavModuleName, "wavEmbedPayload", audioInFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	audioOutFP = fopen(audioFileNameOut, "wb");
	uitsHandleErrorPTR(wavModuleName, "wavEmbedPayload", audioOutFP, ERR_FILE, "Couldn't open audio file for writing\n");
	
	/* calculate how long the input audio file is by seeking to EOF and saving size  */
	audioInFileSize = uitsGetFileSize(audioInFP);
	
	
	/* make sure there isn't an existing UITS payload */
	fseeko(audioInFP, WAV_HEADER_SIZE + 4, SEEK_CUR);	/* seek past RIFF header and WAVE type */
	uitsChunk = wavFindChunkHeader (audioInFP, "UITS", audioInFileSize);
	if (uitsChunk) {
		uitsHandleErrorPTR(wavModuleName, "wavEmbedPayload", NULL, ERR_WAV, 
						   "Audio file already contains a UITS payload\n");
	}
	
	/* no existing payload, rewind and create new file */
	rewind(audioInFP);
	
	/* copy the entire file */
	uitsAudioBufferedCopy(audioInFP, audioOutFP, audioInFileSize);
	
	/* update the RIFF chunk to include the size of the new UITS chunk */
	
	rewind(audioOutFP);
	rewind(audioInFP);
	riffChunk = wavReadChunkHeader(audioInFP);
	riffChunk->chunkSize += WAV_HEADER_SIZE + payloadXMLSize ;	/* UITS chunk size includes header + data */
	fseeko(audioOutFP, 4, SEEK_CUR);
	lswap(&riffChunk->chunkSize);
	fwrite(&riffChunk->chunkSize, 1, 4, audioOutFP);
	
	
	/* add an UITS chunk to the end of the output file*/	
	fseeko(audioOutFP, 0, SEEK_END);
	fwrite("UITS", 1, 4, audioOutFP);						/* 4-bytes ID */
	lswap(&payloadXMLSize);								/* handle big vs little endianness */
	fwrite(&payloadXMLSize, 1, 4, audioOutFP);			/* 4-bytes size */
	fwrite(uitsPayloadXML, 1, payloadXMLSize, audioOutFP);	/* UITS payload */
	
	/* the pad byte if necessary */
	if (payloadXMLSize & 1) {
		fwrite("\0", 1, 1, audioOutFP);
	}
	
	return(OK);
}

/*
 *
 * Function: wavExtractPayload
 * Purpose:	 Extract the UITS payload from an WAV file
 *
 * Returns: pointer to payload, NULL if payload not found or exit if error
 */

char *wavExtractPayload (char *audioFileName) 
{
	
	WAV_CHUNK_HEADER *uitsChunkHeader = NULL;
	FILE			*audioInFP;
	unsigned long	audioInFileSize;
	char			*payloadXML;
	int				payloadXMLSize;
	
//	intelCPUFlag = 0;	/* all WAV files are always little-endian */
	/* open the audio input file */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(wavModuleName, "wavExtractPayload", audioInFP, ERR_FILE, "Couldn't open audio file for reading\n");
	
	audioInFileSize = uitsGetFileSize(audioInFP);
	
	/* seek past start of 'RIFF' chunk header and type (4 bytes)*/
	
	fseeko(audioInFP, WAV_HEADER_SIZE + 4, SEEK_CUR);
	
	/* find a 'UITS' chunk */	
	uitsChunkHeader = wavFindChunkHeader (audioInFP, "UITS", audioInFileSize);
	uitsHandleErrorPTR(wavModuleName, "wavExtractPayload", uitsChunkHeader, ERR_WAV, 
					   "Coudln't find UITS payload in WAV file\n");
	
	fseeko(audioInFP, uitsChunkHeader->saveSeek, SEEK_SET);	/* seek to start of chunk */
	fseeko(audioInFP, WAV_HEADER_SIZE, SEEK_CUR);				/* seek past ID and Size in header */
	
	/* alloc space with an extra null-terminator byte */
	payloadXML = calloc((uitsChunkHeader->chunkSize + 1), 1);	
	payloadXMLSize = uitsChunkHeader->chunkSize;
	
	err = fread(payloadXML, 1L, payloadXMLSize, audioInFP);
	uitsHandleErrorINT(wavModuleName, "wavExtractPayload", err, ERR_FILE, payloadXMLSize, 
					   "Couldn't read UITS payload\n");
	
	return (payloadXML);
}

/*
 * Function: wavReadChunkHeader
 * Purpose:  Read the 4-byte WAV chunk ID and  4-byte WAV chunk size
 * Passed:   File pointer (should point to start of tag)
 * Returns:  Pointer to header structure or NULL if error
 *
 */

WAV_CHUNK_HEADER *wavReadChunkHeader (FILE *fpin)
{
	WAV_CHUNK_HEADER *chunkHeader = calloc(sizeof(WAV_CHUNK_HEADER), 1);
	unsigned char	  header[WAV_HEADER_SIZE];
	unsigned long     chunkSize;
	
	
	chunkHeader->saveSeek = ftello(fpin);
	err = fread(header, 1L, WAV_HEADER_SIZE, fpin);
	uitsHandleErrorINT(wavModuleName, "wavExtractPayload", err, WAV_HEADER_SIZE, ERR_FILE, 
					   "Couldn't read wav chunk header\n");
	
	
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
 * Function: wavFindChunkHeader
 * Purpose:	 Find an WAV chunk header within a WAV file
 *           Chunk header format is :
 *               id:   4-bytes
 *               size: 4-bytes (if size is odd, data is padded to even size)
 *
 *           The file pointer is returned to it's original position
 * Passed:   File pointer (should point to file location to start search)
 *			 ID of chunk to find
 *           Chunk Type (NULL if don't care)
 *			 Location to end searching
 * Returns:  Pointer to header structure or NULL if error
 */


WAV_CHUNK_HEADER *wavFindChunkHeader (FILE *fpin, char *chunkID, unsigned long endSeek)
{
	WAV_CHUNK_HEADER *chunkHeader = calloc(sizeof(WAV_CHUNK_HEADER), 1);
	unsigned long	saveSeek, bytesLeft;
	unsigned long sizeWithPad;
	int	chunkTypeLen = 0;
	char *currChunkType = NULL;
	int chunkFound = FALSE;
	
	saveSeek = ftello(fpin);
	
	bytesLeft = endSeek - saveSeek;
	
	
	/* read chunks until finding the chunk ID (and possibly type) or end of search */
	while (!chunkFound && bytesLeft) {
		chunkHeader = aiffReadChunkHeader(fpin);
		/* seek past header */
		fseeko(fpin, WAV_HEADER_SIZE, SEEK_CUR);
		bytesLeft -= WAV_HEADER_SIZE;
		
		if (strncmp(chunkHeader->chunkID, chunkID, 4) == 0) {
				chunkFound = TRUE;
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
