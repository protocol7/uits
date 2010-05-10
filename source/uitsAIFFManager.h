/*
 *  uitsAIFFManager.h
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 4/19/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */


/*
 * Prevent multiple inclusion...
 */

#ifndef _uitsaiffmanager_h_
#  define _uitsaiffmanager_h_


#define AIFF_HEADER_SIZE 8

/* 
 * Structures
 */

typedef struct {
	unsigned char   chunkID[4];
	unsigned long	chunkSize;		
	unsigned long   saveSeek;	/* saved seek location for the header start */
} AIFF_CHUNK_HEADER;

/* 
 * PUBLIC Functions 
 */

int aiffIsValidFile		(char *audioFileName);

int aiffEmbedPayload	(char *audioFileName, 
						 char *audioFileNameOut, 
						 char *uitsPayloadXML,
						 int  numPadBytes);

char *aiffExtractPayload (char *audioFileName); 

char *aiffGetMediaHash	(char *audioFileName);


/*
 * PRIVATE Functions
 */

AIFF_CHUNK_HEADER *aiffReadChunkHeader (FILE *audioInFP);
char			  *aiffReadChunkData   (FILE *audioInFP);
AIFF_CHUNK_HEADER *aiffFindChunkHeader (FILE *fpin, char *chunkID, char *chunkType, unsigned long endSeek);


#endif