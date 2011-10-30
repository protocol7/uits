/*
 *  cmePayloadManager.h
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 12/3/09.
 *  Copyright 2001 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _cmepayloadmanager_h_
#  define _cmepayloadmanager_h_

// CME Payload IO File Types
enum cmeIOFileTypes {
	CME_PAYLOAD,
	CME_XSD,
};


/*
 *  Function Declarations
 */ 

int cmePayloadManagerInit (void);							// initialize data structures, etc

int cmeCreate (void);										// create a UITS payload (standalone file or embedded in audio)
int cmeVerify (void);										// verify a UITS payload (standalone file or embedded in audio)

UITS_element		*cmeGetMetadataDesc(void);


int  cmeSetSignatureParamValue (char *name, char *value);
int	 cmeSetIOFileName (int fileType, char *name);				// set the name of one of the IO files for the payload
void cmeSetCommandLineParam (char *paramName, int paramValue); 

void cmeSetCommandLineParam (char *paramName, int paramValue);
void cmeCheckRequiredParams (char *command);


#endif

// EOF