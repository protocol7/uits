/*
 *  uitsFLACManager.h
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

#ifndef _uitsflacmanager_h_
#  define _uitsflacmanager_h_

#include <metadata.h>
#include <stream_decoder.h>


/* 
 * PUBLIC Functions 
 */

int flacIsValidFile		(char *audioFileName);

int flacEmbedPayload	(char *audioFileName, 
						 char *audioFileNameOut, 
						 char *uitsPayloadXML,
						 int  numPadBytes);

char *flacExtractPayload (char *audioFileName); 

char *flacGetMediaHash	(char *audioFileName);


/*
 * PRIVATE Functions
 */


FLAC__StreamDecoderWriteStatus flacWriteCallback(const FLAC__StreamDecoder *decoder, 
												 const FLAC__Frame *frame, 
												 const FLAC__int32 * const buffer[], 
												 void *client_data);

void flacMetadataCallback(const FLAC__StreamDecoder *decoder, 
						  const FLAC__StreamMetadata *metadata, 
						  void *client_data);

void flacErrorCallback(const FLAC__StreamDecoder *decoder, 
					   FLAC__StreamDecoderErrorStatus status, 
					   void *client_data);

int flacCloneAudioFile (char *audioFileName,
						char *audioFileNameOut);
#endif

// EOF

