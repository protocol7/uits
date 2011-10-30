/*
 *  xmlManager.h
 *  uits-UITS_Tool-xcode
 *
 *  Created by Chris Angelli on 12/3/09.
 *  Copyright 2011 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef _xmlmanager_h_
#  define _xmlmanager_h_

// Supported payload types
enum xmlSchemaType {
	UITS_XML,
	CME_XML,
};



mxml_node_t * uitsCreatePayloadXML (int xmlSchemaType, UITS_element *metadataPtr, UITS_signature_desc *uitsSignatureDesc);

int  uitsVerifyPayloadXML (mxml_node_t * xmlRootNode, 
						   char *payloadXMLString, 
						   char *XSDFileName, 
						   int mediaHashNoVerifyFlag, 
						   UITS_signature_desc *uitsSignatureDesc);

mxml_node_t *uitsPayloadPopulateMetadata (mxml_node_t *xmlRootNode, UITS_element *metadataPtr);	// create the metadata node

mxml_node_t *uitsPayloadPopulateSignature (mxml_node_t *xmlRootNode, UITS_signature_desc *uitsSignatureDesc);	// create the signature node	

int uitsValidatePayloadSchema (mxml_node_t * xmlRootNode, char *XSDFileName);	// verify the payload against the xsd schema


char *uitsGetMetadataStringMXML (mxml_node_t * xmlRootNode);			// get the metadata XML string from an mxml root node

char *uitsGetElementText	(mxml_node_t *, char *);				// helper function to get the text value for a named element

const char *uitsMXMLWhitespaceCB (mxml_node_t *, int);				// callback for inserting whitespace into XML

char *uitsGetMetadataString (char *payloadXMLString);
char *uitsGetMetadataStringMXML (mxml_node_t * xmlRootNode);			// returns ptr to string containing the metadata element XML
char *uitsMXMLToXMLString (mxml_node_t * xmlMetadataNode);			// wrapper to mxml string conversion call that strips new line


#endif