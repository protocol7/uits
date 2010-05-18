/*
 *  uitsWAVManager.h
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 5/17/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 * 
 *  $Date$
 *  $Revision$
 */


/*
 * Prevent multiple inclusion...
 */

#ifndef _uitswavmanager_h_
#  define _uitswavmanager_h_


#define WAV_HEADER_SIZE 8

/* 
 * Structures
 */

typedef struct {
	unsigned char   chunkID[4];
	unsigned long	chunkSize;		
	unsigned long   saveSeek;	/* saved seek location for the header start */
} WAV_CHUNK_HEADER;

/* 
 * PUBLIC Functions 
 */

int wavIsValidFile		(char *audioFileName);

int wavEmbedPayload	(char *audioFileName, 
						 char *audioFileNameOut, 
						 char *uitsPayloadXML,
						 int  numPadBytes);

char *wavExtractPayload (char *audioFileName); 

char *wavGetMediaHash	(char *audioFileName);


/*
 * PRIVATE Functions
 */

WAV_CHUNK_HEADER *wavReadChunkHeader (FILE *audioInFP);
char			 *wavReadChunkData   (FILE *audioInFP);
WAV_CHUNK_HEADER *wavFindChunkHeader (FILE *fpin, char *chunkID, unsigned long endSeek);


#endif
