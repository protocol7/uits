/*
 *  uitsFLACManager.c
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 4/19/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  This module uses the libFLAC C API to read and write FLAC
 *  files. 
 *
 *  $Date$
 *  $Revision$
 */

#include "uits.h"


char *flacModuleName = "uitsFLACManager.c";


/*
 *
 * Function: flacIsValidFile
 * Purpose:	 Determine if an audio file is an FLAC file
 *			
 *
 * Returns:   TRUE if FLAC, FALSE otherwise
 */

int flacIsValidFile (char *audioFileName) 
{
	FLAC__StreamMetadata *streamMetadata = calloc(sizeof(FLAC__StreamMetadata),1);

	/* All FLAC files must have a stream info metadata block. */ 
	/* FLAC_metadata_get_streaminfo will return true if metadata read, false if not */
	err = FLAC__metadata_get_streaminfo(audioFileName, streamMetadata);
	
	if (err == TRUE) {
		vprintf("Audio file is FLAC\n");
	}

	return (err);
	
	
}

/*
 *
 * Function: flacGetMediaHash
 * Purpose:	 Calcluate the media hash for a FLAC file
 *			 The FLAC file is parsed using the following algorithm:
 *			  1. Use the flac decoder API to decode all of the metadata blocks
 *			  2. The filepointer should be left at the start of the first audio frame
 *			  3. Hash the rest of the data in the file, which is the audio frame data
 *
 * Returns:   Pointer to the hashed frame data
 */

char *flacGetMediaHash (char *audioFileName) 
{
	FILE				*audioFP;
	FLAC__StreamDecoder *decoder = NULL;
	unsigned long		audioFrameStart, audioFrameEnd, audioFrameLength;
	UITS_digest			*mediaHash;
	char				*mediaHashString;
	
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(flacModuleName, "flacGetMediaHash", audioFP, "Couldn't open FLAC audio file for reading\n");
	
	decoder = FLAC__stream_decoder_new();
	uitsHandleErrorPTR(flacModuleName, "flacGetMediaHash", decoder, "Couldn't create FLAC stream decoder\n");

		
	err	= FLAC__stream_decoder_init_FILE (decoder, audioFP, flacWriteCallback, flacMetadataCallback, flacErrorCallback, NULL);
	uitsHandleErrorINT(flacModuleName, "flacGetMediaHash", err, FLAC__STREAM_DECODER_INIT_STATUS_OK, 
					   "Couldn't initialize decoder for file\n");
	
	err = FLAC__stream_decoder_process_until_end_of_metadata(decoder);
	uitsHandleErrorINT(flacModuleName, "flacGetMediaHash", err, TRUE, "FLAC stream decode failed\n");

//	fprintf(stderr, "   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);

	
	audioFrameStart  = ftello(audioFP);
	audioFrameEnd    = uitsGetFileSize(audioFP);
	audioFrameLength = audioFrameEnd - audioFrameStart;

	mediaHash = uitsCreateDigestBuffered (audioFP, audioFrameLength, "SHA256") ;
	mediaHashString = uitsDigestToString(mediaHash);
	
	/* cleanup */
	FLAC__stream_decoder_delete(decoder);
	fclose(audioFP);
	
	return (mediaHashString);
	
}

/*
 *
 * Function: flacEmbedPayload
 * Purpose:	 Embed the UITS payload into an FLAC file
 *			 The following algorithm is used to embed the payload:
 *				1. Clone the input audio file to the output file.
 *				2. Create a new application metadata object for the UITS payload
 *				3. Populate the metadata object
 *				4. Read the existing metadata chain from the file
 *				5. Consolidate any padding in the metadata
 *				6. Add the UITS metadata object to the chain
 *				7. Write the updated metadata chain
 *
 *
 * Returns:   OK or ERROR
 */

int flacEmbedPayload  (char *audioFileName, 
					  char *audioFileNameOut, 
					  char *uitsPayloadXML,
					  int  numPadBytes) 
{
	FLAC__StreamMetadata    *uitsFlacMetadata = NULL;
	FLAC__StreamMetadata    *flacMetadata = NULL;
	FLAC__Metadata_Chain    *flacMetadataChain = NULL;
	FLAC__Metadata_Iterator *flacMetadataIterator = NULL;
	
	FLAC__byte			 *uitsMetadata;
	unsigned long		 payloadSize;
	int					 stillWalking;
	
	/* clone the input file to the output file */
	err = flacCloneAudioFile (audioFileName, audioFileNameOut);
	uitsHandleErrorINT(flacModuleName, "flacEMbedPayload", err, OK, "Couldn't copy input FLAC audio file to output file\n");
	
	/* create the new FLAC metadata block */
	uitsFlacMetadata = FLAC__metadata_object_new (FLAC__METADATA_TYPE_APPLICATION);
	uitsHandleErrorPTR(flacModuleName, "flacEmbedPayload", uitsFlacMetadata, "Couldn't create FLAC metadata object\n");
	
	
	/* add the UITS payload to the metadata object */
	
	/* the payload size must be a multiple of 8, so pad with 0 bytes if necessary */
	payloadSize = strlen(uitsPayloadXML);
	payloadSize	+= (payloadSize % 8);
	
	uitsMetadata = calloc(payloadSize, sizeof(FLAC__byte));
	uitsHandleErrorPTR(flacModuleName, "flacEmbedPayload", uitsMetadata, "Couldn't allocate uitsMetadata\n");	
	
	uitsMetadata = strcpy (uitsMetadata, uitsPayloadXML);
		
	err = FLAC__metadata_object_application_set_data (uitsFlacMetadata, uitsMetadata, payloadSize, FALSE);
  	uitsHandleErrorINT(flacModuleName, "flacEmbedPayload", err, TRUE, "Couldn't add UITS payload data to FLAC metadata object\n");
	
	/* Set the Application block application ID */
	
	uitsFlacMetadata->data.application.id[0] = 'U';
	uitsFlacMetadata->data.application.id[1] = 'I';
	uitsFlacMetadata->data.application.id[2] = 'T';
	uitsFlacMetadata->data.application.id[3] = 'S';
	
	/* now read the metadata chain from the existing FLAC file */
	
	flacMetadataChain = FLAC__metadata_chain_new ();
	uitsHandleErrorPTR(flacModuleName, "flacEmbedPayload", flacMetadataChain, "Couldn't create FLAC metadata chain\n");	
	
	err = FLAC__metadata_chain_read(flacMetadataChain, audioFileNameOut);
	uitsHandleErrorINT(flacModuleName, "flacEmbedPayload", err, TRUE, "Couldn't read FLAC metadata chain\n");
	
	/* move all of the padding blocks, if any, to the end of the chain */
	FLAC__metadata_chain_sort_padding (flacMetadataChain);

	flacMetadataIterator = FLAC__metadata_iterator_new ();
	uitsHandleErrorPTR(flacModuleName, "flacEmbedPayload", flacMetadataChain, "Couldn't create FLAC metadata iterator\n");	

	/* point the iterator at the chain */
	FLAC__metadata_iterator_init (flacMetadataIterator,flacMetadataChain);
	
	/* walk the iterator to the end of the chain and make sure there isn't already a UITS block */
	while (FLAC__metadata_iterator_next(flacMetadataIterator)) {
		if (FLAC__metadata_iterator_get_block_type(flacMetadataIterator) == FLAC__METADATA_TYPE_APPLICATION) {
			flacMetadata = FLAC__metadata_iterator_get_block (flacMetadataIterator);
			if ((flacMetadata->data.application.id[0] == 'U') &&
				(flacMetadata->data.application.id[1] == 'I') &&
				(flacMetadata->data.application.id[2] == 'T') &&
				(flacMetadata->data.application.id[3] == 'S')) {
				
				uitsHandleErrorINT(flacModuleName, "flacEmbedPayload", err, 0, "Audio input file already contains a UITS payload \n");

			}
		}
	}

	/* add the new block */
	err = FLAC__metadata_iterator_insert_block_after (flacMetadataIterator, uitsFlacMetadata);
  	uitsHandleErrorINT(flacModuleName, "flacEmbedPayload", err, TRUE, "Couldn't add UITS payload block to iterator\n");

	
	/* write the new chain */
	err = FLAC__metadata_chain_write (flacMetadataChain, TRUE, FALSE);
  	uitsHandleErrorINT(flacModuleName, "flacEmbedPayload", err, TRUE, "Couldn't write new metadata chain\n");

	/* cleanup */
	FLAC__metadata_chain_delete (flacMetadataChain);
	
	return(OK);
}

/*
 *
 * Function: flacExtractPayload
 * Purpose:	 Extract the UITS payload from an FLAC file
 *
 * Returns: pointer to payload or exit if error
 */

char *flacExtractPayload (char *audioFileName) 
{
	FLAC__StreamMetadata    *uitsFlacMetadata = NULL;
	FLAC__Metadata_Chain    *flacMetadataChain = NULL;
	FLAC__Metadata_Iterator *flacMetadataIterator = NULL;
		
	
	/* read the metadata chain from the existing FLAC file */
	flacMetadataChain = FLAC__metadata_chain_new ();
	uitsHandleErrorPTR(flacModuleName, "flacEmbedPayload", flacMetadataChain, "Couldn't create FLAC metadata chain\n");	
	
	err = FLAC__metadata_chain_read(flacMetadataChain, audioFileName);
	uitsHandleErrorINT(flacModuleName, "flacEmbedPayload", err, TRUE, "Couldn't read FLAC metadata chain\n");
	
	/* create an iterator to walk the chain */
	flacMetadataIterator = FLAC__metadata_iterator_new ();
	uitsHandleErrorPTR(flacModuleName, "flacEmbedPayload", flacMetadataChain, "Couldn't create FLAC metadata iterator\n");	
	
	/* point the iterator at the chain */
	FLAC__metadata_iterator_init (flacMetadataIterator,flacMetadataChain);
	
	/* walk the iterator to the end of the chain */
	while (FLAC__metadata_iterator_next(flacMetadataIterator)) { 
		if (FLAC__metadata_iterator_get_block_type(flacMetadataIterator) == FLAC__METADATA_TYPE_APPLICATION) {
			uitsFlacMetadata = FLAC__metadata_iterator_get_block (flacMetadataIterator);
			if ((uitsFlacMetadata->data.application.id[0] == 'U') &&
				(uitsFlacMetadata->data.application.id[1] == 'I') &&
				(uitsFlacMetadata->data.application.id[2] == 'T') &&
				(uitsFlacMetadata->data.application.id[3] == 'S')) {

				return ((char *) uitsFlacMetadata->data.application.data);
			}
		}
	}
		
	/* cleanup */
	FLAC__metadata_chain_delete (flacMetadataChain);
	
	/* didn't find the UITS application metadata block */
	return (NULL);
	
}

FLAC__StreamDecoderWriteStatus flacWriteCallback(const FLAC__StreamDecoder *decoder, 
												 const FLAC__Frame *frame, 
												 const FLAC__int32 * const buffer[], 
												 void *client_data)
{

//	vprintf("flacWriteCallback\n");
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flacMetadataCallback(const FLAC__StreamDecoder *decoder, 
						  const FLAC__StreamMetadata *metadata, 
						  void *client_data)
{
	(void)decoder, (void)client_data;
	
	/* print some stats */
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		vprintf("Got FLAC_METADATA_TYPE_STREAMINFO\n");
	} else {
		vprintf("Got flac metadata type: %d\n", metadata->type);
	}
					
}

void flacErrorCallback(const FLAC__StreamDecoder *decoder, 
					   FLAC__StreamDecoderErrorStatus status, 
					   void *client_data)
{
	(void)decoder, (void)client_data;
	char *flacErrStr;
	
	uitsHandleErrorINT(flacModuleName, "flacErrorCallback", ERROR, OK, FLAC__StreamDecoderErrorStatusString[status]);
}

/*
 *
 * Function: flacCloneAudioFile
 * Purpose:	 Clone the audio input file to the audio output file
 *           This is required because the FLAC metdata api does reads and writes
 *           to the same file. We want to write our modified metdata to a new file.
 *
 * Returns: OK or ERROR
 */

int flacCloneAudioFile (char *audioFileName,
						char *audioFileNameOut)
{
	FILE			*audioInFP, *audioOutFP;
	unsigned long	audioInFileSize;


	/* open the audio input and output files */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(flacModuleName, "flacCloneAudioFile", audioInFP, "Couldn't open audio file for reading\n");

	audioOutFP = fopen(audioFileNameOut, "wb");
	uitsHandleErrorPTR(flacModuleName, "flacCloneAudioFile", audioOutFP, "Couldn't open audio file for writing\n");

	/* calculate how long the input audio file is by seeking to EOF and saving size  */
	audioInFileSize = uitsGetFileSize(audioInFP);

	/* clone the input file to the output file */
	/* this is required because the FLAC metadata API will only read and write metadata to the same file */
	uitsAudioBufferedCopy(audioInFP, audioOutFP, audioInFileSize);

	fclose(audioInFP);
	fclose(audioOutFP);
	
	return (OK);

}
