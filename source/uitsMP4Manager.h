/*
 *  uitsMP4Manager.h
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 4/2/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _uitsmp4manager_h_
#  define _uitsmp4manager_h_


/*
 * IO Buffer for reading/writing MP4 files
 */

#define MP4_HEADER_SIZE 8

typedef struct {
	unsigned long size;		
	unsigned char type[5];			/* 4-character type, null-terminated */
	unsigned long saveSeek;	/* saved seek location for the header start */
} MP4_ATOM_HEADER;


/* 
 * PUBLIC Functions 
 */

int mp4IsValidFile			(char *audioFileName); 

int mp4EmbedPayload		    (char *audioFileName, 
							 char *audioFileNameOut, 
							 char *uitsPayloadXML,
							 int  numPadBytes);

char *mp4ExtractPayload		(char *audioFileName); 

char *mp4GetMediaHash		(char *audioFileName);


/*
 * PRIVATE Functions
 */

MP4_ATOM_HEADER *mp4FindAtomHeader (FILE *fpin,  char *atomType, unsigned long endSeek);
MP4_ATOM_HEADER *mp4ReadAtomHeader  (FILE *fpin);
int   mp4CopyAtom			(FILE *fpin, FILE *fpout);

#endif


