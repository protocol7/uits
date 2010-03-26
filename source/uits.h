/*
 *  uits.h
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 11/24/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _uits_h_
#  define _uits_h_

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#define LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemastypes.h>

#include <mxml.h>

#include "uitsOpenSSL.h"
#include "uitsPayloadManager.h"
#include "uitsAudioFileManager.h"
#include "uitsMP3Manager.h"


#define OK 0
#define ERROR -1

#define	FALSE 0
#define TRUE  1

#define vprintf(...)  if (verboseFlag) {printf (__VA_ARGS__);}
#define dprintf(...)  if (debugFlag)   {printf (__VA_ARGS__);}

static long int err = 0;						// error flag
int verboseFlag;								// verbose message flag set via command line
int debugFlag;									// debug message flag set via command line


void uitsHandleErrorINT(char *uitsModuleName,  // name of uitsModule where error occured
						char *functionName,	// name of calling function 
						int returnValue,		// return value to check
						int sucessValue,		// success value, if isPtrFlag is FALSE
						char *errorMessage);	// error message string, if any	

void uitsHandleErrorPTR (char *uitsModuleName,  // name of uitsModule where error occured
						 char *functionName,	// name of calling function 
						 void *returnValue,		// return value to check
						 char *errorMessage);	// error message string, if any	


unsigned char *uitsReadFile	(char *filename); 

void uitsPrintHelp (char *command); 
int	 uitsInit(void);										// uits initialization housekeeping
int  uitsGetCommand (int argc, const char * argv[]);
int  uitsGetOptCreate   (int argc, const char * argv[]); 
int  uitsGetOptVerify   (int argc, const char * argv[]); 
int  uitsGetOptExtract  (int argc, const char * argv[]);
int  uitsGetOptGenHash  (int argc, const char * argv[]); 
int  uitsGetOptGenKey   (int argc, const char * argv[]); 


#endif