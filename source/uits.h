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
#include <time.h>

#include <getopt.h>

#define LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemastypes.h>

#include <mxml.h>

#include "uitsError.h"
#include "uitsOpenSSL.h"
#include "uitsPayloadManager.h"
#include "uitsAudioFileManager.h"
#include "uitsMP3Manager.h"
#include "uitsMP4Manager.h"
#include "uitsFLACManager.h"
#include "uitsAIFFManager.h"
#include "uitsWAVManager.h"


#define OK 0
#define ERROR -1

#define	FALSE 0
#define TRUE  1

#define MAX_COMMAND_LINE_OPTIONS 50

#define vprintf(...)  if (!silentFlag && verboseFlag) {printf (__VA_ARGS__);}
#define dprintf(...)  if (!silentFlag && debugFlag)   {printf (__VA_ARGS__);}

int silentFlag ;					// silent mode (overrides verbose if both are specified)
int verboseFlag;					// verbose message flag (DEFAULT mode is verbose)
int displayErrorCodes;				// display error messages and codes
int debugFlag;						// debug message flag set via command line


unsigned char *uitsReadFile		(char *filename); 
int			  uitsGetFileSize	(FILE *fp);

void uitsPrintHelp (char *command); 
int	 uitsInit(void);										// uits initialization housekeeping
int  uitsGetCommand (int argc, const char * argv[]);
int  uitsGetOptCreate   (int argc, const char * argv[]); 
int  uitsGetOptVerify   (int argc, const char * argv[]); 
int  uitsGetOptExtract  (int argc, const char * argv[]);
int  uitsGetOptGenHash  (int argc, const char * argv[]); 
int  uitsGetOptGenKey   (int argc, const char * argv[]); 


#endif

// EOF
