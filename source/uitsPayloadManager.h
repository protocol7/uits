/*
 *  uitsPayloadManager.h
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 12/3/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _uitspayloadmanager_h_
#  define _uitspayloadmanager_h_

/*
 * UITS metadata element and attribute structure definition
 */ 

typedef struct {
	char *name;
	char *value;
} UITS_attributes;

typedef struct  {
	char *name;
	char *value;
	UITS_attributes *attributes;
} UITS_element;


typedef struct  {
	char *algorithm;
	char *pubKeyFileName;
	char *privateKeyFileName;
	char *pubKeyID;
	int  b64LFFlag;
} UITS_signature_desc;

typedef struct {
	char *paramName;
	int  *paramValue;
} UITS_command_line_params;

// UITS Payload IO File Types
enum uitsIOFileTypes {
	AUDIO,
	METADATA,
	PAYLOAD,
	UITS_XSD,
	MEDIAHASH,
	OUTPUT
};


/*
 *  Function Declarations
 */ 

int uitsPayloadManagerInit (void);							// initialize data structures, etc

int uitsCreate (void);										// create a UITS payload (standalone file or embedded in audio)
int uitsVerify (void);										// verify a UITS payload (standalone file or embedded in audio)
int uitsExtract (void);										// extract a UITS payload from an audio file
int uitsGenKey  (void);										// generate a KeyID from a public key file 
int uitsGenHash (void);										// generate media hash for an audio file

UITS_element		*uitsGetMetadataDesc(void);
UITS_signature_desc *uitsGetSignatureDesc(void);


mxml_node_t *uitsCreatePayloadXML	(void);							// create a mxml tree from the  uitsMetadata array
	
int uitsVerifyPayloadXML (mxml_node_t *xmlRootNode);				// validate a UITS payload stored in a MXML tree

mxml_node_t *uitsPayloadPopulateMetadata (mxml_node_t *xmlRootNode);	// create the metadata node

mxml_node_t *uitsPayloadPopulateSignature (mxml_node_t *xmlRootNode);	// create the signature node	

int uitsValidatePayloadSchema (mxml_node_t * xmlRootNode);			// verify the payload against the xsd schema


char *uitsGetMetadataString (mxml_node_t * xmlRootNode);			// get the metadata XML string from an mxml root node

char *uitsGetElementText	(mxml_node_t *, char *);				// helper function to get the text value for a named element

const char *uitsMXMLWhitespaceCB (mxml_node_t *, int);				// callback for inserting whitespace into XML

char *uitsGetMetadataString (mxml_node_t * xmlRootNode);			// returns ptr to string containing the metadata element XML
char *uitsMXMLToXMLString (mxml_node_t * xmlMetadataNode);			// wrapper to mxml string conversion call that strips new line

int	 uitsSetMetadataValue  (char *name, char *value);
int  uitsSetMetadataAttributeValue (char *name, char *value); 
int  uitsSetSignatureParamValue (char *name, char *value);
int	 uitsSetIOFileName (int fileType, char *name);				// set the name of one of the IO files for the payload
																// possible types in uitsIOFileTypes enum
void uitsSetCommandLineParam (char *paramName, int paramValue); 

int uitsVerifyPayloadFile (void); 

void uitsCheckRequiredParams (char *command);

void uitsSetCLMediaHashValue (char *mediaHashValue);
int uitsCompareMediaHash (char *calculatedMediaHashValue, char *mediaHashValue); 
int  uitsVerifyMediaHash (char *mediaHash);


#endif