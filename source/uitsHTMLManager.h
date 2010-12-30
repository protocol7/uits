/*
 *  uitsHTMLManager.h
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 11/11/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 */


/*
 * Prevent multiple inclusion...
 */

#ifndef _uitshtmlmanager_h_
#  define _uitshtmlmanager_h_

/* 
 * PUBLIC Functions 
 */

int htmlIsValidFile (char *inputFileName);

int htmlEmbedPayload  (char *inputFileName, 
						  char *outputFileName, 
						  char *uitsPayloadXML,
						  int  numPadBytes);

char *htmlExtractPayload (char *inputFileName); 

char *htmlGetMediaHash (char *inputFileName);

#endif


