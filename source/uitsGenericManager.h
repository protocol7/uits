/*
 *  uitsGenericManager.h
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 11/4/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _uitsgenericmanager_h_
#  define _uitsgenericmanager_h_

/* 
 * PUBLIC Functions 
 */

int genericIsValidFile (char *inputFileName);

int genericEmbedPayload  (char *inputFileName, 
						  char *outputFileName, 
						  char *uitsPayloadXML,
						  int  numPadBytes);

char *genericExtractPayload (char *inputFileName); 

char *genericGetMediaHash (char *inputFileName);

#endif


