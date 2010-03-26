/*
 *  uitsOpenSSL.h
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

#ifndef _uitsopenssl_h_
#  define _uitsopenssl_h_

/*
 * Include necessary headers
 */

#include <strings.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>

typedef struct {
	int length;
	unsigned char *value;
} UITS_digest;


/*
 * Function Declarations
 */

void			uitsOpenSSLInit (void);
	
UITS_digest		*uitsGetPubKeyID (char *pubKeyFileName);

int				uitsValidatePubKeyID (char *pubKeyFileName, char *pubKeyFromPayload);
UITS_digest		*uitsCreateDigest (unsigned char *message, char *digestName);
UITS_digest		*uitsCreateDigestBuffered (FILE *messageFile, int messageLength, char *digestName); 
char			*uitsDigestToString (UITS_digest *uitsDigest);
unsigned char	*uitsCreateSignature (unsigned char *message,  char *privateKeyFileName,  char *digestName, int b64LFFlag);
int				uitsVerifySignature (char *pubKeyFileName,  unsigned char *data, char *b64Sig, char *digestName);
unsigned char	*uitsBase64Encode (unsigned char *message, int messageLength, int b64LFFlag);
UITS_digest		*uitsBase64Decode (unsigned char *message, int messageLength);
#endif
