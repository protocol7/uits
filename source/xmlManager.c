/*
 *  xmlManager.c
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 10/24/11.
 *  
 *  Copyright 2011 Universal Music Group. All rights reserved.
 *
 *  XML management functions for creating UITS or CME payloads
 *  
 *	$Date$
 *	$Revision$
 *
 */

#include "uits.h"

char *xmlManagerFileName = "xmlManager.c";


/*
 *  Function: uitsCreatePayloadXML ()
 *  Purpose:  Creates a mxml tree from data in the  UITS_element structure
 */

mxml_node_t * uitsCreatePayloadXML (int xmlSchemaType, UITS_element *metadataPtr, UITS_signature_desc *uitsSignatureDesc)
{
	
	mxml_node_t *xml;
	mxml_node_t * xmlRootNode;
	mxml_node_t * xmlMetadataNode;
	mxml_node_t * xmlSignatureNode;
	
	// create the XML Root node
	
	xml = mxmlNewXML("1.0");
	xmlRootNode = mxmlNewElement(xml, "uits:UITS");
	mxmlElementSetAttr( xmlRootNode, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	
	
	switch (xmlSchemaType) {	// uitsAction value is set in uitsGetOpt
			
		case UITS_XML:
			mxmlElementSetAttr( xmlRootNode, "xmlns:uits", "http://www.udirector.net/schemas/2009/uits/1.1");
			break;
			
		case CME_XML:
			mxmlElementSetAttr( xmlRootNode, "xmlns:uits", "http://www.udirector.net/schemas/2011/cmeuits/1.2");
			break;
			
		default:
			snprintf(errStr, ERRSTR_LEN, "Error uitsCreatePayloadXML: Invalid xmlSchemaType value=%d\n", xmlSchemaType);
			uitsHandleErrorINT(xmlManagerFileName, "uitsCreatePayloadXML", ERROR, OK, ERR_VALUE, errStr);
			break;
	}
	
	
	
	// create the metadata node
	xmlMetadataNode = uitsPayloadPopulateMetadata (xmlRootNode, metadataPtr);
	
	// create the signature node
	xmlSignatureNode = uitsPayloadPopulateSignature (xmlRootNode, uitsSignatureDesc);
	
	return (xml);
	
}	

/* 
 * Function: uitsPayloadPopulateMetadata ()
 * Purpose:  Creates or updates the metadata element for the UITS payload and add it to the xml root
 * Returns:  pointer to MXML node or exit if error
 */

mxml_node_t * uitsPayloadPopulateMetadata (mxml_node_t * xmlRootNode, UITS_element *metadataPtr) {
	
	mxml_node_t * xmlMetadataNode = NULL;
	mxml_node_t *elementNode = NULL;
	// UITS_element *metadataPtr = uitsMetadataDesc;
	UITS_attributes *attributePtr;
	
	char *elementName;
	char *elementValue;
	char *attributeValue;
	
	
	/* does the metadata node already exist? */
	xmlMetadataNode = mxmlFindElement( xmlRootNode,  xmlRootNode, "metadata", NULL, NULL, MXML_DESCEND);
	
	if (!xmlMetadataNode) {	/* create it */
		xmlMetadataNode = mxmlNewElement( xmlRootNode, "metadata");
	}
	
	// populate the metadata element with values set on command line
	while (metadataPtr->name) {
		// TIME is treated specially. If the user hasn't specified, default to the currrent time
		if ((strcmp(metadataPtr->name, "Time") == 0) && !metadataPtr->value) {
			metadataPtr->value = uitsGetUTCTime();
		}
		
		// only set the node value if there is a value
		if (metadataPtr->value) {
			if (metadataPtr->multipleFlag) { /* multiple values are specified in a comma-delimited list */
				
				/* turn the plural name into a singular name (URLS becomes URL, Extras becomes Extra) */
				elementName = strdup(metadataPtr->name);
				elementName[strlen(elementName)-1] = 0;
				
				// create an element for each name in the comma-delimited list */
				elementValue = strtok(metadataPtr->value, ",");
				while (elementValue) {
					vprintf ("\t %s: %s\n",elementName, elementValue);
					elementNode = mxmlNewElement( xmlMetadataNode, elementName);
					mxmlNewOpaque(elementNode, elementValue);
					elementValue = strtok(NULL, ",");
				}
				
				// now populate the attributes for each new element
				attributePtr = metadataPtr->attributes;
				if (attributePtr) {
					while (attributePtr->name) {
						attributeValue = strtok(attributePtr->value, ",");
						elementNode = mxmlFindElement(xmlRootNode, xmlRootNode, elementName, NULL, NULL, MXML_DESCEND);
						while (attributeValue) {
							vprintf("\t\t %s: %s\n", attributePtr->name, attributeValue);
							mxmlElementSetAttr(elementNode, attributePtr->name, attributeValue);
							elementNode = mxmlWalkNext(elementNode, xmlRootNode, MXML_NO_DESCEND);
							attributeValue = strtok(NULL, ",");
						}
						attributePtr++;
					}
				}
			} else {
				vprintf ("\t %s: %s\n", metadataPtr->name, metadataPtr->value);
				/* create the mxml element node for this value */
				elementNode = mxmlNewElement( xmlMetadataNode, metadataPtr->name);
				mxmlNewOpaque(elementNode, metadataPtr->value);
				
				/* set the element attributes, if any */
				attributePtr = metadataPtr->attributes;
				if (attributePtr) {
					while (attributePtr->name) {
						vprintf("\t\t %s: %s\n", attributePtr->name, attributePtr->value);
						mxmlElementSetAttr(elementNode, attributePtr->name, attributePtr->value);
						attributePtr++;
					}
				}
			}
		}
		metadataPtr++;
	}
	
	return (xmlMetadataNode);
	
}

/* 
 * Function: uitsPayloadPopulateSignature ()
 * Purpose:  Creates the signature element for the UITS payload and add it to the xml root
 * Returns:  Pointer to MXML node or exit on error
 */

mxml_node_t * uitsPayloadPopulateSignature (mxml_node_t * xmlRootNode, UITS_signature_desc *uitsSignatureDesc) 
{
	
	mxml_node_t		* xmlSignatureNode=NULL;
	mxml_node_t		*xmlSignatureOpaque=NULL;
	//	UITS_digest		*pubKeyID;
	unsigned char	*encodedSignature;
	char			*signatureDigestName;
	//	char			*pubKeyValue;
	char			*metadataString;
	//	char			*strPtr;
	//	int				i;
	
	
	/* does the signature node already exist? */
	xmlSignatureNode = mxmlFindElement( xmlRootNode,  xmlRootNode, "signature", NULL, NULL, MXML_DESCEND);
	
	if (!xmlSignatureNode) {	/* create it */
		xmlSignatureNode = mxmlNewElement( xmlRootNode, "signature");
		/* create a place holder signature. it will be replaced at the end of this function */
		mxmlNewOpaque (xmlSignatureNode, "placeholdersignature");
	}
	
	
	// set the algorithm, cannonicalization, and keyID attributes
	mxmlElementSetAttr( xmlSignatureNode, "algorithm", uitsSignatureDesc->algorithm);
	mxmlElementSetAttr( xmlSignatureNode, "canonicalization", "none");
	
	/* disabled for first release */
	/* this code will generate a Public Key ID from the key file */
	/*
	 // extract the public key id from the file and create a SHA1 digest of the key
	 pubKeyID = uitsGetPubKeyID (uitsSignatureDesc->pubKeyFileName);
	 
	 // Convert the key digest to a string
	 
	 pubKeyValue = uitsDigestToString(pubKeyID);
	 mxmlElementSetAttr( xmlSignatureNode, "keyID", pubKeyValue);
	 
	 */
	
	/* for now, just use the public key id passed on the command line */
	mxmlElementSetAttr( xmlSignatureNode, "keyID", uitsSignatureDesc->pubKeyID);
	
	
	// create a signed hash value for the signature value
	
	/* create hash of metadata */
	if (!strcmp(uitsSignatureDesc->algorithm, "RSA2048")) {
		signatureDigestName = "SHA256";
	} else if (!strcmp(uitsSignatureDesc->algorithm, "DSA2048")) {
		signatureDigestName = "SHA224";
	}
	
	/* create signature for metadata using the private key */
	metadataString = uitsGetMetadataStringMXML ( xmlRootNode);
	
	encodedSignature = uitsCreateSignature(metadataString, 
										   uitsSignatureDesc->privateKeyFileName, 
										   signatureDigestName, 
										   uitsSignatureDesc->b64LFFlag);
	
	
	
	xmlSignatureOpaque = mxmlWalkNext(xmlSignatureNode, xmlRootNode, MXML_DESCEND);
	uitsHandleErrorPTR(xmlManagerFileName, "signature", xmlSignatureOpaque, ERR_PARAM, 
					   "Error: Couldn't get get signature value\n");
	
	err = mxmlSetOpaque(xmlSignatureOpaque, encodedSignature);
	uitsHandleErrorINT(xmlManagerFileName, "uitsPayloadPopulateSignature", err, 0, ERR_PARAM, 
					   "Error: Couldn't update signature text value\n");
	
	//	free (metadataString);
	
	return ( xmlSignatureNode);
	
}

/*
 *
 * Function:  uitsVerifyPayloadXML ()
 * Purpose:	 Validate the UITS payload against the UITS wsd schema, verify the mediahash, and verify the signature
 *			 This validation uses the libxml2 library
 * Returns: OK or exit on error
 */

int  uitsVerifyPayloadXML (mxml_node_t * xmlRootNode, 
						   char *payloadXMLString, 
						   char *XSDFileName, 
						   int mediaHashNoVerifyFlag, 
						   UITS_signature_desc *uitsSignatureDesc) 
{
	char		*metadataString;
	char		*signatureString;
	char		*signatureDigestName;
	//	char		*pubKeyId;
	mxml_node_t *signatureElementNode;
	
	char		*mediaHash;
	
	/* validate the xml against the uits.xsd schema */
	vprintf("\tAbout to validate payload XML against schema\n");
	
	err = uitsValidatePayloadSchema (xmlRootNode, XSDFileName);
	uitsHandleErrorINT(xmlManagerFileName, " uitsVerifyPayloadXML", err, 0, ERR_SCHEMA, 
					   "Error: Couldn't verify the schema\n");
	
	vprintf("\tPayload passed schema validation\n");
	
	if (!mediaHashNoVerifyFlag) {	// don't verify media hash if set 
		/* verify that the media hash is correct */
		vprintf("\tAbout to verify media hash in payload XML\n");
		mediaHash = uitsGetElementText(xmlRootNode, "Media");
		uitsHandleErrorPTR(xmlManagerFileName, "uitsVerifyPayloadXML", mediaHash,  ERR_PAYLOAD,
						   "Error: Couldn't get Media hash value from payload XML for validation\n");
		
		err = uitsVerifyMediaHash (mediaHash);
		uitsHandleErrorINT(xmlManagerFileName, " uitsVerifyPayloadXML", err, 0, ERR_HASH,
						   "Error: Couldn't verify the media hash\n");
		
		vprintf("\tMedia hash verified\n");
	}
	
	// To verify the signature we need the metadata element text, the public key file, and the signature
	metadataString	= uitsGetMetadataString ( payloadXMLString);	
	signatureString = uitsGetElementText( xmlRootNode, "signature");
	
	vprintf("metadataString length: %d metadataString: %s\n", strlen(metadataString), metadataString);
	vprintf("signatureString: %s\n", signatureString);
	
	// CMA Note: Disabled until spec is clarified
	// Verify that the public key file is the correct one for this payload
	
	signatureElementNode = mxmlFindElement( xmlRootNode,  xmlRootNode, "signature", NULL, NULL, MXML_DESCEND);
	//	uitsHandleErrorPTR(xmlManagerFileName, " uitsVerifyPayloadXML", signatureElementNode, ERR_SIG,
	//					"Error: Couldn't find XML signature element node\n");
	
	//	pubKeyId = mxmlElementGetAttr(signatureElementNode, "keyID");
	//	uitsHandleErrorPTR(xmlManagerFileName, " uitsVerifyPayloadXML", pubKeyId, ERR_SIG,
	//					"Error: Couldn't get get keyId attribute value\n");
	
	//	err = uitsValidatePubKeyID (uitsSignatureDesc->pubKeyFileName, pubKeyId);
	//	uitsHandleErrorINT(xmlManagerFileName, " uitsVerifyPayloadXML", err, OK, ERR_SIG,
	//					"Error: Public key in file does not match keyID attribute in payload.\n");
	//	
	/* convert the algorithm name to a digest name */
	if (!strcmp(uitsSignatureDesc->algorithm, "RSA2048")) {
		signatureDigestName = "SHA256";
	} else if (!strcmp(uitsSignatureDesc->algorithm, "DSA2048")) {
		signatureDigestName = "SHA224";
	}
	
	vprintf("\tAbout to verify signature with Public Key in file: %s\n", uitsSignatureDesc->pubKeyFileName );
	err = uitsVerifySignature(uitsSignatureDesc->pubKeyFileName, metadataString, signatureString, signatureDigestName);
	
	uitsHandleErrorINT(xmlManagerFileName, " uitsVerifyPayloadXML", err, 1, ERR_SIG,
					   "Error: Couldn't validate signature\n");
	
	vprintf("\tPayload signature verified\n");
	
	return (OK);
	
}


/* 
 * Function: uitsValidatePayloadSchema
 * Purpose:	 Use libxml2 to validate the xml in the payload schema against the xsd 
 * Returns:  OK or exit on error
 */

int uitsValidatePayloadSchema (mxml_node_t * xmlRootNode, char *XSDFileName) 
{
	xmlDocPtr				doc;
	xmlSchemaPtr			schema = NULL;
	xmlSchemaParserCtxtPtr	ctxt;
	char					*xmlString;
	FILE					*tempFP;
	
	xmlLineNumbersDefault(1);
	
	/* make sure that the xsd file exists */
	tempFP = fopen(XSDFileName, "r");
	dprintf("XSDFilename: %s\n", XSDFileName);
	uitsHandleErrorPTR(xmlManagerFileName, "uitsValidatePayloadSchema", tempFP, ERR_FILE, "Error: Could not open xsd file\n");
	fclose(tempFP);
	
	ctxt = xmlSchemaNewParserCtxt(XSDFileName);
	
	if (!silentFlag) {
		xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
	} else {
		xmlSchemaSetParserErrors(ctxt, NULL, NULL, NULL);
		
	}
	
	schema = xmlSchemaParse(ctxt);
	xmlSchemaFreeParserCtxt(ctxt);
	//xmlSchemaDump(stdout, schema); //To print schema dump
	
	
	/* Convert the mxml tree data structure to a libxml doc structure */	
	xmlString = uitsMXMLToXMLString(xmlRootNode);
	uitsHandleErrorPTR(xmlManagerFileName, "uitsValidatePayloadSchema", xmlString, ERR_PAYLOAD,
					   "Error: Couldn't generate xml string for validation\n");
	
	doc = xmlReadMemory(xmlString, strlen(xmlString), "noname.xml", NULL, 0);	
	uitsHandleErrorPTR(xmlManagerFileName, "uitsValidatePayloadSchema", doc,  ERR_PAYLOAD,
					   "Error: Couldn't parse xml buffer\n");
	
	ctxt = xmlSchemaNewValidCtxt(schema);
	if (!silentFlag) {
		xmlSchemaSetValidErrors(ctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
	} else {
		xmlSchemaSetValidErrors(ctxt, NULL, NULL, NULL);
	}
	
	err = xmlSchemaValidateDoc(ctxt, doc);
	uitsHandleErrorINT(xmlManagerFileName, "uitsValidatePayloadSchema", err, 0, ERR_SCHEMA,
					   "Error: Payload XML failed validation\n");
	
	
	xmlSchemaFreeValidCtxt(ctxt);
	xmlFreeDoc(doc);
	// free the resource
	if(schema != NULL)
		xmlSchemaFree(schema);
	
	xmlSchemaCleanupTypes();
	xmlCleanupParser();
	xmlMemoryDump();
	
	
	return (OK);
}

/* 
 * Function: uitsGetElementText
 * Purpose:	 Helper function to get the text value for a named element node
 * Returns:  ptr to the element's text value or exit on error
 *
 */

char *uitsGetElementText (mxml_node_t *xml, char *name) {
	
	mxml_node_t *elementNode;
	mxml_node_t *textNode;
	char *textValue;
	
	// find the named node element in the xml tree. 
	// note that this assumes only ONE node with the requested name.
	elementNode = mxmlFindElement(xml, xml, name, NULL, NULL, MXML_DESCEND);
	uitsHandleErrorPTR(xmlManagerFileName, "uitsGetElementText", elementNode, ERR_PAYLOAD,
					   "Error: Couldn't get find XML element node\n");
	
	// now find the element's text value
	textNode = mxmlWalkNext(elementNode, xml, MXML_DESCEND);
	uitsHandleErrorPTR(xmlManagerFileName, "uitsGetElementText", elementNode, ERR_PAYLOAD,
					   "Error: Couldn't get get text element value\n");
	
	//		vprintf("UITS_get_element_text Got element: %s value: %s\n", name, text_node->value.text.string);
	//		vprintf("UITS_get_element_text Got element: %s value: %s\n", name, text_node->value.opaque);
	
	return (textNode->value.opaque);
	
}

/* 
 * Function: uitsMXMLWhitespaceCB
 * Purpose:	 Adds whitespace (CR/Tab) to XML that is being output to a 
 *              file so that it is more human-readable
 * Returns:  Ptr to whitespace or NULL
 *
 */	

const char * uitsMXMLWhitespaceCB (mxml_node_t *node, int where) {
	
	const char * name;
	
	name = node->value.element.name;
	
	// add a newline and a tab before elements that are NOT the "UITS" or "METADATA" elements
	
	if (strcmp(name, "uits:UITS") && strcmp(name, "metadata") && strcmp(name, "<?xml>")) {
		if (where == MXML_WS_BEFORE_OPEN) {
			return("\n\t");
		} 
	} else { // add a newline before an open a close for "UITS" and "METADATA" elements
		if (where == MXML_WS_BEFORE_OPEN ||
			where == MXML_WS_BEFORE_CLOSE) 
			return ("\n");
	}
	
	// NULL if no whitespace added
	return (NULL);
	
}

/* 
 * Function: uitsGetMetadataString
 * Purpose:	 Get the metadata XML string within a UITS payload string
 * Returns:  Ptr to string
 *
 */	

char * uitsGetMetadataString (char *payloadXMLString) {
	
	char *metadataString;
	char *metadataStart;
	char *metadataEnd;
	int metadataLen;
	
	metadataStart = strstr(payloadXMLString, "<metadata>");
	metadataEnd = strstr(payloadXMLString, "</metadata>");
	metadataEnd += strlen("</metadata>");
	
	metadataLen = metadataEnd - metadataStart;
	
	metadataString = calloc(sizeof (char), metadataLen+1);
	
	metadataString = strncpy(metadataString, metadataStart, metadataLen);
	
	
	return(metadataString);
}	

/* 
 * Function: uitsGetMetadataStringMXML
 * Purpose:	 Get the metadata XML string from an mxml root node
 * Returns:  Ptr to node or exit on error
 *
 */	

char * uitsGetMetadataStringMXML (mxml_node_t * xmlRootNode) {
	
	mxml_node_t * xmlMetadataNode;
	mxml_node_t * UITS_element_next_node;
	char *metadataString;
	
	xmlMetadataNode = mxmlFindElement( xmlRootNode, 
									  xmlRootNode, 
									  "metadata",
									  NULL,
									  NULL,
									  MXML_DESCEND);
	
	
	// This is a very ugly workaround to handle an anomaly in how mxml works
	// mxml will save a node and it's siblings in it's savestring function
	// to workaround that, make the metadata element's next node be null, temporarily,
	// then restore the next pointer after the print
	
	
	UITS_element_next_node =  xmlMetadataNode->next;
	xmlMetadataNode->next = NULL;
	
	
	metadataString = uitsMXMLToXMLString(xmlMetadataNode);
	
	
	// now restore the next node
	
	xmlMetadataNode->next =  UITS_element_next_node;
	
	return(metadataString);
	
}

/*
 *
 *  Function: uitsMXMLToXMLString
 *  Purpose:  Convert an mxml structure to it's xml string
 *  Returns:  Pointer to string
 *
 */

char *uitsMXMLToXMLString (mxml_node_t * xmlMetadataNode)
{
	char *xmlString;
	int xmlStrLen;
	
	xmlString = mxmlSaveAllocString(xmlMetadataNode, MXML_NO_CALLBACK);
	
	// we need the string to be exact. no extra whitespace or line feeds are allowed, 
	// otherwise the signature will not be correct. The mxmlsSaveAllocString adds a
	// '\n' to the end of the string that we need to remove.
	
	xmlStrLen = strlen(xmlString);
	
	if (xmlString[xmlStrLen-1] == '\n') {
		xmlString[xmlStrLen-1] = '\0';
	}
	
	return (xmlString);
	
}