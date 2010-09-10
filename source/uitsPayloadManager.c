/*
 *  uitsPayloadManager.c
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 12/3/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *	$Date$
 *	$Revision$
 *
 */

#include "uits.h"

char *payloadModuleName = "uitsPayloadManager.c";

// CMA: these are all command-line parameters. this needs be cleaned up in version 2.0

char *audioFileName;				// audio file name 
char *metadataFileName;				// metadata file name 
char *payloadFileName;				// UITS payload file name 
char *XSDFileName;					// UITS scema description 
char *outputFileName;				// Output file name 

int	 embedFlag;						// set if payload should be embedded into audio fle
int	 verifyFlag;					// set if extracted payload should be verified
int  numPadBytes;					// number of bytes of padding to insert into MP3 ID3 tag (optional)
int  gpMediaHashFlag;				// genparam: Media_Hash
int  gpB64MediaHashFlag;			// genparam: Base64 Media_Hash
int  gpPubKeyIDFlag;				// genparam: Public Key ID 
int  mediaHashNoVerifyFlag;			// set if media hash should not be verified

char *clMediaHashValue;				// media hash value passed from the command-line
char *mediaHashFileName;			// file containing pre-computed media hash

UITS_command_line_params clParams [] = {
	{"embed",		   &embedFlag},
	{"verify",         &verifyFlag},
	{"pad",            &numPadBytes},
	{"media_hash",     &gpMediaHashFlag},
	{"b64_media_hash", &gpB64MediaHashFlag},
	{"public_key_ID",  &gpPubKeyIDFlag},
	{"nohash",		   &mediaHashNoVerifyFlag},
	
	{0,	0}	// end of list	
};



/*
 *  The following data structures describe the UITS payload element and attribute data:
 */

#define MAX_NUM_ATTRIBUTES 5
#define MAX_NUM_ELEMENTS   MAX_COMMAND_LINE_OPTIONS

UITS_attributes uits_product_ID_attributes [] = {
	{"type",		"UPC"},
	{"completed",	"false"},
	{0, 0}	// end of list
};

UITS_attributes uits_asset_ID_attributes [] = {
	{"type",		"ISRC"},
	{0, 0}	// end of list
};

UITS_attributes uits_TID_attributes [] = {
	{"version",		"1"},
	{0, 0}	// end of list
};

UITS_attributes uits_UID_attributes [] = {
	{"version",		"1"},
	{0, 0}	// end of list
};

UITS_attributes uits_URL_attributes [] = {
	{"type",		"WPUB"},
	{0, 0}	// end of list
};

UITS_attributes uits_copyright_attributes [] = {
	{"value",		"allrightsreserved"},
	{0, 0}	// end of list
};

UITS_attributes uits_media_attributes [] = {
	{"algorithm",		"SHA256"},
	{0, 0}	// end of list
};

UITS_attributes uits_extra_attributes [] = {
	{"type",		NULL},
	{0, 0}	// end of list
};



UITS_element uitsMetadataDesc [MAX_NUM_ELEMENTS] = {
	{"nonce",		NULL, FALSE, NULL},
	{"Distributor", NULL, FALSE, NULL},
	{"Time",		NULL, FALSE, NULL},
	{"ProductID",	NULL, FALSE, uits_product_ID_attributes},
	{"AssetID",		NULL, FALSE, uits_asset_ID_attributes},
	{"TID",			NULL, FALSE, uits_TID_attributes},
	{"UID",			NULL, FALSE, uits_UID_attributes},
	{"Media",		NULL, FALSE, uits_media_attributes},
	{"URL",			NULL, FALSE, uits_URL_attributes},
	{"URLS",		NULL, TRUE,  uits_URL_attributes},
	{"PA",			NULL, FALSE, NULL},
	{"Copyright",	NULL, FALSE, uits_copyright_attributes},
	{"Extra",		NULL, FALSE, uits_extra_attributes},
	{"Extras",		NULL, TRUE,  uits_extra_attributes},
	{0,	0, 0}	// end of list	
};


UITS_signature_desc *uitsSignatureDesc;

/* 
 * Function: uitsPayloadManagerInit
 * Purpose:  Initialize payload operations and structures if necessary
 * Returns:  OK
 *
 */

int uitsPayloadManagerInit (void) 
{
	// initialize the signature description
	uitsSignatureDesc =  calloc(sizeof(UITS_signature_desc), 1);
	uitsSignatureDesc->b64LFFlag		  = FALSE;	// add LF in b64 signature 
	uitsSignatureDesc->algorithm          = "RSA2048";
	uitsSignatureDesc->pubKeyFileName     = NULL;	// name of the file containing the private key for signing
	uitsSignatureDesc->privateKeyFileName = NULL;	// name of the file containing the public key for signature verification
	uitsSignatureDesc->pubKeyID			  = NULL;	// for now, public key ID must be passed on command line
	audioFileName		= NULL;
	metadataFileName	= NULL;
	payloadFileName		= NULL;
	mediaHashFileName	= NULL;
	outputFileName		= NULL;
	embedFlag			= FALSE;
	verifyFlag			= FALSE;
	numPadBytes			= 0;
	XSDFileName			= "uits.xsd";		// default to look for uits.xsd in current directory 
	gpMediaHashFlag     = FALSE;			// genparam: Media_Hash
	gpB64MediaHashFlag  = FALSE;			// genparam: Base64 Media_Hash
	gpPubKeyIDFlag      = FALSE;			// genparam: Public Key ID 
	clMediaHashValue	= NULL;				// media hash value passed from the command-line
	mediaHashFileName	= NULL;				// file containing pre-computed media hash
	mediaHashNoVerifyFlag = 0;
	return (OK);
}


/* 
 *  Function: uitsCreate ()
 *  Purpose:  Creates a UITS payload 
 * 			The payload is written to a file either as a standalone payload OR it is embedded into
 *			the audio data. In that case the whole audio file, including the payload and
 *			any extra pad bytes are written to the payload file.
 *  Returns:  OK or exit on error
 */

int uitsCreate () 
{
	
	mxml_node_t *xml = NULL;
	FILE		*payloadFP;
	char		*payloadXMLString;
	
	char *mediaHashValue;

	vprintf("Create UITS payload ...\n");

	/* make sure that all required parameters are non-null */
	uitsCheckRequiredParams("create");
	
	vprintf("Creating UITS payload from comand-line options ... \n");		
	
	if (clMediaHashValue) {	// if media hash value passed on command line, set it
		mediaHashValue = clMediaHashValue;
	} else {
		/* Calculate the media hash value for the audio file */
		dprintf("about to uitsAudioGetMediaHash");
		fflush (stdout);
		mediaHashValue = uitsAudioGetMediaHash(audioFileName);
		dprintf("done uitsAudioGetMediaHash");
		fflush (stdout);

		/* base 64 encode the media hash, if requested */
		if (gpB64MediaHashFlag) {
			vprintf("Base 64 Encoding Media Hash ...\n");
			mediaHashValue = uitsBase64Encode(mediaHashValue, strlen(mediaHashValue), TRUE);
			
		}
	}

	/* Set the element value in the uits metadata array */
	
	err = uitsSetMetadataValue ("Media", mediaHashValue);
	uitsHandleErrorINT(payloadModuleName, "uitsCreateMediaHashElement", err, OK, ERR_PAYLOAD,
					"Couldn't set metadata value for Media hash\n");
	
	/* create the XML in an mxml data structure from the metadata array */
	
	xml = uitsCreatePayloadXML ();
	uitsHandleErrorPTR(payloadModuleName, "uitsCreate", xml, ERR_PAYLOAD,
					"Error: Couldn't create XML payload\n");

	/* convert the mxml data structure into a string containing the XML */
	payloadXMLString = uitsMXMLToXMLString(xml);
	
	// TO BE IMPLEMENTED: if no metadata file specified and no command-line options specified, try to read from standard in
	
	
	// validate the xml payload that was created
	vprintf("Validating payload ...\n");
	
	mediaHashNoVerifyFlag = TRUE;	// dont' verify the media hash on create
	
	err =  uitsVerifyPayloadXML (xml, payloadXMLString);
	uitsHandleErrorINT(payloadModuleName, "uitsCreate", err, OK, ERR_PAYLOAD, 
					"Error: Couldn't validate XML payload\n");
	
	//	mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK); // no whitespace
	
	// write the output to a separate payload file or insert it into the audio file
	
	if (embedFlag) {
		vprintf("Embedding payload and writing audio file: %s ...\n", payloadFileName);
		

		/* read the audio file, embed the XML into it, write the new audio file */
		err = uitsAudioEmbedPayload (audioFileName, payloadFileName, payloadXMLString, numPadBytes);
		uitsHandleErrorINT(payloadModuleName, "uitsCreate", err, OK, ERR_PAYLOAD, "Couldn't embed payload into audio file\n");
	} else {
		vprintf("Writing standalone UITS payload to file: %s ...\n", payloadFileName);
		
		payloadFP = fopen(payloadFileName, "wb");
		uitsHandleErrorPTR(payloadModuleName, "uitsCreate", payloadFP, ERR_FILE,
						"Error: Couldn't open payload file\n");
		
		err = mxmlSaveFile(xml, payloadFP, MXML_NO_CALLBACK);
		uitsHandleErrorINT(payloadModuleName, "uitsCreate", err, 0, ERR_FILE,
						"Error: Couldn't open save xml to file\n");
		
		fclose(payloadFP);
	}
	
	vprintf("Success\n");
	return (OK);
	
}


/*
 * Function: uitsVerify ()
 * Purpose:	 Verify the UITS payload 
 * Returns:  OK or exit on error
 */

int uitsVerify (void) 
{
	FILE *payloadFP;
	char *uitsPayloadXML;
	mxml_node_t *xml;
	
	vprintf("Verify UITS payload ...\n");
	
	/* make sure that all required parameters are non-null */
	uitsCheckRequiredParams("verify");
	
	/* initialize the payload xml by either reading it from a standalone payload or
	 * extracting from an audio file. If both standalone and audio file are specified,
	 * standalone takes precedence.
	 */
	
	if (payloadFileName) {	/* verify standalone payload */
		vprintf("About to verify payload in file %s ...\n", payloadFileName);

		uitsPayloadXML = uitsReadFile(payloadFileName);
	} else {
		vprintf("About to verify payload in file %s ...\n", audioFileName);
		
		uitsPayloadXML = uitsAudioExtractPayload (audioFileName);
		uitsHandleErrorPTR(payloadModuleName, "uitsVerify", uitsPayloadXML, ERR_PAYLOAD,
						   "Couldn't extract payload XML from audio file\n");
		
	}
	
	/*	convert the XML string to an mxml tree */
	xml = mxmlLoadString (NULL, uitsPayloadXML, MXML_OPAQUE_CALLBACK);
	uitsHandleErrorPTR(payloadModuleName, "uitsVerify", xml, ERR_PAYLOAD,
					   "Couldn't convert payload XML to  xml tree\n");
	
	err =  uitsVerifyPayloadXML (xml, uitsPayloadXML);
	uitsHandleErrorINT(payloadModuleName, "uitsVerifyPayloadFile", err, 0, ERR_VERIFY,
					   "Error: Payload failed validation\n");
	
	
	vprintf("Payload verified!\n");
	
	return (OK);
}



/*
 * Function: uitsExtract ()
 * Purpose:	 Extract the UITS payload XML from an audio file and write it to a payload file
 * Returns:  OK or exit on error
 *
 */

int uitsExtract () 
{
	
	char *uitsPayloadXML;
	int  payloadLength;
	FILE *payloadFP;
	
	vprintf("Extract UITS payload ...\n");
	
	uitsCheckRequiredParams("extract");

	/* extract the uits payload XML from the audio file */
	
	vprintf("Extracting uits payload from %s ... \n", audioFileName);
	uitsPayloadXML = uitsAudioExtractPayload (audioFileName);
	uitsHandleErrorPTR(payloadModuleName, "uitsExtract", uitsPayloadXML, ERR_EXTRACT,
					"Couldn't extract payload XML from audio file\n");
	
	/* write the XML to the payload file */

	vprintf("Writing payload to %s ...\n", payloadFileName);
	payloadLength = strlen(uitsPayloadXML);
	
	payloadFP = fopen(payloadFileName, "wb");
	uitsHandleErrorPTR(payloadModuleName, "uitsExtract", payloadFP, ERR_FILE,
					   "Couldn't open payload file for output\n");
	
	err = fwrite(uitsPayloadXML, 1, payloadLength, payloadFP);
	uitsHandleErrorINT(payloadModuleName, "uitsAudioExtractPayload", err,payloadLength, 
					    ERR_FILE, "Couldn't write UITS payload to file\n");
	fclose(payloadFP);
	
	/* if requested, verify the payload */
	if (verifyFlag) {
		vprintf("About to verify payload in file %s ...\n", payloadFileName);
		err = uitsVerify ();
		uitsHandleErrorINT(payloadModuleName, "uitsAudioExtractPayload", err, OK, ERR_VERIFY,
						"Couldn't verify UITS payload file\n");
		vprintf("Payload verified\n");
		
	}
		
	vprintf("Success\n");
	return (OK);
	
	
}

/*
 * Function: uitsGenKey ()
 * Purpose:	 Generate a Key ID from a public key file
 * Returns:  OK or exit on error
 *
 */

int uitsGenKey () 
{
	UITS_digest		*pubKeyIDDigest;
	char			*pubKeyIDValue;
	FILE			*outFP;
	int				len;
	
	vprintf("Generate Key ...\n");
	
	uitsCheckRequiredParams("genkey");
	
	// extract the public key id from the file and create a SHA1 digest of the key
	pubKeyIDDigest = uitsGetPubKeyID (uitsSignatureDesc->pubKeyFileName);
		
	// Convert the key digest to a string
		
	pubKeyIDValue = uitsDigestToString(pubKeyIDDigest);
	vprintf("Public Key ID for file %s is %s\n", uitsSignatureDesc->pubKeyFileName, pubKeyIDValue);

	if (outputFileName) {
		vprintf("Writing public Key ID to file %s\n", outputFileName);
		outFP = fopen(outputFileName, "w");
		uitsHandleErrorPTR(outputFileName, "uitsGenKey", outFP, ERR_FILE, "Couldn't open output file\n");
		
		len = strlen(pubKeyIDValue);
		err = fwrite(pubKeyIDValue, 1, len, outFP);
		uitsHandleErrorINT(payloadModuleName, "uitsGenKey", err, len, ERR_FILE,
						   "Couldn't write public key ID to file\n");
		fclose(outFP);
	}
		
	return (OK);
	
}

/*
 * Function: uitsGenHash ()
 * Purpose:	 Generate a media hash from an audio file
 * Returns:  OK or exit on error
*
 */

int uitsGenHash () 
{
	unsigned char *mediaHash = NULL;
	unsigned char *b64MediaHash = NULL;
	unsigned char *outputMediaHash = NULL;
	FILE *outFP;
	int len;
	
	vprintf("Generate media hash ...\n");
	
	uitsCheckRequiredParams("genhash");
	
	mediaHash = uitsAudioGetMediaHash(audioFileName);
	vprintf("Media Hash for file %s is: \n\t%s\n", audioFileName, mediaHash);
	outputMediaHash = mediaHash;
	
	if (gpB64MediaHashFlag) {
		b64MediaHash = uitsBase64Encode(mediaHash, strlen(mediaHash), TRUE);
		outputMediaHash = b64MediaHash;
		vprintf("Base 64 Encoded Media Hash for file %s is:\n\t %s\n", audioFileName, b64MediaHash);
	}

	if (outputFileName) {
		vprintf("Writing media hash to file %s\n", outputFileName);
		outFP = fopen(outputFileName, "w");
		uitsHandleErrorPTR(outputFileName, "uitsGenHash", outFP, ERR_FILE, "Couldn't open output file\n");
		
		len = strlen(outputMediaHash);
		err = fwrite(outputMediaHash, 1, len, outFP);
		uitsHandleErrorINT(payloadModuleName, "uitsGenHash", err, len, ERR_FILE,
						   "Couldn't write media hash to file\n");
		fclose(outFP);
	}
	
	
	return (OK);
	
	
}

/*
 * Function: uitsCheckRequiredParams 
 * Purpose:	 Make sure all of the required parameters are set for create, verify or extract option
 *
 */
void uitsCheckRequiredParams (char *command)
{
	/* This is an ugly, brute-force method of checking, but time is short */
	
	
	vprintf("uitsCheckRequiredParams command: %s\n", command);
		

	if (strcmp(command, "create") == 0) {
		if (!audioFileName) {
			snprintf(errStr, ERRSTR_LEN, "Error: Can't %s UITS payload. No audio file specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!payloadFileName) {
			snprintf(errStr, ERRSTR_LEN, "Error: Can't %s UITS payload. No payload file specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (strcmp(audioFileName, payloadFileName) == 0) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. Payload file must have different name than audio file.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!uitsSignatureDesc->algorithm) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No algorithm specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!uitsSignatureDesc->pubKeyFileName) {
			err = snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No public key file specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!uitsSignatureDesc->privateKeyFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No private key file specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!uitsSignatureDesc->pubKeyID) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No public key ID specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
	}
	
	if (strcmp(command, "verify") == 0) {
		if (!uitsSignatureDesc->algorithm) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No algorithm specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!uitsSignatureDesc->pubKeyFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No public key file specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		
		/* The media hash can be verified in one of 3 ways
		 *	1. Calculate the media hash in the audio file and compare with what's in the UITS payload
		 *  2. Pass the media hash on the commandline and compare with what's in the UITS payload
		 *  3. Read a media hash from a file and compare with what's in the UITS payload
		 *
		 *  Therefore, either an audio file, a media hash, or a media hash file must be specfied on 
		 * the command line
		 */
		 
		 if (!audioFileName) {
			 if (!payloadFileName) { /* no audio file and no payload file. nothing to verify */
				 snprintf(errStr, ERRSTR_LEN, 
						  "Error: Can't %s UITS payload.  No payload or audio file specified for verification.\n", command);
				 uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
			 }
			 if (!mediaHashNoVerifyFlag && !clMediaHashValue && !mediaHashFileName) {
				 snprintf(errStr, ERRSTR_LEN, 
						  "Error: Can't %s UITS payload. No audio file or media hash specified.\n", command);
				 uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
			 }
		 } else {

			 if (!mediaHashNoVerifyFlag && (clMediaHashValue && mediaHashFileName)) {
				 snprintf(errStr, ERRSTR_LEN, 
						  "Error: Can't %s UITS payload. Multiple reference media hashes specified. Please provide either command line value or file.", 
						  command);
				 uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
			 }
		 
		 }
	}
	
	if (strcmp(command, "extract") == 0) {
		if (!audioFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No audio file specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!payloadFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. No payload file specified.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (strcmp(audioFileName, payloadFileName) == 0) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s UITS payload. Payload file must have different name than audio file.\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		
		if (verifyFlag) {
			if (!uitsSignatureDesc->algorithm) {
				snprintf(errStr, ERRSTR_LEN, 
						 "Error: Can't %s UITS payload. No algorithm specified\n", command);
				uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
			}
			if (!uitsSignatureDesc->pubKeyFileName) {
				snprintf(errStr, ERRSTR_LEN, 
						 "Error: Can't %s UITS payload. No public key file specified.\n", command);
				uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
			}
		}
	}

	if (strcmp(command, "genkey") == 0) {
		if (!uitsSignatureDesc->pubKeyFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't generate public key ID, no public key file file specified\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
	}

	if (strcmp(command, "genhash") == 0) {
		if (!audioFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't generate media hash, no audio file specified\n", command);
			uitsHandleErrorINT(payloadModuleName, "uitsCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
	}	
}

/* 
 *  Function: uitsGetMetadataDesc
 *  Purpose: Returns pointer to metadata element description
 *
 */

UITS_element *uitsGetMetadataDesc(void)
{
	return (uitsMetadataDesc);
}

UITS_signature_desc *uitsGetSignatureDesc(void)
{
	return (uitsSignatureDesc);
}

/*
*  Function: uitsCreatePayloadXML ()
*  Purpose:  Creates a mxml tree from data in the  UITS_element structure
*/

mxml_node_t * uitsCreatePayloadXML () 
{
	
	mxml_node_t *xml;
	mxml_node_t * xmlRootNode;
	mxml_node_t * xmlMetadataNode;
	mxml_node_t * xmlSignatureNode;
		
	// create the XML Root node
	
	xml = mxmlNewXML("1.0");
	xmlRootNode = mxmlNewElement(xml, "uits:UITS");
	mxmlElementSetAttr( xmlRootNode, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	mxmlElementSetAttr( xmlRootNode, "xmlns:uits", "http://www.udirector.net/schemas/2009/uits/1.1");
	
	// create the metadata node
	 xmlMetadataNode = uitsPayloadPopulateMetadata (xmlRootNode);
	
	// create the signature node
	 xmlSignatureNode = uitsPayloadPopulateSignature (xmlRootNode);
	
	return (xml);
	
}	

/* 
 * Function: uitsPayloadPopulateMetadata ()
 * Purpose:  Creates or updates the metadata element for the UITS payload and add it to the xml root
 * Returns:  pointer to MXML node or exit if error
 */

mxml_node_t * uitsPayloadPopulateMetadata (mxml_node_t * xmlRootNode) {
	
	mxml_node_t * xmlMetadataNode = NULL;
	mxml_node_t *elementNode = NULL;
	UITS_element *metadataPtr = uitsMetadataDesc;
	UITS_attributes *attributePtr;
	
	char *elementName;
	char *elementValue;
	char *attributeValue;
	
	
	/* does the metadata node already exist? */
	xmlMetadataNode = mxmlFindElement( xmlRootNode,  xmlRootNode, "metadata", NULL, NULL, MXML_DESCEND);

	if (!xmlMetadataNode) {	/* create it */
		xmlMetadataNode = mxmlNewElement( xmlRootNode, "metadata");
	}
	
	// populate the metadata element with values set on command line
	while (metadataPtr->name) {
		// TIME is treated specially. If the user hasn't specified, default to the currrent time
		if ((strcmp(metadataPtr->name, "Time") == 0) && !metadataPtr->value) {
			metadataPtr->value = uitsGetUTCTime();
		}
		
		// only set the node value if there is a value
		if (metadataPtr->value) {
			if (metadataPtr->multipleFlag) { /* multiple values are specified in a comma-delimited list */
				
				/* turn the plural name into a singular name (URLS becomes URL, Extras becomes Extra) */
				elementName = strdup(metadataPtr->name);
				elementName[strlen(elementName)-1] = 0;
								
				// create an element for each name in the comma-delimited list */
				elementValue = strtok(metadataPtr->value, ",");
				while (elementValue) {
					vprintf ("\t %s: %s\n",elementName, elementValue);
					elementNode = mxmlNewElement( xmlMetadataNode, elementName);
					mxmlNewOpaque(elementNode, elementValue);
					elementValue = strtok(NULL, ",");
				}
				
				// now populate the attributes for each new element
				attributePtr = metadataPtr->attributes;
				if (attributePtr) {
					while (attributePtr->name) {
						attributeValue = strtok(attributePtr->value, ",");
						elementNode = mxmlFindElement(xmlRootNode, xmlRootNode, elementName, NULL, NULL, MXML_DESCEND);
						while (attributeValue) {
							vprintf("\t\t %s: %s\n", attributePtr->name, attributeValue);
							mxmlElementSetAttr(elementNode, attributePtr->name, attributeValue);
							elementNode = mxmlWalkNext(elementNode, xmlRootNode, MXML_NO_DESCEND);
							attributeValue = strtok(NULL, ",");
						}
						attributePtr++;
					}
				}
			} else {
				vprintf ("\t %s: %s\n", metadataPtr->name, metadataPtr->value);
				/* create the mxml element node for this value */
				elementNode = mxmlNewElement( xmlMetadataNode, metadataPtr->name);
				mxmlNewOpaque(elementNode, metadataPtr->value);

				/* set the element attributes, if any */
				attributePtr = metadataPtr->attributes;
				if (attributePtr) {
					while (attributePtr->name) {
						vprintf("\t\t %s: %s\n", attributePtr->name, attributePtr->value);
						mxmlElementSetAttr(elementNode, attributePtr->name, attributePtr->value);
						attributePtr++;
					}
				}
			}
		}
		metadataPtr++;
	}
	
	return (xmlMetadataNode);
	
}

/* 
 * Function: uitsPayloadPopulateSignature ()
 * Purpose:  Creates the signature element for the UITS payload and add it to the xml root
 * Returns:  Pointer to MXML node or exit on error
 */

mxml_node_t * uitsPayloadPopulateSignature (mxml_node_t * xmlRootNode) 
{
	
	mxml_node_t		* xmlSignatureNode=NULL;
	mxml_node_t		*xmlSignatureOpaque=NULL;
//	UITS_digest		*pubKeyID;
	unsigned char	*encodedSignature;
	char			*signatureDigestName;
//	char			*pubKeyValue;
	char			*metadataString;
//	char			*strPtr;
//	int				i;
	

	/* does the signature node already exist? */
	xmlSignatureNode = mxmlFindElement( xmlRootNode,  xmlRootNode, "signature", NULL, NULL, MXML_DESCEND);
	
	if (!xmlSignatureNode) {	/* create it */
		xmlSignatureNode = mxmlNewElement( xmlRootNode, "signature");
		/* create a place holder signature. it will be replaced at the end of this function */
		mxmlNewOpaque (xmlSignatureNode, "placeholdersignature");
	}
	
		
	// set the algorithm, cannonicalization, and keyID attributes
	mxmlElementSetAttr( xmlSignatureNode, "algorithm", uitsSignatureDesc->algorithm);
	mxmlElementSetAttr( xmlSignatureNode, "canonicalization", "none");
	
	/* disabled for first release */
	/* this code will generate a Public Key ID from the key file */
	/*
		// extract the public key id from the file and create a SHA1 digest of the key
		pubKeyID = uitsGetPubKeyID (uitsSignatureDesc->pubKeyFileName);
		
		// Convert the key digest to a string
		
		pubKeyValue = uitsDigestToString(pubKeyID);
		mxmlElementSetAttr( xmlSignatureNode, "keyID", pubKeyValue);

	*/
	
	/* for now, just use the public key id passed on the command line */
	mxmlElementSetAttr( xmlSignatureNode, "keyID", uitsSignatureDesc->pubKeyID);

	
	// create a signed hash value for the signature value
	
	/* create hash of metadata */
	if (!strcmp(uitsSignatureDesc->algorithm, "RSA2048")) {
		signatureDigestName = "SHA256";
	} else if (!strcmp(uitsSignatureDesc->algorithm, "DSA2048")) {
		signatureDigestName = "SHA224";
	}
	
	/* create signature for metadata using the private key */
	metadataString = uitsGetMetadataStringMXML ( xmlRootNode);
	
	encodedSignature = uitsCreateSignature(metadataString, 
										   uitsSignatureDesc->privateKeyFileName, 
										   signatureDigestName, 
										   uitsSignatureDesc->b64LFFlag);
	
	
		
	xmlSignatureOpaque = mxmlWalkNext(xmlSignatureNode, xmlRootNode, MXML_DESCEND);
	uitsHandleErrorPTR(payloadModuleName, "signature", xmlSignatureOpaque, ERR_PARAM, 
					"Error: Couldn't get get signature value\n");
	
	err = mxmlSetOpaque(xmlSignatureOpaque, encodedSignature);
	uitsHandleErrorINT(payloadModuleName, "uitsPayloadPopulateSignature", err, 0, ERR_PARAM, 
					"Error: Couldn't update signature text value\n");
	
//	free (metadataString);
	
	return ( xmlSignatureNode);
	
}

/*
 *
 * Function:  uitsVerifyPayloadXML ()
 * Purpose:	 Validate the UITS payload against the UITS wsd schema, verify the mediahash, and verify the signature
 *			 This validation uses the libxml2 library
 * Returns: OK or exit on error
 */

int  uitsVerifyPayloadXML (mxml_node_t * xmlRootNode, char *payloadXMLString) 
{
	char		*metadataString;
	char		*signatureString;
	char		*signatureDigestName;
//	char		*pubKeyId;
	mxml_node_t *signatureElementNode;

	char		*mediaHash;
	
	/* validate the xml against the uits.xsd schema */
	vprintf("\tAbout to validate payload XML against schema\n");
	
	err = uitsValidatePayloadSchema (xmlRootNode);
	uitsHandleErrorINT(payloadModuleName, " uitsVerifyPayloadXML", err, 0, ERR_SCHEMA, 
					"Error: Couldn't verify the schema\n");
	
	vprintf("\tPayload passed schema validation\n");

	if (!mediaHashNoVerifyFlag) {	// don't verify media hash if set 
		/* verify that the media hash is correct */
		vprintf("\tAbout to verify media hash in payload XML\n");
		mediaHash = uitsGetElementText(xmlRootNode, "Media");
		uitsHandleErrorPTR(payloadModuleName, "uitsVerifyPayloadXML", mediaHash,  ERR_PAYLOAD,
						"Error: Couldn't get Media hash value from payload XML for validation\n");

		err = uitsVerifyMediaHash (mediaHash);
		uitsHandleErrorINT(payloadModuleName, " uitsVerifyPayloadXML", err, 0, ERR_HASH,
						   "Error: Couldn't verify the media hash\n");
		
		vprintf("\tMedia hash verified\n");
	}
	
	// To verify the signature we need the metadata element text, the public key file, and the signature
	metadataString	= uitsGetMetadataString ( payloadXMLString);	
	signatureString = uitsGetElementText( xmlRootNode, "signature");
	
	vprintf("metadataString: %s\n", metadataString);
	vprintf("signatureString: %s\n", signatureString);
	
	// CMA Note: Disabled until spec is clarified
	// Verify that the public key file is the correct one for this payload
	
	signatureElementNode = mxmlFindElement( xmlRootNode,  xmlRootNode, "signature", NULL, NULL, MXML_DESCEND);
//	uitsHandleErrorPTR(payloadModuleName, " uitsVerifyPayloadXML", signatureElementNode, ERR_SIG,
//					"Error: Couldn't find XML signature element node\n");
	
//	pubKeyId = mxmlElementGetAttr(signatureElementNode, "keyID");
//	uitsHandleErrorPTR(payloadModuleName, " uitsVerifyPayloadXML", pubKeyId, ERR_SIG,
//					"Error: Couldn't get get keyId attribute value\n");
	
//	err = uitsValidatePubKeyID (uitsSignatureDesc->pubKeyFileName, pubKeyId);
//	uitsHandleErrorINT(payloadModuleName, " uitsVerifyPayloadXML", err, OK, ERR_SIG,
//					"Error: Public key in file does not match keyID attribute in payload.\n");
//	
	/* convert the algorithm name to a digest name */
	if (!strcmp(uitsSignatureDesc->algorithm, "RSA2048")) {
		signatureDigestName = "SHA256";
	} else if (!strcmp(uitsSignatureDesc->algorithm, "DSA2048")) {
		signatureDigestName = "SHA224";
	}
	
	vprintf("\tAbout to verify signature with Public Key in file: %s\n", uitsSignatureDesc->pubKeyFileName );
	err = uitsVerifySignature(uitsSignatureDesc->pubKeyFileName, metadataString, signatureString, signatureDigestName);
	
	uitsHandleErrorINT(payloadModuleName, " uitsVerifyPayloadXML", err, 1, ERR_SIG,
					"Error: Couldn't validate signature\n");
	
	vprintf("\tPayload signature verified\n");
		
	return (OK);
	
}

/*
 *
 * Function: uitsVerifyMediaHash ()
 * Purpose:	 Verify that the media hash in the uits payload matches the reference media hash
 *			 The reference media hash can either be calculated from an audio file or provided on the command line
 *			 This validation uses the libxml2 library
 * Returns:  OK or exit on error
 */

int  uitsVerifyMediaHash (char *mediaHash) 
{
	char		*referenceMediaHash;
	int			rMHLen;
	char		*lastChar;

	// vprintf("mediaHash: %s\n", mediaHash);
	
	/* The command line reference media hash takes precedence over the audio file */
		
	if (clMediaHashValue) {
		vprintf("\tVerifying against command line reference media hash\n");
		referenceMediaHash = clMediaHashValue;
	} else if (mediaHashFileName) {
		vprintf("\tVerifying against reference media hash in file: %s\n", mediaHashFileName);
		referenceMediaHash = uitsReadFile(mediaHashFileName);
		uitsHandleErrorPTR(payloadModuleName, "uitsVerifyPayloadXML", mediaHash, ERR_FILE,
						   "Error: Couldn't read media hash from file for validation\n");
		
		/* some editors (ahem EMACS) add an extra new-line at the end of a file. */
		/* if it's there, remove it and warn the user */
		rMHLen = strlen(referenceMediaHash);
		lastChar = referenceMediaHash + (rMHLen - 1);
		
		if (strchr(lastChar, '\n')) {
			vprintf("WARNING: Media hash file has an extra newline at end. Removing it for comparison\n");
			referenceMediaHash[rMHLen-1] = 0;
		}

	} else if (audioFileName) {
		vprintf("\tVerifying against media hash generated from file %s\n", audioFileName);
		referenceMediaHash = uitsAudioGetMediaHash (audioFileName);
		uitsHandleErrorPTR(payloadModuleName, "uitsVerifyPayloadXML", mediaHash,  ERR_HASH,
					   "Error: Couldn't generate media hash value from audio file for validation\n");
	}
	
	/* validate the media hash in the payload XML against the reference media hash */
	err = uitsCompareMediaHash(referenceMediaHash, mediaHash);
	uitsHandleErrorINT(payloadModuleName, " uitsVerifyPayloadXML", err, 0, ERR_HASH,
					   "Error: Invalid media hash\n");	
	
	return (OK);

}

/*
 *
 * Function: uitsCompareMediaHash ()
 * Purpose:	 Compare a reference media hash to a payload media hash
 *               1. Check if they match exactly (SUCCESS)
 *               2. Check if the payload hash is base-64 encoded (SUCCESS with warning)
 *               3. Check if they only differ in case (SUCCESS with warning)
 * Returns:  OK or exit on error
 */

int uitsCompareMediaHash (char *calculatedMediaHashValue, char *mediaHashValue) 
{
	unsigned char *b64CalculatedMediaHashValue;
	char *lowerCaseMediaHashValue;
	int b64HasNewlines;
	int i;
		
	// compare the strings
	err = strcmp(mediaHashValue, calculatedMediaHashValue);
	if (err == OK) {
		return(OK);
	}
	
	// no exact match, see if the media hash is bas64 encoded
	
	if (strchr(mediaHashValue, '\n') == NULL) {	// check for newlines in hash value
		b64HasNewlines = FALSE;
	} else {
		b64HasNewlines = TRUE;
	}		
	
	b64CalculatedMediaHashValue = uitsBase64Encode(calculatedMediaHashValue, strlen(calculatedMediaHashValue), b64HasNewlines);
	
	err = strcmp(mediaHashValue, b64CalculatedMediaHashValue);
	if (err == OK) {
		vprintf ("Warning: Media hash in payload is base 64 encoded\n");
		return(OK);
	}
	
	// not base64, see if the values only differ in case
	
	lowerCaseMediaHashValue = strdup(mediaHashValue);
	for (i=0; lowerCaseMediaHashValue[i] != '\0'; i++) { 
		lowerCaseMediaHashValue[i] = tolower(lowerCaseMediaHashValue[i]);
	}
	
	for (i=0; calculatedMediaHashValue[i] != '\0'; i++) {
		calculatedMediaHashValue[i] =  tolower(calculatedMediaHashValue[i]);
	}
	
	err = strcmp(lowerCaseMediaHashValue, calculatedMediaHashValue);
	if (err == OK) {
		vprintf ("Warning: Media hash in payload has different case than calculated media hash.\n");
		return(OK);
	}
	
	// couldn't match error out
	
	uitsHandleErrorINT(payloadModuleName, "uitsCompareMediaHash", err, OK, ERR_HASH,
					   "Error: Media hash in payload does not match reference media hash\n");
	
	/* The next line will never get executed, but include it to get rid of compiler warning */
	return (err);
}


/* 
 * Function: uitsValidatePayloadSchema
 * Purpose:	 Use libxml2 to validate the xml in the payload schema against the xsd 
 * Returns:  OK or exit on error
 */

int uitsValidatePayloadSchema (mxml_node_t * xmlRootNode) 
{
	xmlDocPtr				doc;
	xmlSchemaPtr			schema = NULL;
	xmlSchemaParserCtxtPtr	ctxt;
	char					*xmlString;
	FILE					*tempFP;
	
	xmlLineNumbersDefault(1);
	
	/* make sure that the xsd file exists */
	tempFP = fopen(XSDFileName, "r");
	uitsHandleErrorPTR(payloadModuleName, "uitsValidatePayloadSchema", tempFP, ERR_FILE, "Error: Could not open xsd file\n");
	fclose(tempFP);
	
	ctxt = xmlSchemaNewParserCtxt(XSDFileName);
	
	if (!silentFlag) {
		xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
	} else {
		xmlSchemaSetParserErrors(ctxt, NULL, NULL, NULL);

	}

	schema = xmlSchemaParse(ctxt);
	xmlSchemaFreeParserCtxt(ctxt);
	//xmlSchemaDump(stdout, schema); //To print schema dump
	
	
	/* Convert the mxml tree data structure to a libxml doc structure */	
	xmlString = uitsMXMLToXMLString(xmlRootNode);
	uitsHandleErrorPTR(payloadModuleName, "uitsValidatePayloadSchema", xmlString, ERR_PAYLOAD,
					"Error: Couldn't generate xml string for validation\n");
	
	doc = xmlReadMemory(xmlString, strlen(xmlString), "noname.xml", NULL, 0);	
	uitsHandleErrorPTR(payloadModuleName, "uitsValidatePayloadSchema", doc,  ERR_PAYLOAD,
					"Error: Couldn't parse xml buffer\n");
	
	ctxt = xmlSchemaNewValidCtxt(schema);
	if (!silentFlag) {
		xmlSchemaSetValidErrors(ctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
	} else {
		xmlSchemaSetValidErrors(ctxt, NULL, NULL, NULL);
	}

	err = xmlSchemaValidateDoc(ctxt, doc);
	uitsHandleErrorINT(payloadModuleName, "uitsValidatePayloadSchema", err, 0, ERR_SCHEMA,
					"Error: Payload XML failed validation\n");
	
	
	xmlSchemaFreeValidCtxt(ctxt);
	xmlFreeDoc(doc);
	// free the resource
	if(schema != NULL)
		xmlSchemaFree(schema);
	
	xmlSchemaCleanupTypes();
	xmlCleanupParser();
	xmlMemoryDump();
	
	
	return (OK);
}

/* 
 * Function: uitsGetElementText
 * Purpose:	 Helper function to get the text value for a named element node
 * Returns:  ptr to the element's text value or exit on error
 *
 */

char *uitsGetElementText (mxml_node_t *xml, char *name) {
	
	mxml_node_t *elementNode;
	mxml_node_t *textNode;
	char *textValue;
	
	// find the named node element in the xml tree. 
	// note that this assumes only ONE node with the requested name.
	elementNode = mxmlFindElement(xml, xml, name, NULL, NULL, MXML_DESCEND);
	uitsHandleErrorPTR(payloadModuleName, "uitsGetElementText", elementNode, ERR_PAYLOAD,
					"Error: Couldn't get find XML element node\n");
	
	// now find the element's text value
	textNode = mxmlWalkNext(elementNode, xml, MXML_DESCEND);
	uitsHandleErrorPTR(payloadModuleName, "uitsGetElementText", elementNode, ERR_PAYLOAD,
					"Error: Couldn't get get text element value\n");
	
	//		vprintf("UITS_get_element_text Got element: %s value: %s\n", name, text_node->value.text.string);
	//		vprintf("UITS_get_element_text Got element: %s value: %s\n", name, text_node->value.opaque);
	
	return (textNode->value.opaque);
	
}

/* 
 * Function: uitsMXMLWhitespaceCB
 * Purpose:	 Adds whitespace (CR/Tab) to XML that is being output to a 
 *              file so that it is more human-readable
 * Returns:  Ptr to whitespace or NULL
 *
 */	

const char * uitsMXMLWhitespaceCB (mxml_node_t *node, int where) {
	
	const char * name;
	
	name = node->value.element.name;
	
	// add a newline and a tab before elements that are NOT the "UITS" or "METADATA" elements
	
	if (strcmp(name, "uits:UITS") && strcmp(name, "metadata") && strcmp(name, "<?xml>")) {
		if (where == MXML_WS_BEFORE_OPEN) {
			return("\n\t");
		} 
	} else { // add a newline before an open a close for "UITS" and "METADATA" elements
		if (where == MXML_WS_BEFORE_OPEN ||
			where == MXML_WS_BEFORE_CLOSE) 
			return ("\n");
	}
	
	// NULL if no whitespace added
	return (NULL);
	
}

/* 
 * Function: uitsGetMetadataString
 * Purpose:	 Get the metadata XML string within a UITS payload string
 * Returns:  Ptr to string
 *
 */	

char * uitsGetMetadataString (char *payloadXMLString) {
	
	char *metadataString;
	char *metadataStart;
	char *metadataEnd;
	int metadataLen;
	
	metadataStart = strstr(payloadXMLString, "<metadata>");
	metadataEnd = strstr(payloadXMLString, "</metadata>");
	metadataEnd += strlen("</metadata>");
	
	metadataLen = metadataEnd - metadataStart;
	
	metadataString = calloc(sizeof (char), metadataLen+1);
	
	metadataString = strncpy(metadataString, metadataStart, metadataLen);
	
	
	return(metadataString);
}	

/* 
 * Function: uitsGetMetadataStringMXML
 * Purpose:	 Get the metadata XML string from an mxml root node
 * Returns:  Ptr to node or exit on error
 *
 */	

char * uitsGetMetadataStringMXML (mxml_node_t * xmlRootNode) {
	
	mxml_node_t * xmlMetadataNode;
	mxml_node_t * UITS_element_next_node;
	char *metadataString;
	
	 xmlMetadataNode = mxmlFindElement( xmlRootNode, 
												  xmlRootNode, 
												 "metadata",
												 NULL,
												 NULL,
												 MXML_DESCEND);
	
	
	// This is a very ugly workaround to handle an anomaly in how mxml works
	// mxml will save a node and it's siblings in it's savestring function
	// to workaround that, make the metadata element's next node be null, temporarily,
	// then restore the next pointer after the print
	
	
	 UITS_element_next_node =  xmlMetadataNode->next;
	 xmlMetadataNode->next = NULL;
	
	
	metadataString = uitsMXMLToXMLString(xmlMetadataNode);

	
	// now restore the next node
	
	 xmlMetadataNode->next =  UITS_element_next_node;
	
	return(metadataString);
	
}

/*
 *
 *  Function: uitsMXMLToXMLString
 *  Purpose:  Convert an mxml structure to it's xml string
 *  Returns:  Pointer to string
 *
 */

char *uitsMXMLToXMLString (mxml_node_t * xmlMetadataNode)
{
	char *xmlString;
	int xmlStrLen;
	
	xmlString = mxmlSaveAllocString(xmlMetadataNode, MXML_NO_CALLBACK);
	
	// we need the string to be exact. no extra whitespace or line feeds are allowed, 
	// otherwise the signature will not be correct. The mxmlsSaveAllocString adds a
	// '\n' to the end of the string that we need to remove.
	
	xmlStrLen = strlen(xmlString);
	
	if (xmlString[xmlStrLen-1] == '\n') {
		xmlString[xmlStrLen-1] = '\0';
	}
	
	return (xmlString);
	
}

/*
 *
 *  Function: uitsSetMetadataValue
 *  Purpose:  Set the value for a UITS metadata parameter.
 *  Returns:  OK or ERROR
 *
 */


int uitsSetMetadataValue (char *name, char *value) 
{
	UITS_element *metadataPtr = uitsMetadataDesc;
	
	// walk the uits_metadata array to find the named element
	while (metadataPtr->name) {
		if (!strcmp(metadataPtr->name, name)) {
			metadataPtr->value = value;
			return(OK);
		}
		metadataPtr++;
	}
	vprintf("Tried to set non-existent metadata value: name '%s' value '%s'\n", name, value);
	return (ERROR);
}


/*
 *
 *  Function: uitsSetMetadataAttributeValue
 *  Purpose:  Set the value for a UITS metadata element attribute passed from the command line.
 *      		 attribute compand-line option names are of the format "elementname_attributename"
 *  Returns:  OK or ERROR
 *
 */

int uitsSetMetadataAttributeValue (char *name, char *value) 
{
	
	UITS_element *metadata_ptr = uitsMetadataDesc;
	UITS_attributes *attribute_ptr;
	char *element_name;
	char *attribute_name;
	char *temp_string;
	
	// unpack the element name and attribute name from the long option name
	temp_string = strdup(name);
	element_name = strtok(temp_string, "_");
	attribute_name = strtok(NULL, "_");
	
	// walk the uits_metadata array to find the named element
	while (metadata_ptr->name) {
		if (!strcmp(metadata_ptr->name, element_name)) {
			attribute_ptr = metadata_ptr->attributes;
			while (attribute_ptr->name) {
				if (!strcmp(attribute_ptr->name, attribute_name)) {
					attribute_ptr->value = value;
					return (OK);
				}
				attribute_ptr++;
			}
		}
		metadata_ptr++;
	}
	vprintf("Tried to set non-existent metadata element attribute value: name '%s' value '%s'\n", name, value);
	return (ERROR);
}

/*
 *  Function: uitsSetSignatureValue
 *  Purpose:  Set the value for a UITS signature parameter
 *  Returns:  OK or ERROR
 *
 */

int uitsSetSignatureParamValue (char *name, char *value) 
{
	// vprintf("uitsSetSignatureParamValue name: %s, value %s\n", name, value);
	
	
	if (strcmp (name, "algorithm") == 0) {
		uitsSignatureDesc->algorithm = value;
	} else if (strcmp (name, "pubKeyFileName") == 0) {
		uitsSignatureDesc->pubKeyFileName = value;
	} else if (strcmp (name, "privateKeyFileName") == 0) {
		uitsSignatureDesc->privateKeyFileName = value;
	} else if (strcmp (name, "b64LFFlag") == 0) {	/* hack: if the b64 flag is present on command line, it's true */
		uitsSignatureDesc->b64LFFlag = TRUE;
	} else if (strcmp (name, "pubKeyID") == 0) {
		vprintf("pubKeyID value = %s\n", value);
		uitsSignatureDesc->pubKeyID = value;
	} else {
		snprintf(errStr, ERRSTR_LEN, "ERROR: invalid signature parameter name=%s\n", name);
		uitsHandleErrorINT(payloadModuleName, "uitsSetSignatureParamValue", ERROR, OK, ERR_VALUE, errStr);
	}

	return (OK);
}

/*
 *  Function: uitsSetIOFileName
 *  Purpose:  Set the value one of the uits payload io files
 *			   possible types in uitsIOFileTypes enum: AUDIO, METADATA, PAYLOAD
 *  Returns:  OK or ERROR
 */
int	 uitsSetIOFileName (int fileType, char *name)
{
	switch (fileType) {	// uitsAction value is set in uitsGetOpt
			
		case AUDIO:
			audioFileName = name;
			break;
			
		case METADATA:
			metadataFileName = name;
			break;
			
		case PAYLOAD:
			payloadFileName = name;
			break;

		case UITS_XSD:
			XSDFileName= name;
			break;

		case MEDIAHASH:
			mediaHashFileName= name;
			break;
			
		case OUTPUT:
			outputFileName= name;
			break;
			
		default:
			snprintf(errStr, ERRSTR_LEN, "Error uitsSetIOFileName: Invalid fileType value=%d\n", fileType);
			uitsHandleErrorINT(payloadModuleName, "uitsSetIOFileName", ERROR, OK, ERR_VALUE, errStr);
			break;
	}
	
	return (OK);
	
}


/*
 *
 *  Function: uitsSetCommandLineParam
 *  Purpose:  Set the value of an integer command line parameter
 */

void uitsSetCommandLineParam (char *paramName, int paramValue) 
{
	UITS_command_line_params *clParamPtr = clParams;
	
	// walk the uits_metadata array to find the named element
	while (clParamPtr->paramName) {
		if (strcmp(clParamPtr->paramName, paramName) == 0) {
			*clParamPtr->paramValue = paramValue;
			return;
		}
		clParamPtr++;
	}
	
	vprintf("Warning: tried to set non-existent parameter: %s value %d\n", paramName, paramValue);
}

/*
 *
 *  Function: uitsSetCLMediaHashValue
 *  Purpose:  Last-minute hack to allow the mediahash value for verification to be
 *            set from the command line. This needs to be rewritten in 2.0.
 */

void uitsSetCLMediaHashValue (char *mediaHashValue) 
{
	clMediaHashValue = mediaHashValue;
}

char *uitsGetUTCTime() 
{
	struct tm *tm_utcTime = NULL;
	time_t t;
	char *utcTime;
	
	t = time(NULL);
	
	/* convert to UTC */
	tm_utcTime = gmtime(&t);
	
	utcTime = calloc(UITS_UTC_TIME_SIZE, 1);
	
	/* format the time */
	strftime(utcTime, UITS_UTC_TIME_SIZE, "%FT%TZ", tm_utcTime);
	
	dprintf("UTC time and date: %s\n", utcTime);	
	
	return	(utcTime);
}
	
// EOF
