/*
 *  uitsHTMLManager.c
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 11/11/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 * 
 */

#include "uits.h"


char *htmlModuleName = "uitsHTMLManager.c";


/*
 *
 * Function: htmlIsValidFile
 * Purpose:	 Check to see if file is an html file
 *			
 *
 * Returns:   TRUE
 */

int htmlIsValidFile (char *inputFileName) 
{
	
	FILE		*inputFP;
	mxml_node_t	*topNode;
	
	inputFP = fopen(inputFileName, "rb");
	uitsHandleErrorPTR(htmlModuleName, "htmlIsValidFile", inputFP, ERR_FILE, "Couldn't open input file for reading\n");
	
	topNode = mxmlLoadFile(NULL, inputFP, MXML_OPAQUE_CALLBACK);
	if (topNode) {
		vprintf("Input file is HTML ");
		return (TRUE);

	}
	return (FALSE);
}

/*
 *
 * Function:htmlGetMediaHash
 * Purpose:	 Calcluate the media hash for an html file
 *			 The Media Hash is created by hashing the entire file, except for the payload if it is in there
 *
 * Returns:   Pointer to the hash
 */

char *htmlGetMediaHash (char *inputFileName) 
{
	char				*inputHTMLString;
	char				*hashHTMLString;
	char				*uitsPayloadStart;
	char				*uitsPayloadEnd;
	UITS_digest			*mediaHash = NULL;
	char				*mediaHashString = NULL;
	
		
	inputHTMLString = uitsReadFile(inputFileName);
	uitsHandleErrorPTR(htmlModuleName, "htmlGetMediaHash", inputHTMLString, ERR_FILE, "Couldn't read input file\n");
	
	/* strip out the uits payload, if necessary */
	uitsPayloadStart = strstr (inputHTMLString, "<uits:UITS");
	if (uitsPayloadStart) {
		
		hashHTMLString = strdup(inputHTMLString);
		uitsHandleErrorPTR(htmlModuleName, "htmlGetMediaHash", hashHTMLString, ERR_FILE, "Couldn't duplicate html string\n");

		uitsPayloadStart = strstr (hashHTMLString, "<uits:UITS");

		/* search for the end of the uits payload in the input string */
		printf("About to search for /head\n");
		uitsPayloadEnd = strcasestr (inputHTMLString, "</head>");
		uitsHandleErrorPTR(htmlModuleName, "htmlGetMediaHash", uitsPayloadEnd, ERR_FILE, 
						   "Couldn't find end of UITS payload in input file\n");
		printf("uitsPayloadEnd: %s", uitsPayloadEnd);

		/* overwrite the payload with the contents of the original html file */
		strcpy (uitsPayloadStart, uitsPayloadEnd);
		
	} else { // no payload, just use the input HTML string as-is
		hashHTMLString = inputHTMLString;
	}
	
		
//	mediaHash = uitsCreateDigestBuffered (hashHTMLString, fileLength, "SHA256") ;
	mediaHash = uitsCreateDigest (hashHTMLString, "SHA256");
	mediaHashString = uitsDigestToString(mediaHash);

	return (mediaHashString);
	
}

/*
 *
 * Function:htmlEmbedPayload
 * Purpose:	Embed uits payload in an HTML file. The payload is embedded
 *			at the end of the <HEAD> element, right before the </HEAD> tag.
 *
 * Returns:  ERROR
 */

int htmlEmbedPayload  (char *inputFileName, 
				   char *outputFileName, 
				   char *uitsPayloadXML,
				   int  numPadBytes) 
{
	FILE			*inputFP;
	FILE			*outputFP;
	char			*inputHTMLString;
	char			*existingPayload;
	unsigned long	payloadXMLSize;
	char		    *strippedPayloadXML;
	char			*uitsPayloadStart;
	int				inputHTMLHeaderSize, inputHTMLBodySize;
	
	vprintf("Embedding payload in file %s\n", outputFileName);
	vprintf("Input file is: %s\n", inputFileName);

	/* make sure input file doesn't already have a UITS payload */
	existingPayload = htmlExtractPayload(inputFileName);
	if (existingPayload) {
		uitsHandleErrorPTR(htmlModuleName, "htmlEmbedPayload", NULL, ERR_FILE, 
						   "Couldn't embed payload. Input file has an existing UITS payload");
	}

	/* remove the <?xml> header from the uits payload */
	strippedPayloadXML = strstr(uitsPayloadXML, "<uits:UITS");
	payloadXMLSize = strlen(strippedPayloadXML);
	
	
	outputFP = fopen(outputFileName, "wb");
	uitsHandleErrorPTR(htmlModuleName, "htmlEmbedPayload", outputFP, ERR_FILE, "Couldn't open output file for writing\n");
		
	inputHTMLString = uitsReadFile(inputFileName);
	uitsHandleErrorPTR(htmlModuleName, "htmlEmbedPayload", inputHTMLString, ERR_FILE, "Couldn't read input file f\n");
	
	/* insert the payload right before the closing header tag */
	uitsPayloadStart = strcasestr (inputHTMLString, "</head>");

	/* some pointer math to figure out where to split the input HTML string */
	inputHTMLHeaderSize = uitsPayloadStart - inputHTMLString;
	inputHTMLBodySize = strlen(inputHTMLString) - inputHTMLHeaderSize;
	
	/* write data from beginning of file to start of </head> */
	fwrite(inputHTMLString, 1, inputHTMLHeaderSize, outputFP);

	/* write the payload */
	fwrite(strippedPayloadXML, 1, payloadXMLSize, outputFP);	
	
	/* write the rest of the file */
	fwrite(uitsPayloadStart, 1,inputHTMLBodySize, outputFP);
	
	fclose(outputFP);
	
	return(OK);
}

/*
 *
 * Function:htmlExtractPayload
 * Purpose:	 Extract the UITS payload from an HTML file
 *
 * Returns:	 pointer to the payload or NULL if not found
 */

char *htmlExtractPayload (char *inputFileName) 
{
	FILE *inputFP;
	char *inputHTMLString;
	char *uitsPayloadStart;
	char *uitsPayloadEnd;
	
	
	inputHTMLString = uitsReadFile(inputFileName);
	uitsHandleErrorPTR(htmlModuleName, "htmlEmbedPayload", inputHTMLString, ERR_FILE, "Couldn't read input file\n");
	
	/* search for the start of the uits payload */
	uitsPayloadStart = strstr (inputHTMLString, "<uits:UITS");
	if (uitsPayloadStart) {
		/* search for the end of the uits payload */
		uitsPayloadEnd = strcasestr (inputHTMLString, "</head>");
		uitsHandleErrorPTR(htmlModuleName, "htmlExtractPayload", uitsPayloadEnd, ERR_FILE, "Couldn't find end of UITS payload in input file\n");
		
		/* make the payload string end at the start of the header end tag */
		*uitsPayloadEnd = '\0';
		return (uitsPayloadStart);
	}
	
	/* no uits payload in file */
	return (NULL);
}


// EOF

