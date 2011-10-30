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

#define UITS_UTC_TIME_SIZE 128

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
	int  multipleFlag;	/* TRUE = can have multiple instances of this element */
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


int	 uitsSetMetadataValue  (char *name, char *value, UITS_element *metadata_ptr);
int uitsSetMetadataAttributeValue (char *name, char *value, UITS_element *metadata_ptr);
int  uitsSetSignatureParamValue (char *name, char *value);
int	 uitsSetIOFileName (int fileType, char *name);				// set the name of one of the IO files for the payload
																// possible types in uitsIOFileTypes enum
void uitsSetCommandLineParam (char *paramName, int paramValue); 

int uitsVerifyPayloadFile (void); 

void uitsCheckRequiredParams (char *command);

void uitsSetCLMediaHashValue (char *mediaHashValue);
int  uitsCompareMediaHash (char *calculatedMediaHashValue, char *mediaHashValue); 
int  uitsVerifyMediaHash (char *mediaHash);
char *uitsGetUTCTime(void);


#endif

// EOF
