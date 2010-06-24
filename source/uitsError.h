/*
 *  uitsError.h
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 6/1/10.
 *  Copyright 2010 UMG. All rights reserved.
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _uitserror_h_
#  define _uitserror_h_

enum {
	ERR_UITS = 128,	// Generic error code
	ERR_FILE,		// Error opening/reading/seeking file
	ERR_VALUE,		// Invalid value
	ERR_PARSE,		// Error parsing command-line option
	ERR_PARAM,		// Missing command line parameter
	ERR_PAYLOAD,	// Error occurred in the payload manager
	ERR_CREATE,		// Error occurred creating payload
	ERR_VERIFY,		// Error occurred verifying payload
	ERR_EXTRACT,	// Error occurred extracting payload
	ERR_EMBED,		// Error occurred embedding payload
	ERR_AUD,		// Error occurred in audio file manager
	ERR_MP4,		// Error occurred in MP4 manager
	ERR_MP3,		// Error occurred in MP3 manager
	ERR_FLAC,		// Error occurred in FLAC manager
	ERR_AIFF,		// Error occurred in AIFF manager
	ERR_WAV,		// Error occurred in WAV file manager
	ERR_SCHEMA,		// Error occurred validating schema against payload XML
	ERR_HASH,		// Error occurred verifying the media hash
	ERR_SIG,		// Error occurred verifying signature
	ERR_SSL			// Error occurred in open ssl manager
};

typedef struct {
	int errCode;
	char *errorMessage;
} UITS_ERROR_MESSAGES;

static long int err = 0;			// error flag

#define ERRSTR_LEN 255
#ifndef ERRSTR
#  define ERRSTR
char errStr[ERRSTR_LEN];
#else
extern char *errStr;				// buffer to hold an error message string
#endif


void uitsHandleErrorINT(char *uitsModuleName,	// name of uitsModule where error occured
						char *functionName,		// name of calling function 
						int returnValue,		// return value to check
						int sucessValue,		// success value
						int uitsErrorCode,	   // uits error code from uitsError enum
						char *errorMessage);	// error message string, if any	

void uitsHandleErrorPTR (char *uitsModuleName,  // name of uitsModule where error occured
						 char *functionName,	// name of calling function 
						 void *returnValue,		// return value to check
						 int uitsErrorCode,	   // uits error code from uitsError enum
						 char *errorMessage);	// error message string, if any	

void uitsListErrorCodes (void);


#endif

// EOF
