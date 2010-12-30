/*
 *  uitsGenericManager.c
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 11/4/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 */ 

#include "uits.h"


char *genericModuleName = "uitsGenericManager.c";


/*
 *
 * Function: genericIsValidFile
 * Purpose:	 Fallthrough file type.
 *			
 *
 * Returns:   TRUE
 */

int genericIsValidFile (char *inputFileName) 
{
	
	vprintf("Unknown input file type. Creating generic UITS payload.\n");

	
	return (TRUE);
}

/*
 *
 * Function: genericGetMediaHash
 * Purpose:	 Calcluate the media hash for a generic file
 *			 The Media Hash is created by hashing the entire file
 *
 * Returns:   Pointer to the hash
 */

char *genericGetMediaHash (char *inputFileName) 
{
	FILE				*inputFP;
	UITS_digest			*mediaHash = NULL;
	char				*mediaHashString = NULL;
	
	unsigned long	fileLength;
		
	inputFP = fopen(inputFileName, "rb");
	uitsHandleErrorPTR(genericModuleName, "genericGetMediaHash", inputFP, ERR_FILE, "Couldn't open input file for reading\n");
		
	/* get file size */
	fileLength = uitsGetFileSize(inputFP);
		
	mediaHash = uitsCreateDigestBuffered (inputFP, fileLength, "SHA256") ;
	mediaHashString = uitsDigestToString(mediaHash);
	
	fclose(inputFP);
	
	return (mediaHashString);
	
}

/*
 *
 * Function: genericEmbedPayload
 * Purpose:	 Stub function required for all file types. Embedding is not supported for generic file types.
 *
 * Returns:  ERROR
 */

int genericEmbedPayload  (char *inputFileName, 
						  char *outputFileName, 
						  char *uitsPayloadXML,
						  int  numPadBytes) 
{
	FILE *outputFP;
	unsigned long	payloadXMLSize;
	
	vprintf("Cannot embed UITS payload into unknown file type. Writing standalone UITS paylaod file to %s\n", outputFileName)
	
	outputFP = fopen(outputFileName, "wb");
	uitsHandleErrorPTR(genericModuleName, "genericEmbedPayload", outputFP, ERR_FILE, "Couldn't open output file for writing\n");
	
	payloadXMLSize = strlen(uitsPayloadXML);
	
	fwrite(uitsPayloadXML, 1, payloadXMLSize, outputFP);	/* UITS payload */

	fclose(outputFP);
	
	return(OK);
}

/*
 *
 * Function: genericExtractPayload
 * Purpose:	 Stub function required for all file types. Embedding is not supported for generic file types.
 *
 * Returns:	 NULL
 */

char *genericExtractPayload (char *inputFileName) 
{
	
	
	uitsHandleErrorPTR (genericModuleName, "genericEmbedPayload", NULL, ERR_FILE, 
					   "Input file has unknown type. Embeded payload not supported.\n");
	
	return (NULL);
}

	
// EOF

