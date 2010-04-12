/*
 *  uitsMP3Manager.h
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 12/8/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _uitsmp3manager_h_
#  define _uitsmp3manager_h_

#include	<math.h>
#include	<wchar.h>
#include	<unistd.h>
#include	<iconv.h>


/*
 * Defines
 */


/*
 * Frame types
 */

#define ID3V2TAG		1L
#define AUDIOFRAME		2L
#define ID3FRAME		3L
#define XINGFRAME		4L
#define PADDING			5L
#define ID3V1TAG		6L

/*
 * IO Buffer for reading/writing MP3 files
 */

#define MP3_HEADER_SIZE 10

/* 
 * Structures
 */

typedef struct {
	int				majorVersion;
	int				minorVersion;
	char			flags;
	unsigned long	size;
} MP3_ID3_HEADER;

typedef struct {
	int mpeg1Flag;		// 1= "MPEG1"	0= "NOMP1"
	int layer3Flag;		// 1= "LAYER3"	0= "NOTLY3"
	int crcFlag;		// 1= "CRC"		0= "NOCRC"
	int bitrate;
	int samplerate;
	int paddedFlag;		// 1= "PADDED"	0= "NOPAD"
	int privateFlag;	// 1= "PRIVATE" 0= "NO PRIV"
	char *chanmode;	
	int stereoFlag;		// 1= "STEREO" 0= "MONO"
	char *modeExtension;
	int copyrightFlag;	// 1= "1"		0= "0"
	int	origFlag;		// 1= "orig"	0= "copy"
	char *emphasis;
	int frameLength;
	int vbrHeaderflag;	// 1= 'xing', 'Info', or 'VBR' frame		
} MP3_AUDIO_FRAME_HEADER;

/* 
typedef struct {
	unsigned char header[10];
	unsigned int  size;
	unsigned char *data;
} MP3_PRIV_TAG;
 */

/*
 *  Function Declarations
 */ 

/* 
 * PUBLIC Functions 
 */

int mp3IsValidFile			(char *audioFileName); 

int mp3EmbedPayload		    (char *audioFileName, 
							 char *audioFileNameOut, 
							 char *uitsPayloadXML,
							 int  numPadBytes);

char *mp3ExtractPayload		(char *audioFileName); 

char *mp3GetMediaHash		(char *audioFileName);

// int mp3ValidateMediaHash	(char *audioFileName, 
//							 char *mediaHashValue);

/* 
 * PRIVATE Functions 
 */

int mp3CheckFileVersion		(char *audioFileName);

int mp3IdentifyFrame		(FILE *fpin);
int mp3HasID3v1Tag			(FILE *fpin);
int mp3IsVBRFrame			(FILE *fpin, 
							 MP3_AUDIO_FRAME_HEADER *frameHeader);

MP3_ID3_HEADER		   *mp3ReadID3Header		(FILE *fpin);
MP3_AUDIO_FRAME_HEADER *mp3ReadAudioFrameHeader	(FILE *fpin);

int  mp3HandleID3Tag		(FILE *audioInFP, FILE *audioOutFP);
int  mp3HandleID3Frame		(FILE *audioInFP, FILE *audioOutFP);
int  mp3SkipPadBytes		(FILE *audioInFP);
int  mp3WritePadBytes		(FILE *audioOutFP, int numPadBytes);

int  mp3WritePRIVFrame		(FILE *audioOutFP, char *uitsPayloadXML); 
int	 mp3WriteID3Header		(FILE *audioOutFP, MP3_ID3_HEADER *mp3Header);

char *mp3FindUITSPayload	(FILE *audioInFP); 

void uitsMake28From32		(long *length);
void uitsMake32From28		(long *length);

/*
 *  Housekeeping functions - to convert endian-ness of 2, 4, and 8-byte integers, when necessary...
 *
 */

int wswap(short *word);
int lswap(long *lword);
int llswap(unsigned long long *llword);


#endif

