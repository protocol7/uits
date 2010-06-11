/*
 *  uitsError.c
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 6/1/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 */

#include "uits.h"

UITS_ERROR_MESSAGES uitsErrorMessages []= {
	{ ERR_UITS,		"(ERR_UITS)    UITS Error\n" },
	{ ERR_FILE,		"(ERR_FILE)    Error opening/reading/seeking file\n" },
	{ ERR_VALUE,	"(ERR_VALUE)   Invalid value\n" },
	{ ERR_PARSE,	"(ERR_PARSE)   Error parsing command-line option\n" },
	{ ERR_PARAM,	"(ERR_PARAM)   Missing command line parameter\n" },
	{ ERR_PAYLOAD,	"(ERR_PAYLOAD) Error occurred in the payload manager\n" },
	{ ERR_CREATE,	"(ERR_CREATE)  Error occurred creating payload\n"},
	{ ERR_VERIFY,	"(ERR_VERIFY)  Error occurred verifying payload\n" },
	{ ERR_EXTRACT,	"(ERR_EXTRACT) Error occurred extracting payload\n" },
	{ ERR_EMBED,	"(ERR_EMBED)   Error occurred embedding payload\n" },
	{ ERR_AUD,		"(ERR_AUD)     Error occurred in audio file manager\n" },
	{ ERR_MP4,		"(ERR_MP4)     Error occurred in MP4 manager\n" },
	{ ERR_MP3,		"(ERR_MP3)     Error occurred in MP3 manager\n" },
	{ ERR_FLAC,		"(ERR_FLAC)    Error occurred in FLAC manager\n" },
	{ ERR_AIFF,		"(ERR_AIFF)    Error occurred in AIFF manager\n" },
	{ ERR_WAV,		"(ERR_WAV)     Error occurred in WAV file manager\n" },
	{ ERR_SCHEMA,	"(ERR_SCHEMA)  Error occurred validating schema against payload XML\n" },
	{ ERR_HASH,		"(ERR_HASH)    Error occurred verifying the media hash\n" },
	{ ERR_SIG,		"(ERR_SIG)     Error occurred verifying signature\n" },
	{ ERR_SSL,		"(ERR_SSL)     Error occurred in open ssl manager\n" },
	{ 0, NULL}	
};



/*
 * Function: uitsHandleErrorINT
 * Purpose:  Generic error handling for functions that return INT. 
 *           Checks return value against success value. If equal, no error. If 
 *           not equal, print error message and exit.
 * Returns:  Nothing if no error. Exits if error.
 *
 */
void uitsHandleErrorINT(char *uitsModuleName,  // name of uitsModule where error occured
						char *functionName,    // name of calling function 
						int returnValue,       // return value to check
						int sucessValue,       // success value
						int uitsErrorCode,	   // uits error code from uitsError enum
						char *errorMessage)    // error message string, if any	
{
	int gotError = FALSE;	// clear the global error flag
	
	if ((int) returnValue != sucessValue) {
		gotError = TRUE;
		if (errorMessage) {
			fprintf(stderr, "%s", errorMessage);
		}
		fprintf(stderr, "Error: Return value of %s in %s was %d should have been %d\n", 
				functionName, 
				uitsModuleName,
				returnValue,
				sucessValue);
	}
	
	// Some modules have additional error messages. Print them if relevant.
	
	if (gotError) {
		if (!strcmp(uitsModuleName, "uitsOpenSSL.c")) {
			fprintf(stderr, "OpenSSL error messages:\n");
			ERR_print_errors_fp(stderr);
			exit(ERR_get_error());
		}
		exit(uitsErrorCode);
	}
	
	return;
}

/*
 * Function: uitsHandleErrorPTR
 * Purpose:  Generic error handling for functions that return pointers. 
 *           Checks return value against NULL. If non-NULL, no error. If 
 *           NULL, print error message and exit.
 * Returns:  Nothing if no error. Exits if error.
 *
 */

void uitsHandleErrorPTR (char *uitsModuleName,      // name of uitsModule where error occured
						 char *functionName,	    // name of calling function 
						 void *returnValue,			// return value to check
						 int uitsErrorCode,			// uits error code from uitsError enum
						 char *errorMessage)	    // error message string, if any	
{
	int gotError = FALSE;	// clear the global error flag
	
	if (!returnValue) {
		gotError = TRUE;
		if (errorMessage) {
			fprintf(stderr, "%s", errorMessage);
		}
		fprintf(stderr, "Error: Return value of %s in %s was NULL\n", functionName, uitsModuleName);
	}		
	
	// Some modules have additional error messages. Print them if relevant.
	
	if (gotError) {
		if (!strcmp(uitsModuleName, "uitsOpenSSL.c")) {
			fprintf(stderr, "OpenSSL error messages:\n");
			ERR_print_errors_fp(stderr);
			exit(ERR_get_error());
		}
		exit(uitsErrorCode);
	}
	
	return;
}

void uitsListErrorCodes (void) {
	
	UITS_ERROR_MESSAGES *currMessage = uitsErrorMessages;
	
	fprintf(stderr, "UITS exit error codes and messages\n");
	fprintf(stderr, "Run UITS tool in VERBOSE mode (-v) for more error information\n");
	while (currMessage->errCode) {
		fprintf(stderr, "Error code: %d: \t %s", currMessage->errCode, currMessage->errorMessage);
		currMessage++;
	}
}

