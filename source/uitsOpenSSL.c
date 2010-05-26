/*
 *  uitsOpenSSL.c
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 11/24/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 *  This module calls the OpenSSL api to manage the hashing, signature creation and
 *  signature validation for the UITS tool.
 *
 *  The signature hash is created using the following steps: 
 *
 *	D1 = Canonicalize(“<metadata>…</metadata>”) (NOT YET IMPLEMENTED)
 *	D2 = Hash(D1)
 *	D3 = Sign(private_key, D2)
 *	D4 = Base64encode(D3)
 *
 *	OpenSSL command line equivalents for RSA:
 *		1. create a private key and write it to a file
 *			openssl genrsa -out privateRSA2048.pem 2048
 *		2. create a public key for that private key and write it to a file
 *			openssl rsa -in privateRSA2048.pem -pubout >pubRSA2048.pem
 *		3. encrypt and sign payload
 *			openssl dgst -sha256 -out sig.bin -sign privateRSA2048.pem metadata.xml
 *		4. base64 encode the signature
 *			openssl base64 -in sig.bin -out signature.txt
 *    To verify the RSA signature file from the command line:
 *		1. decrypt the signature
 *			openssl enc -d -base64 -in signature.txt >RSAsig.bin
 *		2. very the decrypted signature
 *			openssl dgst -sha256 -verify pubRSA2048.pem -signature RSAsig.bin metadata.xml
 *
 *	OpenSSL commandline equivalents for DSA:
 *		1. create a private DSA key 
 *			openssl dsaparam -out dsaparam.pem 2048
 *			openssl gendsa -out privateDSA2048.pem dsaparam.pem
 *		2. create a public key for that private key and write it to a file
 *			openssl dsa -in privateDSA2048.pem -pubout -out pubDSA2048.pem
 *		3. encrypt and sign payload
 *			openssl dgst -sha224 -out sig.bin -sign privateDSA2048.pem metadata.xml
 *		4. base64 encode the signed payload
 *			openssl base64 -in sig.bin -out signature.txt
 *    To verify the DSA signature file from the command line:
 *		1. decrypt the signature
 *			openssl enc -d -base64 -in signature.txt >DSAsig.bin
 *		2. very the decrypted signature
 *			openssl dgst -sha224 -verify pubDSA2048.pem -signature DSAsig.bin metadata.xml
 *
 */	


#include "uits.h"


char *openSSLmoduleName = "uitsOpenSSL.c";	// Global variable used for error reporting

/* 
 * Function: uitsOpenSSLInit
 * Purpose:  Initialize openSSL operations and structures if necessary
 * Returns:  Nothing
 *
 */

void uitsOpenSSLInit (void) 
{
	OpenSSL_add_all_digests();	
	return;
}


/* 
 * Function: uitsValidatePubKeyID
 * Purpose:  Reads the public key file, creates a SHA1 hash digest of the contents 
 *			 of the file, and compares it to the value in the keyID attribute in the 
 *			 payload. Used to verify that we've got the correct public key for the payload.
 * Returns:  OK or exit on error
 *
 */

int uitsValidatePubKeyID (char *pubKeyFileName, 
						  char *pubKeyFromPayload) 
{					
	unsigned char *pubKeyData;
	UITS_digest   *pubKeyID;
	char		  *pubKeyValue;
	
	// read the public key from the file 
	pubKeyData = uitsReadFile(pubKeyFileName);
	
	// create SHA1 hash of the file data
	pubKeyID = uitsCreateDigest(pubKeyData, "SHA1");	
	
	pubKeyValue = uitsDigestToString(pubKeyID);
		
	err = strcmp(pubKeyFromPayload, pubKeyValue);
	uitsHandleErrorINT(openSSLmoduleName, "uitsValidatePubKeyID", err, 0, 
					"Error: Public Key file does not match keyID in payload\n");

	/* cleanup */
	free(pubKeyData);
	free(pubKeyID);
	free(pubKeyValue);

	return (OK);
}

/* 
 * Function: uitsCreateDigest
 * Purpose:  Create a digest for a memory-resident message
 * Returns:  Pointer to digest structure or exit on error
 *
 */

UITS_digest *uitsCreateDigest (unsigned char *message, 
							   char *digestName) 
{
	EVP_MD_CTX	  *mdctx = calloc(sizeof(EVP_MD_CTX), 1);
	const EVP_MD  *md;
	unsigned char *mdValue = calloc((EVP_MAX_MD_SIZE * sizeof(unsigned char)), 1);
	int			  mdLen;
	int			  messageLength;
	UITS_digest	  *uitsDigest = calloc(sizeof(UITS_digest), 1);
	
	
//	OpenSSL_add_all_digests();	
	md = EVP_get_digestbyname(digestName);
	uitsHandleErrorPTR(openSSLmoduleName, "EVP_DigestInit_ex", md, 
					"Error: Couldn't initialize message digest\n");
	
	
	EVP_MD_CTX_init(mdctx);	
	err = EVP_DigestInit_ex(mdctx, md, NULL);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_DigestInit_ex", err, 1, NULL);
	
	messageLength = strlen(message);
	
	err = EVP_DigestUpdate(mdctx, message, messageLength);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_DigestUpdate",  err, 1, NULL);
	
	err = EVP_DigestFinal_ex(mdctx, mdValue, &mdLen);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_DigestFinal_ex",err, 1, NULL);
	
	EVP_MD_CTX_cleanup(mdctx);
	uitsDigest->length = mdLen;
	uitsDigest->value  = mdValue;
	
//	vprintf("Digest Length is: %d\n", uitsDigest->length);
//	vprintf("Digest is: ");
//	for(i = 0; i < uitsDigest->length; i++) printf("%02x", uitsDigest->value[i]);
//	vprintf("\n");
	
	
	return (uitsDigest);
}

/* 
 * Function: uitsCreateDigestBuffered
 * Purpose:  Create a message disgest using buffered I/O. 
 * Returns:  Pointer to digest structure or exit on error
 *
 */
UITS_digest *uitsCreateDigestBuffered (FILE *messageFile,
									   int   messageLength,
									   char *digestName) 
{
	EVP_MD_CTX	  *mdctx = calloc(sizeof(EVP_MD_CTX), 1);
	const EVP_MD  *md;
	unsigned char *mdValue = calloc((EVP_MAX_MD_SIZE * sizeof(unsigned char)), 1);
	int			  mdLen;
	UITS_digest	  *uitsDigest = calloc(sizeof(UITS_digest), 1);
	
	int			  messageBufferSize = 1024;
	int			  messageBytesLeft = messageLength;
	int			  bytesRead;
	unsigned char *messageBuffer = calloc(messageBufferSize, 1);
	int i;
	
	
	//	OpenSSL_add_all_digests();	
	md = EVP_get_digestbyname(digestName);
	uitsHandleErrorPTR(openSSLmoduleName, "EVP_DigestInit_ex", md, 
					"Error: Couldn't initialize message digest\n");
	
	
	EVP_MD_CTX_init(mdctx);	
	err = EVP_DigestInit_ex(mdctx, md, NULL);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_DigestInit_ex", err, 1, NULL);
		
	
	// read and process the data in the file in 1024K chunks for length of message
	while (messageBytesLeft) {
		messageBufferSize = (messageBytesLeft > messageBufferSize) ? messageBufferSize : messageBytesLeft;
		bytesRead = fread(messageBuffer, 1, messageBufferSize, messageFile);
		if (bytesRead != messageBufferSize) {
			uitsHandleErrorINT(openSSLmoduleName, "uitsCreateDigestBuffered", ERROR, OK, 
							"Incorrect number of bytes read from message file\n");
		}
		err = EVP_DigestUpdate(mdctx, messageBuffer, bytesRead);
		uitsHandleErrorINT(openSSLmoduleName, "EVP_DigestUpdate",  err, 1, NULL);
		messageBytesLeft -= messageBufferSize;
	}
		
	err = EVP_DigestFinal_ex(mdctx, mdValue, &mdLen);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_DigestFinal_ex",err, 1, NULL);
	
	EVP_MD_CTX_cleanup(mdctx);
	uitsDigest->length = mdLen;
	uitsDigest->value  = mdValue;
	
//	vprintf("Digest Length is: %d\n", uitsDigest->length);
//		vprintf("Digest is: ");
//		for(i = 0; i < uitsDigest->length; i++) printf("%02x", uitsDigest->value[i]);
//		vprintf("\n");
	
	
	return (uitsDigest);
}


/* 
 * Function: uitsDigestToString
 * Purpose:  Convert a uitsDigest to a string 
 * Returns:  Pointer to string
 *
 */
 char *uitsDigestToString (UITS_digest *uitsDigest)
{
	char *digestStringValue, *strPtr;
	int digestStringLength;	// lenght of hash + 1 byte for null terminator
	int i;

	digestStringLength = (2 * uitsDigest->length) + 1; // 2 hex bytes per character
	digestStringValue = calloc(digestStringLength, 1);
	strPtr = digestStringValue;
	
	for(i = 0; i < uitsDigest->length; i++) {
		sprintf(strPtr, "%02x", uitsDigest->value[i]);
		strPtr +=2;
	}
	
	//	vprintf("uitsDigestToString: Digest Length is: %d\n", uitsDigest->length);
	//	vprintf("uitsDigestToString Digest is: %s\n", digestStringValue);
	
	return (digestStringValue);
}

/* 
 * Function: uitsCreateSignature
 * Purpose:  Sign a message using a private key 
 * Returns:  A base-64 encoded signature or exit on error
 *
 */

unsigned char *uitsCreateSignature(unsigned char *message, 
								   char *privateKeyFileName, 
								   char *digestName,
								   int b64LFFlag)
{
	
	FILE		  *fp;
	EVP_PKEY	  *evpPrivateKey;
	int			  dataLen;
	unsigned char *sig;
	int			  sigLen;	
	EVP_MD_CTX	  *ctx;
	EVP_MD		  *mdType;
	unsigned char *b64Sig;
	
	/* Read private key */
	
	fp = fopen (privateKeyFileName, "r");
	if (!fp) {
		snprintf(errStr, strlen((char *)errStr), "ERROR: Couldn't open private key file %s\n", privateKeyFileName);
		uitsHandleErrorINT(openSSLmoduleName, "uitsCreateSignature", ERROR, OK, errStr);
	}
	
	evpPrivateKey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
	fclose (fp);
	
	if (!evpPrivateKey) {
		uitsHandleErrorINT(openSSLmoduleName, "uitsCreateSignature", ERROR, OK, "ERROR: Couldn't read private key from file\n");
	}
	
	/* sign the message using either RSA/SHA256 or DSA/SHA224 digest */
	if (!strcmp(digestName, "SHA256")) {
		mdType = EVP_sha256();
	} else if (!strcmp(digestName, "SHA224")) {
		mdType = EVP_sha224();
	} else {
		snprintf(errStr, strlen((char *)errStr), 
				 "ERROR: Couldn't assign digest type for signing. Unrecognized digest name: %s\n", digestName);
		uitsHandleErrorINT(openSSLmoduleName, "uitsCreateSignature", ERROR, OK, errStr);
	}
	
	ctx = EVP_MD_CTX_create();
	
	sigLen = EVP_PKEY_size(evpPrivateKey);
	sig = calloc(sigLen, 1);
	
	err = EVP_SignInit_ex(ctx, mdType, NULL);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_SignInit_ex", err, 1, NULL);
	
	dataLen	= strlen(message);
	err = EVP_SignUpdate(ctx, message, dataLen);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_SignUpdate", err, 1, NULL);
	
	err = EVP_SignFinal(ctx, sig, &sigLen,  evpPrivateKey);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_SignFinal", err, 1, NULL);
	
	
//	fp = fopen ("testsig.bin", "w");
//	if (!fp) {
//		vprintf("ERROR: Couldn't open testsig.bin\n");
//		exit (1);
//	}
//	fwrite(sig, sigLen, 1, fp);
//	fclose(fp);
	
	
	EVP_MD_CTX_destroy(ctx);
	
	b64Sig = uitsBase64Encode(sig, sigLen, b64LFFlag);
	
	return (b64Sig);
	
}

/* 
 * Function: uitsVerifySignature
 * Purpose:  Verify a signature
 *           This will verify that the passed signature was generated using the private key
 *            associated with the public key file for the passed message digest.
 * Returns:  OK or exit on error
 *
 */

int uitsVerifySignature (char			*pubKeyFileName,
						 unsigned char  *data,
						 char			*b64Sig,
						 char			*digestName)
{
//	BIO			*pubKeyBio;
	FILE		*fp;
	EVP_PKEY	*pubKey;
	UITS_digest	*sig;
	int			dataLen;
	const EVP_MD *md;
	EVP_MD_CTX   *ctx;
	int			result;
		
	
	// read the public key from a file
	fp = fopen(pubKeyFileName, "r");	
	uitsHandleErrorPTR(openSSLmoduleName, "EVP_VerifyInit_ex", fp,  
					"Error: Coudln't open public key file\n");

	pubKey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
	
	fclose(fp);

	// this code will read the public key from memory instead of a file
	// get the public key from a file
//	pubKeyData = uitsReadFile(pubKeyFileName);
//	pubKeyBio = BIO_new(BIO_s_mem());
//	BIO_write(pubKeyBio, pubKeyData, strlen(pubKeyData));
//	pubKey = PEM_read_bio_PUBKEY(pubKeyBio, NULL, NULL, NULL);
	
	// decode the signature
	sig = uitsBase64Decode (b64Sig, strlen(b64Sig));
	uitsHandleErrorPTR(openSSLmoduleName, "uitsVerifySignature", sig, 
					"Error decoding Base 64 signature\n");


	ctx = EVP_MD_CTX_create();
	
	md = EVP_get_digestbyname(digestName);
	uitsHandleErrorPTR(openSSLmoduleName, "uitsVerifySignature", md, 
					"Error creating message digest object, unknown name?\n");
	

	err = EVP_VerifyInit_ex(ctx, md, NULL);
	uitsHandleErrorINT(openSSLmoduleName, "EVP_VerifyInit_ex", err, 1, NULL);
	
	//	data = calloc(EVP_MD_size(md), 1);
	//	data_len = fread(data, 1, EVP_MD_size(md), data_file);
	dataLen	= strlen(data);
	
	EVP_VerifyUpdate(ctx, data, dataLen);
	
	result = EVP_VerifyFinal(ctx, sig->value, sig->length, pubKey);
	uitsHandleErrorINT (openSSLmoduleName, "Verify data", result, 1, NULL);
	
	EVP_MD_CTX_destroy(ctx);

	EVP_PKEY_free(pubKey);

	return result;
}

/* 
 * Function: uitsBase64Encode
 * Purpose:  Base 64 encode a message
 * Returns:  pointer to encoded message or exit on error
 *
 */
unsigned char *uitsBase64Encode	(unsigned char *message, 
								 int messageLength,
								 int b64LFFlag)
{
	//	vprintf("Base64Encode message: %s\n", message);
	
	BIO *bmem, *b64;
	BUF_MEM *bptr;
	
	b64 = BIO_new(BIO_f_base64());
	if (!b64LFFlag) {
		vprintf("\tCreating base64 signature with no newlines\n");
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	}
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, message, messageLength);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);
	
	char *buff = (char *)calloc((bptr->length+1), 1);
	
	memcpy(buff, bptr->data, bptr->length);
	
	/* need to null-terminate the string */
	if (b64LFFlag) {
		buff[bptr->length-1] = 0;	/* if newlines are included, length includes the newline, so replace it */

	} else {
		buff[bptr->length] = 0;		/* if no newlines, terminate after last character */
	}
	BIO_free_all(b64);
	
	return (buff);
}

/* 
 * Function: uitsBase64Decode
 * Purpose:  Decode a message that was base64 encoded. Will decode
 *			 messages with and without newlines.
 * Returns:  Pointer to digest structure or exit if error
 *
 */

UITS_digest *uitsBase64Decode (unsigned char *message, 
							   int messageLength)
{
	BIO_METHOD	*bioMethod;
	BIO			*b64, *bmem;
	UITS_digest *decodedMessage = calloc(sizeof(UITS_digest), 1);
	
	char *buffer = (char *)calloc(messageLength, 1); // the decoded buffer will always be smaller than the encoded one?
	memset(buffer, 0, messageLength);
		
	bioMethod = BIO_f_base64();	
	b64 = BIO_new(bioMethod);	

	/* if the message doesn't have newlines, set the appropriate flag for decoding */
	if (strchr(message, '\n') == NULL) {
		vprintf("\tDecoding base64 signature with no newlines\n");
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	} 
	
	bmem = BIO_new_mem_buf(message, messageLength);
	bmem = BIO_push(b64, bmem);
	
	decodedMessage->length = BIO_read(bmem, buffer, messageLength);
	
	BIO_free_all(bmem);
	
//	vprintf("base64decode buffer length: %d\n", decodedMessage->length);
	
	decodedMessage->value = buffer;
	return (decodedMessage);
}


/* 
 * Function: uitsGetPubKeyID
 * Purpose:  Reads the public key file and creates a SHA1 hash digest of the contents of the file
 * Returns:  Ppointer to the public key digest or exit if error
 *
 */

UITS_digest *uitsGetPubKeyID (char *pubKeyFileName) 
{					
	unsigned char *pubKeyData;
	UITS_digest   *pubKeyID;
	// int i;
	
	pubKeyData = uitsReadFile(pubKeyFileName);
	
	pubKeyID = uitsCreateDigest(pubKeyData, "SHA1");	/* create a SHA1 hash of the key file data */
	
	//	vprintf("pubKeyID = %d\n", pubKeyID->length);
	//	vprintf("pubKeyID is: \n");
	//	for(i = 0; i < pubKeyID->length; i++) printf("%02x", pubKeyID->value[i]);
	//	vprintf("\n");
	
	/* cleanup */
	free(pubKeyData);
	
	return (pubKeyID);
}

// EOF

