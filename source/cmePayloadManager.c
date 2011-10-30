/*
 *  cmeFileManager.c
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 10/16/11.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  This file based on uitsPayloadManager, with modifications to conform to CME spec.
 *  From CME Spec:
 *
 *  The Unique Identifier Technology Solution (UITS) is an industry-backed specification that 
 *  describes a common way to provide metadata along side or embedded into unprotected media files 
 *  such that metadata tampering can be detected. 
 *
 *  Standard UITS tokens are associated with individual media files such as audio tracks.  
 *  The Connected Media Experience standards body (CME) has adopted a variant of UITS as a digital 
 *  proof-of-purchase within CME Packages.  The Connected Media Experience variant of UITS (hereafter, 
 *  CME-UITS) has a number of important differences relative to the v1.1 UITS standard.  
 *  Specifically:
 *          * CME-UITS is used at the Package level rather than for specific media files
 *          * CME-UITS does not include an asset identifier
 *          * CME-UITS does not include a media hash 
 *          * CME-UITS does not include the user ID element
 *          * The <product> element in CME-UITS does not include a “completed” attribute
 *          * The one keyID option has been deprecated vs the original UITS specification
 *          * The CME-UITS has a different XML namespace
 *          * The CME-UITS is placed in its own file at the root of a CME Package as described in the CME specifications.
 *  
 *	$Date: 2011-09-24 09:32:54 -0700 (Sat, 24 Sep 2011) $
 *	$Revision: 89 $
 *
 */

#include "uits.h"

char *cmePayloadModuleName = "cmeFileManager.c";


// CMA: these are all command-line parameters. this needs be cleaned up in version 2.0

char *payloadFileName;				// CME UITS payload file name 
UITS_signature_desc *cmeSignatureDesc;
char *cmeXSDFileName = "cme-uits.xsd";				// CME scema description 

int	 verifyFlag;					// set if extracted payload should be verified
int  gpMediaHashFlag;				// genparam: Media_Hash
int  gpB64MediaHashFlag;			// genparam: Base64 Media_Hash
int  gpPubKeyIDFlag;				// genparam: Public Key ID 


UITS_command_line_params cmeParams [] = {
	{"verify",         &verifyFlag},
	{"b64_media_hash", &gpB64MediaHashFlag},
	{"public_key_ID",  &gpPubKeyIDFlag},
	
	{0,	0}	// end of list	
};



/*
 *  The following data structures describe the CME payload element and attribute data:
 */

#define MAX_NUM_ATTRIBUTES 5
#define MAX_NUM_ELEMENTS   MAX_COMMAND_LINE_OPTIONS

UITS_attributes cme_product_ID_attributes [] = {
	{"type",		"UPC"},
	{0, 0}	// end of list
};

UITS_attributes cme_TID_attributes [] = {
	{"version",		"1"},
	{0, 0}	// end of list
};

UITS_attributes cme_URL_attributes [] = {
	{"type",		"WPUB"},
	{0, 0}	// end of list
};

UITS_attributes cme_copyright_attributes [] = {
	{"value",		"allrightsreserved"},
	{0, 0}	// end of list
};


UITS_attributes cme_extra_attributes [] = {
	{"type",		NULL},
	{0, 0}	// end of list
};



UITS_element cmeMetadataDesc [MAX_NUM_ELEMENTS] = {
	{"nonce",		NULL, FALSE, NULL},
	{"Distributor", NULL, FALSE, NULL},
	{"Time",		NULL, FALSE, NULL},
	{"ProductID",	NULL, FALSE, cme_product_ID_attributes},
	{"TID",			NULL, FALSE, cme_TID_attributes},
	{"URL",			NULL, FALSE, cme_URL_attributes},
	{"URLS",		NULL, TRUE,  cme_URL_attributes},
	{"PA",			NULL, FALSE, NULL},
	{"Copyright",	NULL, FALSE, cme_copyright_attributes},
	{"Extra",		NULL, FALSE, cme_extra_attributes},
	{"Extras",		NULL, TRUE,  cme_extra_attributes},
	{0,	0, 0, 0}	// end of list	
};



/* 
 * Function: cmeFileManagerInit
 * Purpose:  Initialize CME operations and structures if necessary
 * Returns:  OK
 *
 */

int cmePayloadManagerInit (void) 
{
	// initialize the signature description
	cmeSignatureDesc =  calloc(sizeof(UITS_signature_desc), 1);
	cmeSignatureDesc->b64LFFlag		  = FALSE;	// add LF in b64 signature 
	cmeSignatureDesc->algorithm          = "RSA2048";
	cmeSignatureDesc->pubKeyFileName     = NULL;	// name of the file containing the private key for signing
	cmeSignatureDesc->privateKeyFileName = NULL;	// name of the file containing the public key for signature verification
	cmeSignatureDesc->pubKeyID			  = NULL;	// for now, public key ID must be passed on command line
	payloadFileName		= NULL;
	verifyFlag			= FALSE;
	gpMediaHashFlag     = FALSE;			// genparam: Media_Hash
	gpB64MediaHashFlag  = FALSE;			// genparam: Base64 Media_Hash
	gpPubKeyIDFlag      = FALSE;			// genparam: Public Key ID 

	return (OK);
}


/* 
 *  Function: cmeCreate ()
 *  Purpose:  Creates a CME payload 
 * 			The payload is written to a file as a standalone payload 
 
 *  Returns:  OK or exit on error
 */

int cmeCreate () 
{
	
	mxml_node_t *xml = NULL;
	FILE		*payloadFP;
	char		*payloadXMLString;
	
	vprintf("Create CME payload ...\n");
	
	/* make sure that all required parameters are non-null */
	cmeCheckRequiredParams("create");
	
	vprintf("Creating CME payload from comand-line options ... \n");		
	
	/* Set the element value in the uits metadata array */
	
	/* create the XML in an mxml data structure from the metadata array */
	
	xml = uitsCreatePayloadXML (CME_XML, cmeMetadataDesc, cmeSignatureDesc);
	uitsHandleErrorPTR(cmePayloadModuleName, "cmeCreate", xml, ERR_PAYLOAD,
					   "Error: Couldn't create XML payload\n");
	
	/* convert the mxml data structure into a string containing the XML */
	payloadXMLString = uitsMXMLToXMLString(xml);
	
	
	// validate the xml payload that was created
	vprintf("Validating payload ...\n");
	
	err =  uitsVerifyPayloadXML (xml, payloadXMLString, cmeXSDFileName, TRUE, cmeSignatureDesc);
	uitsHandleErrorINT(cmePayloadModuleName, "cmeCreate", err, OK, ERR_PAYLOAD, 
					   "Error: Couldn't validate XML payload\n");
	
	//	mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK); // no whitespace
	
	
	vprintf("Writing standalone CME payload to file: %s ...\n", payloadFileName);
	
	payloadFP = fopen(payloadFileName, "wb");
	uitsHandleErrorPTR(cmePayloadModuleName, "cmeCreate", payloadFP, ERR_FILE,
					   "Error: Couldn't open payload file\n");
	
	err = mxmlSaveFile(xml, payloadFP, MXML_NO_CALLBACK);
	uitsHandleErrorINT(cmePayloadModuleName, "cmeCreate", err, 0, ERR_FILE,
					   "Error: Couldn't open save xml to file\n");
	
	fclose(payloadFP);
	
	vprintf("Success\n");
	return (OK);
	
}


/*
 * Function: cmeVerify()
 * Purpose:	 Verify the CME payload 
 * Returns:  OK or exit on error
 */

int cmeVerify (void) 
{
	char *payloadXMLString;
	mxml_node_t *xml;
	
	vprintf("Verify CME payload ...\n");
	
	/* make sure that all required parameters are non-null */
	cmeCheckRequiredParams("verify");
	
	/* initialize the payload xml by reading it from a standalone payload 	 */
	
	vprintf("About to verify CME payload in file %s ...\n", payloadFileName);
	
	payloadXMLString = uitsReadFile(payloadFileName);
	
	/*	convert the XML string to an mxml tree */
	xml = mxmlLoadString (NULL, payloadXMLString, MXML_OPAQUE_CALLBACK);
	uitsHandleErrorPTR(cmePayloadModuleName, "cmeVerify", xml, ERR_PAYLOAD,
					   "Couldn't convert payload XML to  xml tree\n");
	
	err =  uitsVerifyPayloadXML (xml, payloadXMLString, cmeXSDFileName, TRUE, cmeSignatureDesc);
	uitsHandleErrorINT(cmePayloadModuleName, "cmeVerifyPayloadFile", err, 0, ERR_VERIFY,
					   "Error: Payload failed validation\n");
	
	
	vprintf("Payload verified!\n");
	
	return (OK);
}

/*
 * Function: cmeCheckRequiredParams 
 * Purpose:	 Make sure all of the required parameters are set for create or verify option
 *
 */
void cmeCheckRequiredParams (char *command)
{
	/* This is an ugly, brute-force method of checking, but time is short */
	
	
	vprintf("cmeCheckRequiredParams command: %s\n", command);
	
	
	if (strcmp(command, "create") == 0) {
		
		if (!payloadFileName) {
			snprintf(errStr, ERRSTR_LEN, "Error: Can't %s CME payload. No payload file specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!cmeSignatureDesc->algorithm) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s CME payload. No algorithm specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!cmeSignatureDesc->pubKeyFileName) {
			err = snprintf(errStr, ERRSTR_LEN, 
						   "Error: Can't %s CME payload. No public key file specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!cmeSignatureDesc->privateKeyFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s CME payload. No private key file specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!cmeSignatureDesc->pubKeyID) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s CME payload. No public key ID specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
	}
	
	if (strcmp(command, "verify") == 0) {
		if (!payloadFileName) {
			snprintf(errStr, ERRSTR_LEN, "Error: Can't %s CME payload. No payload file specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!cmeSignatureDesc->algorithm) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s CME payload. No algorithm specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
		if (!cmeSignatureDesc->pubKeyFileName) {
			snprintf(errStr, ERRSTR_LEN, 
					 "Error: Can't %s CME payload. No public key file specified.\n", command);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeCheckRequiredParams", ERROR, OK, ERR_PARAM, errStr);
		}
	}
	
}
/*
 *  Function: cmeSetSignatureParamValue
 *  Purpose:  Set the value for a UITS signature parameter
 *  Returns:  OK or ERROR
 *
 */

int cmeSetSignatureParamValue (char *name, char *value) 
{
	// vprintf("uitsSetSignatureParamValue name: %s, value %s\n", name, value);
	
	
	if (strcmp (name, "algorithm") == 0) {
		cmeSignatureDesc->algorithm = value;
	} else if (strcmp (name, "pubKeyFileName") == 0) {
		cmeSignatureDesc->pubKeyFileName = value;
	} else if (strcmp (name, "privateKeyFileName") == 0) {
		cmeSignatureDesc->privateKeyFileName = value;
	} else if (strcmp (name, "b64LFFlag") == 0) {	/* hack: if the b64 flag is present on command line, it's true */
		cmeSignatureDesc->b64LFFlag = TRUE;
	} else if (strcmp (name, "pubKeyID") == 0) {
		vprintf("pubKeyID value = %s\n", value);
		cmeSignatureDesc->pubKeyID = value;
	} else {
		snprintf(errStr, ERRSTR_LEN, "ERROR: invalid signature parameter name=%s\n", name);
		uitsHandleErrorINT(cmePayloadModuleName, "cmeSetSignatureParamValue", ERROR, OK, ERR_VALUE, errStr);
	}
	
	return (OK);
}



/*
 *  Function: cmeSetIOFileName
 *  Purpose:  Set the value one of the uits payload io files
 *			   possible types in cmeIOFileTypes enum: PAYLOAD, CME_XSD
 *  Returns:  OK or ERROR
 */
int	 cmeSetIOFileName (int fileType, char *name)
{
	switch (fileType) {	// uitsAction value is set in uitsGetOpt
			
		case CME_PAYLOAD:
			payloadFileName = name;
			break;
			
		case CME_XSD:
			cmeXSDFileName= name;
			break;
			
		default:
			snprintf(errStr, ERRSTR_LEN, "Error cmeSetIOFileName: Invalid fileType value=%d\n", fileType);
			uitsHandleErrorINT(cmePayloadModuleName, "cmeSetIOFileName", ERROR, OK, ERR_VALUE, errStr);
			break;
	}
	
	return (OK);
	
}


/*
 *
 *  Function: cmeSetCommandLineParam
 *  Purpose:  Set the value of an integer command line parameter
 */

void cmeSetCommandLineParam (char *paramName, int paramValue) 
{
	UITS_command_line_params *clParamPtr = cmeParams;
	
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
 *  Function: cmeGetMetadataDesc
 *  Purpose: Returns pointer to metadata element description
 *
 */

UITS_element *cmeGetMetadataDesc(void)
{
	return (cmeMetadataDesc);
}

// EOF
