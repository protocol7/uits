/*
 *  UMG Unique Identifier Technology Solution (UITS) Tool
 *  
 *  Command-line tool for creating, verifying and inserting UITS payloads for 
 *  MP3 audio files.
 * 
 *  Created by Chris Angelli on 11/24/09.
 *  Copyright 2009 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 */

#include "uits.h"

char *moduleName = "main.c";


enum uitsActions {
	CREATE,
	VERIFY,
	EXTRACT,
	HASH,
	KEY,
	HELP,
	ERRORS,
};


int main (int argc, const char * argv[]) {	
	
	int uitsAction = 0;				// UITS action to perform (CREATE, VERIFY, EXTRACT, etc)

//	uitsInit();						// standard initializations 								
	
	uitsAction = uitsGetCommand (argc, argv);
	uitsHandleErrorINT(moduleName, "main", err, OK, ERR_PARSE,
					"Error: Couldn't parse command-line options\n");
	
	switch (uitsAction) {	// uitsAction value is set in uitsGetOpt
			
		case CREATE:
			dprintf ("Create UITS payload\n");
			err = uitsGetOptCreate(argc, argv);		// parse the command-line options 
			fflush(stdout);
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_PARSE,
							"Error: Couldn't parse command line options for create\n");
			err = uitsCreate();
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_CREATE,
							"Error: Couldn't create UITS payload\n");
			break;

		case VERIFY:
			dprintf ("Verify UITS payload\n");
			err = uitsGetOptVerify(argc, argv);		// parse the command-line options 
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_PARSE,
							"Error: Couldn't parse command line options for verify\n");
			err = uitsVerify();
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_VERIFY,
							"Error: Couldn't verify UITS payload\n");
			break;

		case EXTRACT:
			dprintf ("Extract UITS payload\n");
			err = uitsGetOptExtract(argc, argv);		// parse the command-line options
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_PARSE,
							"Error: Couldn't parse command line options for extract\n");
			err = uitsExtract();
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_EXTRACT,
							"Error: Couldn't extract UITS payload\n");
			break;

		case HASH:
			dprintf ("Generate media hash parameters\n");
			err = uitsGetOptGenHash(argc, argv);		// parse the command-line options 
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_PARSE,
							"Error: Couldn't parse command line options for hash\n");
			err = uitsGenHash();
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_UITS,
							"Error: Couldn't generate hash parameter\n");
			break;

		case KEY:
			dprintf ("Generate media hash parameters\n");
			err = uitsGetOptGenKey(argc, argv);		// parse the command-line options 
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_PARSE,
							   "Error: Couldn't parse command line options for hash\n");
			err = uitsGenKey();
			uitsHandleErrorINT(moduleName, "main", err, OK, ERR_UITS,
							   "Error: Couldn't generate key parameter\n");
			break;
			
		case ERRORS:
			uitsListErrorCodes();
			break;
			
		default:
			uitsHandleErrorINT(moduleName, "main", ERROR, OK, ERR_VALUE, "Error: Invalid action value\n");
			break;
	}
	
	
	exit (OK);
}

/*
 * Function: uitsPrintHelp ()
 * Purpose:  Print help and exit
 * Passed:   Command for printing detailed help
 * Returns:  Nothing
 */

void uitsPrintHelp (char *command) 
{
	char *version = "$Revision$";
		
	if (strcmp(command, "version") == 0) {
		printf("UITS_Tool version : %s\n", version);
	} else if (strcmp(command, "hash") == 0) {
		printf("\n");
		printf("Usage: uits_tool hash [options]\n");
		printf("--verbose   (-v)\n");
		printf("--audio     (-a)   [file-name] (REQUIRED): Audio file for which to generate media hash\n");
		printf("--b64       (-c)               (OPTIONAL): Base-64 encode the media hash (DEFAULT is hex)\n");
		printf("--output    (-o)   [file-name] (OPTIONAL): Output file to write the hash to (DEFAULT is stdout)\n");

	} else if (strcmp(command, "key") == 0) {
		printf("Usage: uits_tool key [options]\n");
		printf("\n");
		printf("--verbose  (-v)\n");
		printf("--pub      (-b)   [file-name] (REQUIRED ): Name of the file containing the public key\n");
		printf("--output   (-o)   [file-name] (OPTIONAL): Output file to write the hash to (DEFAULT is stdout)\n");
	} else if(strcmp(command, "create") == 0) {
		printf("Usage: uits_tool create [options]\n");
		printf("\n");
		printf("--verbose   (-v)                            Run in verbose mode (DEFAULT)\n");
		printf("--silent    (-s)                            Run in silent mode \n");
		printf("--audio     (-a)    [file-name] (REQUIRED): Name of the audio file for which to create payload\n");
		printf("--uits      (-u)    [file-name] (REQUIRED): Name of UITS payload file\n");
		printf("--embed     (-e)                (OPTIONAL): Embed UITS payload into the audio and write to payload file\n");
		printf("--algorithm (-r)    [name]      (OPTIONAL): Name of the algorithm to use for signing. \n");
		printf("                                            Possible values: RSA2048 (DEFAULT)\n"); 
		printf("                                                             DSA2048\n");
		printf("--pub       (-b)    [file-name] (REQUIRED): Name of the file containing the public key\n");
		printf("--priv      (-i)    [file-name] (REQUIRED): Name of the file contianing the private key for signing\n");
		printf("--pubID     (-k)    [value]     (REQUIRED): The SHA1 hash of the public key needed to \n");
		printf("                                            validate the signature\n");
//		printf("--metadata_file     [file-name] (OPTIONAL): Name of a file containing the metadtata. This file is an XML\n");
//		printf("                                            file containing a valid UITS payload. The Media hash, KeyID,\n");
//		printf("                                            and signature in the metadata_file will be re-calculated.\n");
		printf("--pad       (-d)    [num bytes] (OPTIONAL): If UITS payload is being embedded into an MP3 audio file,\n"); 
		printf("                                            add [num bytes] pad bytes to ID3 tag.\n");
		printf("--xsd       (-x)    [file-name] (OPTIONAL): Name of the schema to use for validation\n"); 
		printf("                                            DEFAULT is uits.xsd in current directory\n");
		printf("--ml        (-m)    [file-name] (OPTIONAL): Store the base 64 encoded signature in multiple lines\n"); 
		printf("                                            DEFAULT is a single line for signature\n");
		printf("--b64       (-c)                (OPTIONAL): Base 64 encode the media hash. Media hash is hex by default\n"); 
		printf("--hash      (-h)    [hash-value](OPTIONAL): Use the passed hash value instead of calculating from audio frames\n"); 
		printf("\n");
		printf("The following parameters are UITS metadata. All values are treated as text. \n");
		printf("--nonce                 [value] (REQUIRED)\n");
		printf("--Distributor           [value] (REQUIRED)\n");
		printf("--Time                  [value] (OPTIONAL) If no Time value is provided, Time will be set to current UTC time.\n");
		printf("--ProductID             [value] (REQUIRED)\n");
		printf("--ProductID_type        [value] (OPTIONAL) Possible values: UPC (DEFAULT)\n");
		printf("                                                            GRID\n");
		printf("--ProductID_completed  [value] (OPTIONAL) Possible values: false (DEFAULT)\n");
		printf("                                                            true\n");
		printf("--AssetID               [value] (REQUIRED)\n");
		printf("--AssetID_type          [value] (OPTIONAL) Possible values: ISRC (DEFAULT)\n");
		printf("--TID                   [value] (REQUIRED if no UID)\n");
		printf("--TID_version           [value] (OPTIONAL) Possible values: 1 (DEFAULT)\n");
		printf("--UID                   [value] (REQUIRED if no TID)\n");
		printf("--UID_version           [value] (OPTIONAL) Possible values: 1 (DEFAULT)\n");
		printf("--URL                   [value] (OPTIONAL)\n");
		printf("--URL_type              [value] (OPTIONAL) Possible values: WPUB (DEFAULT)\n");
		printf("                                                            WCOM\n");
		printf("                                                            WCOP\n");
		printf("                                                            WOAF\n");
		printf("                                                            WOAR\n");
		printf("                                                            WOAS\n");
		printf("                                                            WORS\n");
		printf("                                                            WPAY\n");
		printf("--URLS                  [value] (OPTIONAL) List of multiple URL values. Either --URL or --URLS can be used, not both\n");
		printf("                                           URLS are provided as a comma-delimited lined list.\n");
		printf("                                               (eg. --URLS \"http://www.umusic.com,http://www.blah.com\"\n");
		printf("--URLS_type             [value] (OPTIONAL) List of type values for multiple URLS. There must be a 1-to-1 correspondence\n");
		printf("                                           between URLS and URLS_type. If none specified, default is WPUB for all URLS.\n");
		printf("                                               (eg. --URLS_type \"WPUB,WCOP\"\n");
		printf("--PA                    [value] (OPTIONAL) Possible values: unspecified\n");
		printf("                                                            explicit\n");
		printf("                                                            edited\n");
		printf("--Copyright             [value] (OPTIONAL)\n");
		printf("--Copyright_value       [value] (OPTIONAL) Possible values: allrightsreserved (DEFAULT)\n");
		printf("                                                            unspecified\n");
		printf("                                                            prerelease\n");
		printf("                                                            other\n");
		printf("--Extra                 [value] (OPTIONAL)\n");
		printf("--Extra_type            [value] (OPTIONAL)\n");
		printf("--Extras                [value] (OPTIONAL) List of multiple Extra values. Either --Extra or --Extras can be used, \n");
		printf("                                           but not both. Extras are provided as a comma-delimited lined list.\n");
		printf("                                               (eg. --Extras \"Extra content 1, Extra Content 2\"\n");
		printf("--Extras_type           [value] (OPTIONAL) List of type values for multiple Extra values. There must be a 1-to-1 \n");
		printf("                                           correspondence between Extras and Extras_type. \n");
		printf("                                               (eg. --Extras_type \"content1 type, content 2 type\"\n");
		
	} else if (strcmp(command, "verify") == 0) {
		printf("Usage: uits_tool verify [options]\n");
		printf("\n");
		printf("--verbose    (-v)                           Run in verbose mode (DEFAULT)\n");
		printf("--silent     (-s)                           Run in silent mode \n");
		printf("--nohash     (-n)				 (OPTIONAL) Disable media hash validation.\n");
		printf("--audio      (-a)   [file-name]  (REQUIRED if hash validation enabled and no hash or hashfile specified)\n");
		printf("                                            Name of the audio file for which to verify payload\n");
		printf("                                            If the audio file contains a UITS payload, that \n");
		printf("                                            payload will be verified.\n");
		printf("--uits       (-u)   [file-name]  (REQUIRED if no audio file with embedded UITS payload)\n");
		printf("                                            Name of a file containing a UITS payload ONLY.\n");
		printf("                                            in the audio file. \n");
		printf("--hash       (-h)   [hash value] (REQUIRED if hash validation enabled and no audio or hashfile specified)\n");
		printf("                                            The reference media hash value for the UITS payload. This\n");
		printf("                                            value will be compared with the value in the payload for verification. \n");
		printf("--hashfile   (-f)   [file-name]  (REQUIRED if hash validation enabled and no audio file or hash specified)\n");
		printf("                                            Name of a file containing the reference media hash value\n");
		printf("                                            The reference media hash value will be compared with the value \n");
		printf("                                            in the payload for verification. \n");
		printf("--algorithm  (-r)   [name]      (OPTIONAL): Name of the algorithm to use for signing. \n");
		printf("                                            Possible values: RSA2048 (DEFAULT)\n");
		printf("                                                             DSA2048\n");
		printf("--pub        (-b)   [file-name] (REQUIRED): Name of the file containing the public key for validating\n");
		printf("--xsd        (-x)   [file-name] (OPTIONAL): Name of the schema to use for validation\n"); 
		printf("                                            DEFAULT is uits.xsd in current directory\n");

	} else if (strcmp(command, "extract") == 0) {
		printf("Usage: uits_tool extract [options]\n");
		printf("\n");
		printf("--verbose   (-v)                              Run in verbose mode (DEFAULT)\n");
		printf("--silent    (-s)                              Run in silent mode \n");
		printf("--audio     (-a)      [file-name] (REQUIRED): Name of the audio file from which to extract payload\n");
		printf("--uits      (-u)      [file-name] (REQUIRED): Name of the standalone file to write payload\n");
		printf("--verify    (-v)    			  (OPTIONAL): If specified, verify the payload after it is extracted from the audio file\n");
		printf("--algorithm (-r)      [name]      (OPTIONAL): If verifying, name of the algorithm to use for signing. \n");
		printf("                                              Possible values: RSA2048 (DEFAULT)\n");
		printf("                                                               DSA2048\n");
		printf("--pub       (-b)     [file-name] (REQUIRED for verify): Name of the file containing the public key for validating\n");
		printf("--xsd       (-x)     [file-name] (OPTIONAL): Name of the schema to use for validation\n"); 
		printf("                                             DEFAULT is uits.xsd in current directory\n");
	} else if (strcmp(command, "errors") == 0) {
		printf("\n");
		printf("Usage: uits_tool errors \n");		
	} else {	/* print the general help */
		printf ("\nUsage: uits_tool command [options]\n");
		printf("\n");
		printf("commands\n");
		printf("help        Display general help\n");
		printf("version     Dislay the tool version number\n");
		printf("errors      Display exit error codes and message list\n");
		printf("create      Create a UITS payload for an audio file. \n");
		printf("            Payload can be embedded into the audio file or written to a separate file\n");
		printf("verify      Verify a UITS payload\n");
		printf("            Payload can be within an audio file or in a separate file\n");
		printf("extract     Extract a UITS payload from an audio file and write to a separate file\n");
		printf("hash        Generate a media hash for an audio file\n");
		printf("key         Generate a key ID for a public key file\n");
		printf("\n");
		printf("Usage: uits_tool help command	- provides detailed help for a command\n");
		printf("\n");
	}
	
	exit(0);

}

/* Function: uitsGetCommand
 * Purpose:  Parse the command line options to determine which action to perform
 * Passed:   argc, argv
 * Returns:	 UITS action to perform or ERROR
 *           UITS actions are defined in the uitsActions enum
 */

int uitsGetCommand (int argc, const char * argv[]) {
	
	char *command = (char *) argv[1];
	int  action = 0;
	
	if (!argv[1]) {
		uitsPrintHelp("help");	// print general help and exit 
	}
	
	dprintf("uitsGetCommand: %s\n", command);
	
	if (strcmp(command, "create") == 0) {
		action = CREATE;
	} else if (strcmp(command, "verify") == 0) {
		action = VERIFY;
	} else if (strcmp(command, "extract") == 0) {
		action = EXTRACT;
	} else if (strcmp(command, "hash") == 0) {
		action = HASH;
	} else if (strcmp(command, "key") == 0) {
		action = KEY;
	} else if (strcmp(command, "errors") == 0) {
		action = ERRORS;
	} else if (strcmp(command, "version") == 0) {
		uitsPrintHelp(command);	// print version  and exit 
	} else if (strcmp(command, "help") == 0) {
		/* check to see if the usage was "uits_tool help action" */
		command = (char *)argv[2];
		if (command && strcmp(command, "help")) {
			uitsPrintHelp(command);	// print help for action and exit 
		} else {
			uitsPrintHelp("help");	// print general help and exit 
		}
	}
	
	return (action);
}

/*
 *
 * Function: uitsGetOptCreate
 * Purpose:  Parse the options for
 *			 Usage: uits_tool create [options]
 * Returns:	 OK or ERROR
 *
 */


int uitsGetOptCreate (int argc, const char * argv[]) 
{
	int				option_index = 0;
	int				c;		// character for command-line processing
	struct			option *option_ptr;
	int				option_count = 0;
	char			*option_value;
	UITS_attributes *attributePtr;
	char			*attributeOptionName;
	int				attributeOptionNameLen;
	
	UITS_element *metadataPtr  = uitsGetMetadataDesc();
//	UITS_signature_desc *signaturePtr = uitsGetSignatureDesc();	
	
	// command-line option definition for option parsing
	static struct option long_options [MAX_COMMAND_LINE_OPTIONS] = {
		{"debug",           no_argument,		0,	'w'},	// turn on debug printing (hidden option!)
		{"verbose",			no_argument,		0,	'v'},	// turn on verbose mode
		{"silent",			no_argument,		0,	's'},	// turn on silent mode
		{"audio",   		required_argument,	0,	'a'},	// audio file
		{"uits",	        required_argument,	0,	'u'},	// uits payload file
		{"embed",			no_argument,		0,	'e'},	// embed audio into audio file
		{"metadata_file",	required_argument,	0,	'f'},	// metadata file
		{"algorithm",		required_argument,	0,	'r'},	// algorithm for signature encryption file: 'DSA2048' or 'RSA2048'
		{"pub",	            required_argument,	0,	'b'},	// public key file
		{"priv",            required_argument,	0,	'i'},	// private key file
		{"pubID",	        required_argument,	0,	'k'},	// public key id (sha1 hash of pub key file)
		{"pad",				required_argument,	0,	'd'},	// pad bytes for MP3 payload injection
		{"xsd",				required_argument,	0,	'x'},	// xsd file for schema validation
		{"ml",              no_argument,		0,	'm'},	// generate a multi-line signature
		{"b64",             no_argument,		0,	'c'},	// base64 encode media hash
		{"hash",		    required_argument,	0,	'h'},	// hash value to use instead of calculating from audio frames
		/* end of option list */
		{0,			0,				0,				0}
	};
	
	
	// Add the UITS metadata to the list of options. This allows the metadata to be added to options processing
	// dynamically so that the metadata definition lives only in the uits_metadata array.
	
	// find the end of the long_options array
	option_ptr = long_options;
	
	while (option_ptr->name) {
		option_count++;
		option_ptr++;
	}
	
	// now add the uits_metadata elements and attributes to the long_options array
	// only long options can be entered on the command-line (eg. --nonce)
	// the short option for all elements is converted internal to -Y 
	// the short option for all element attributes is converted to -Z
	
	while ( metadataPtr->name) {
		long_options[option_count].name =  metadataPtr->name;
		long_options[option_count].has_arg = required_argument;
		long_options[option_count].flag = 0;
		long_options[option_count].val = 'Y';
		attributePtr =  metadataPtr->attributes;
		if ( attributePtr) {		// add any attributes for this element
			while ( attributePtr->name) {				
				// attribute option name is elementname_attributename (eg. "productID_type")
				attributeOptionNameLen = strlen( metadataPtr->name) + strlen( attributePtr->name) + 2;
				attributeOptionName = calloc(attributeOptionNameLen, sizeof(char));
				uitsHandleErrorPTR(moduleName, "uitsGetOptCreate", attributeOptionName, ERR_UITS, "Error allocting attributeOptionName\n");		
				attributeOptionName = strcat( attributeOptionName,  metadataPtr->name);
				attributeOptionName = strcat( attributeOptionName, "_");
				attributeOptionName = strcat( attributeOptionName,  attributePtr->name);
				option_count++;
				long_options[option_count].name =  attributeOptionName;
				long_options[option_count].has_arg = required_argument;
				long_options[option_count].flag = 0;
				long_options[option_count].val = 'Z';
				attributePtr++;
			}
			
		}
		metadataPtr++;
		option_count++;
		if (option_count > MAX_COMMAND_LINE_OPTIONS) {
			uitsHandleErrorINT(moduleName, "uitsGetOptCreate", ERROR, OK, ERR_UITS,  
							   "ERROR uitsGetOpt: too many UITS metadata parameters. Increase MAX_COMMAND_LINE_OPTIONS.\n");
		}
	}
	
	while (1) {
		c = getopt_long (argc, argv, "vsemcoa:u:f:h:r:b:i:k:d:x:m:h:Y:Z:w:", long_options, &option_index);
		// dprintf("Got option: %c, value: %s\n", c, optarg);
		
		fflush(stdout);
		if (c == -1) { break; }
		
		switch (c) {				
			case 'w':		// hidden debug flag
				debugFlag = TRUE;
				dprintf("Debug printing enabled\n");
				break;

			case 'v':		// set verbose flag
				verboseFlag = TRUE;
				break;

			case 's':		// set silent mode
				silentFlag = TRUE;
				break;
				
			case 'a':		// set input audio file name
				option_value = strdup(optarg);
				dprintf ("audio file '%s'\n", option_value);
				uitsSetIOFileName (AUDIO, option_value);
				break;
				
				
			case 'u':		// set payload file name
				option_value = strdup(optarg);
				dprintf ("UITS payload file '%s'\n", option_value);
				uitsSetIOFileName (PAYLOAD, option_value);
				break;
				
			case 'f':		// set metadata file name
				option_value = strdup(optarg);
				dprintf ("metadata file '%s'\n", option_value);
				uitsSetIOFileName (METADATA, option_value);
				break;
				
			case 'x':		// set xsd file name
				option_value = strdup(optarg);
				dprintf ("xsd file '%s'\n", option_value);
				uitsSetIOFileName (UITS_XSD, option_value);
				break;
				
			case 'r':		// set algorithm name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("algorithm", option_value);
				dprintf ("algorithm '%s'\n", option_value);
				break;
				
			case 'b':		// set public key file name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("pubKeyFileName", option_value);
				dprintf ("public key file '%s'\n", option_value);
				break;
				
			case 'i':		// set private key file name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("privateKeyFileName", option_value);
				dprintf ("private key file '%s'\n", option_value);
				break;
				
			case 'k':		// set public key id 
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("pubKeyID", option_value);
				dprintf ("Public Key ID '%s'\n", option_value);
				break;

			case 'h':		// set media hash value for verification
				option_value = strdup(optarg);
				dprintf ("media hash value '%s'\n", option_value);
				uitsSetCLMediaHashValue (option_value);
				break;
				
			case 'd':		// set padding
				uitsSetCommandLineParam ("pad", atoi(optarg));
				dprintf ("padding '%s'\n", option_value);
				break;
				
			case 'm':		// set b65 line feed flag
				uitsSetSignatureParamValue ("b64LFFlag", "TRUE");
				dprintf ("base 64 linefeeds enabled\n");
				break;
				
			case 'c':		// set base 64 encoding for media hash
				uitsSetCommandLineParam ("b64_media_hash", TRUE);
				dprintf ("base 64 encode media hash\n");
				break;
								
			case 'e':		// set inject flag
				uitsSetCommandLineParam ("embed", TRUE);
				dprintf ("Embed payload in audio file\n");
				break;
				
				// the UITS metadata parameters all have a short option of "Y"	
			case 'Y':
				option_value = strdup(optarg);
				uitsSetMetadataValue ((char *)long_options[option_index].name, option_value);
				dprintf ("Setting metadata element: %s value: %s\n", long_options[option_index].name, option_value);
				break;
				
				// the UITS metadata element attributes all have a short option of "A"	
				// NOTE: if a metadata file is specified, command-line parameters are ignored
			case 'Z':
				option_value = strdup(optarg);
				uitsSetMetadataAttributeValue ((char *)long_options[option_index].name, option_value);
				dprintf ("Setting metadata attribute: %s value: %s\n", long_options[option_index].name, option_value);
				break;
				
			default: 
				snprintf(errStr, ERRSTR_LEN, "Error processing options: unknown option: %c\n", c);
				uitsHandleErrorINT(moduleName, "uitsGetOptCreate", ERROR, OK, ERR_VALUE, errStr);
		}
	}
	
	return (OK);
	
}


/*
 * Function: uitsGetOptVerify
 * Purpose:  Parse the options for
 *				uits_tool verify [options]
 * Passed:   argc, argv
 * Returns:  OK or ERROR
 */

int uitsGetOptVerify (int argc, const char * argv[]) 
{
	int				option_index = 0;
	int				c;		// character for command-line processing
	char			*option_value;
	
	// command-line option definition for option parsing
	static struct option long_options [MAX_COMMAND_LINE_OPTIONS] = {
		{"debug",           no_argument,		0,	'w'},	// turn on debug printing (hidden option!)
		{"verbose",			no_argument,		0,	'v'},	// turn on verbose mode
		{"silent",			no_argument,		0,	's'},	// turn on silent mode
		{"audio",		    required_argument,	0,	'a'},	// audio file
		{"uits",			required_argument,	0,	'u'},	// payload file
		{"hash",			required_argument,	0,	'h'},	// media hash value against which to verify
		{"hashfile",		required_argument,	0,	'f'},	// file containing media hash value against which to verify
		{"algorithm",		required_argument,	0,	'r'},	// algorithm for signature encryption file: 'DSA2048' or 'RSA2048'
		{"pub",				required_argument,	0,	'b'},	// public key file
		{"xsd",				required_argument,	0,	'x'},	// xsd file for schema validation
		{"nohash",			no_argument,		0,	'n'},	// don't validate media hash
		
		/* end of option list */
		{0,			0,				0,				0}
	};
		
	while (1) {
		c = getopt_long (argc, argv, "vsa:u:h:f:r:b:x:w:", long_options, &option_index);
		
		if (c == -1) { break; }
		
		switch (c) {				
			case 'w':		// set the hidden debug flag
				debugFlag = TRUE;
				dprintf("Debug printing enabled\n");
				break;
				
			case 'v':		// set the verbose flag
				verboseFlag = TRUE;
				break;

			case 's':		// set silent mode flag
				silentFlag = TRUE;
				break;
				
			case 'a':		// set input audio file name
				option_value = strdup(optarg);
				dprintf ("audio file '%s'\n", option_value);
				uitsSetIOFileName (AUDIO, option_value);
				break;
				
			case 'h':		// set media hash value for verification
				option_value = strdup(optarg);
				dprintf ("media hash value '%s'\n", option_value);
				uitsSetCLMediaHashValue (option_value);
				break;

			case 'f':		// set media hash file name
				option_value = strdup(optarg);
				dprintf("media hash file '%s'\n", option_value);
				uitsSetIOFileName (MEDIAHASH, option_value);
				break;

			case 'u':		// set uits payload file name
				option_value = strdup(optarg);
				dprintf ("payload file '%s'\n", option_value);
				uitsSetIOFileName (PAYLOAD, option_value);
				break;
								
			case 'x':		// set xsd file name
				option_value = strdup(optarg);
				dprintf ("xsd file '%s'\n", option_value);
				uitsSetIOFileName (UITS_XSD, option_value);
				break;
				
			case 'r':		// set algorithm name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("algorithm", option_value);
				dprintf("algorithm '%s'\n", option_value);
				break;
				
			case 'b':		// set public key file name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("pubKeyFileName", option_value);
				dprintf ("public key file '%s'\n", option_value);
				break;
				
			case 'n':		// set the flag to disable media hash verification
				uitsSetCommandLineParam ("nohash", TRUE);
				dprintf("Media hash will not be verified\n");
				break;
				
			default: 
				snprintf(errStr, ERRSTR_LEN, "Error processing options: unknown option: %c\n", c);
				uitsHandleErrorINT(moduleName, "uitsGetOptVerify", ERROR, OK, ERR_VALUE, errStr);
		}
	}
	
	//	vprintf ("Got verbose flag\n");
	return (OK);
	
}


/*
 *
 * Function: uitsGetOptExtract
 * Purpose:  Parse the options for
 *				uits_tool extract [options]
 * Passed:   argc, argv
 * Returns: OK or ERROR
 */

int uitsGetOptExtract (int argc, const char * argv[]) 
{
	int				option_index = 0;
	int				c;		// character for command-line processing
	char			*option_value;
	
	// command-line option definition for option parsing
	static struct option long_options [MAX_COMMAND_LINE_OPTIONS] = {
		{"debug",           no_argument,		0,	'w'},	// turn on debug printing (hidden option!)
		{"verbose",			no_argument,		0,	'v'},	// turn on verbose mode (DEFAULT)
		{"silent",			no_argument,		0,	's'},	// turn on silent mode
		{"verify",			no_argument,		0,  'y'},	// verify payload after extraction
		{"audio",		    required_argument,	0,	'a'},	// audio file
		{"uits",			required_argument,	0,	'u'},	// payload file
		{"algorithm",		required_argument,	0,	'r'},	// algorithm for signature encryption file: 'DSA2048' or 'RSA2048'
		{"pub",				required_argument,	0,	'b'},	// public key file
		{"xsd",				required_argument,	0,	'x'},	// xsd file for schema validation
		
		/* end of option list */
		{0,			0,				0,				0}
	};
	
	
	while (1) {
		c = getopt_long (argc, argv, "vsya:u:r:b:x:w:", long_options, &option_index);
		
		if (c == -1) { break; }
		
		switch (c) {
				
			case 'w':		// set the hidden debug flag
				debugFlag = TRUE;
				dprintf("Debug printing enabled\n");
				break;

			case 'v':		// set the verbose flag
				verboseFlag = TRUE;
				break;
				
			case 's':		// set silent mode flag
				silentFlag = TRUE;
				break;
				
			case 'y':		// set verify flag
				uitsSetCommandLineParam ("verify", TRUE);
				dprintf("Verify extracted payload\n");
				break;
				
			case 'a':		// set input audio file name
				option_value = strdup(optarg);
				dprintf ("audio file '%s'\n", option_value);
				uitsSetIOFileName (AUDIO, option_value);
				break;
								
			case 'u':		// set uits payload file name
				option_value = strdup(optarg);
				dprintf ("payload file '%s'\n", option_value);
				uitsSetIOFileName (PAYLOAD, option_value);
				break;
				
			case 'x':		// set xsd file name
				option_value = strdup(optarg);
				dprintf ("xsd file '%s'\n", option_value);
				uitsSetIOFileName (UITS_XSD, option_value);
				break;
				
			case 'r':		// set algorithm name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("algorithm", option_value);
				dprintf ("algorithm '%s'\n", option_value);
				break;
				
			case 'b':		// set public key file name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("pubKeyFileName", option_value);
				dprintf ("public key file '%s'\n", option_value);
				break;
				
			default: 
				snprintf(errStr, ERRSTR_LEN, "Error processing options: unknown option: %c\n", c);
				uitsHandleErrorINT(moduleName, "uitsGetOptExtract", ERROR, OK, ERR_VALUE, errStr);
		}
		
	}
	
	return (OK);
	
}


/*
 *
 * Function: uitsGetOptGenHash
 * Purpose:  Parse the options for
 *				uits_tool hash [options]
 * Passed:   argc, argv
 *
 * Returns: OK or ERROR
 */
int uitsGetOptGenHash (int argc, const char * argv[]) 
{
	int				option_index = 0;
	int				c;		// character for command-line processing
	char			*option_value;
	
	// command-line option definition for option parsing
	static struct option long_options [MAX_COMMAND_LINE_OPTIONS] = {
		{"debug",           no_argument,		0,	'w'},	// turn on debug printing (hidden option!)
		{"verbose",			no_argument,		0,	'v'},	// turn on verbose mode
		{"b64",				no_argument,	    0,	'c'},	// generate base 64 encoded media hash (hex default)
		{"audio",			required_argument,	0,	'a'},	// audio file		
		{"output",			required_argument,	0,	'o'},	// output file		
		/* end of option list */
		{0,			0,				0,				0}
	};
	
	
	while (1) {
		c = getopt_long (argc, argv, "vca:o:w:", long_options, &option_index);
		
		if (c == -1) { break; }
		
		switch (c) {
				
			case 'w':		// set the hidden debug flag
				debugFlag = TRUE;
				dprintf("Debug printing enabled\n");
				break;
				
			case 'v':		// set the verbose flag
				verboseFlag = TRUE;
				break;
				
			case 'a':		// set input audio file name
				option_value = strdup(optarg);
				dprintf ("audio file '%s'\n", option_value);
				uitsSetIOFileName (AUDIO, option_value);
				break;
				
			case 'o':		// set input audio file name
				option_value = strdup(optarg);
				dprintf ("output file '%s'\n", option_value);
				uitsSetIOFileName (OUTPUT, option_value);
				break;

			case 'c':		// set base 64 encode media hash flag
				uitsSetCommandLineParam ("b64_media_hash", TRUE);
				dprintf ("Media hash will be base 64 encoded\n");
				break;
							
				
			default: 
				snprintf(errStr, ERRSTR_LEN, "Error processing options: unknown option: %c\n", c);
				uitsHandleErrorINT(moduleName, "uitsGetOptGenHash", ERROR, OK, ERR_VALUE, errStr);
		}
	}
	
	return (OK);
	
}
/*
 *
 * Function: uitsGetOptGenKey
 * Purpose:  Parse the options for
 *				uits_tool hash [options]
 * Passed:   argc, argv
 * Returns: OK or ERROR
 */
int uitsGetOptGenKey (int argc, const char * argv[]) 
{
	int				option_index = 0;
	int				c;		// character for command-line processing
	char			*option_value;
	
	// command-line option definition for option parsing
	static struct option long_options [MAX_COMMAND_LINE_OPTIONS] = {
		{"debug",           no_argument,		0,	'w'},	// turn on debug printing (hidden option!)
		{"verbose",			no_argument,		0,	'v'},	// turn on verbose mode
		{"pub",	            required_argument,	0,	'b'},	// public key file
		{"output",			required_argument,	0,	'o'},	// output file		
		
		/* end of option list */
		{0,			0,				0,				0}
	};
	while (1) {
		c = getopt_long (argc, argv, "vb:o:w:", long_options, &option_index);
		
		if (c == -1) { break; }
		
		switch (c) {				
			case 'w':		// set the hidden debug flag
				debugFlag = TRUE;
				dprintf("Debug printing enabled\n");
				break;
				
			case 'v':		// set the verbose flag
				verboseFlag = TRUE;
				break;
				
			case 'o':		// set input audio file name
				option_value = strdup(optarg);
				dprintf ("output file `%s'\n", option_value);
				uitsSetIOFileName (OUTPUT, option_value);
				break;
					
			case 'b':		// set public key file name
				option_value = strdup(optarg);
				uitsSetSignatureParamValue ("pubKeyFileName", option_value);
				dprintf ("public key file `%s'\n", option_value);
				break;
				
			default: 
				snprintf(errStr, ERRSTR_LEN, "Error processing options: unknown option: %c\n", c);
				uitsHandleErrorINT(moduleName, "uitsGetOptGenKey", ERROR, OK, ERR_VALUE, errStr);
		}
	}
	
	return (OK);
	
}

/*
 * Function: uitsInit
 * Purpose:  Initialize whatever needs to be initialized for the tool
 * Passed:   nothing
 * Returns:  OK
 *
 */
int	uitsInit() {											
	

	// init the printf output flags
	silentFlag  = FALSE;			// silent mode		OFF
	verboseFlag = TRUE;				// verbose mode		ON
	debugFlag   = FALSE;			// debug messages	OFF
	
	// make sure the mxml writing routines don't do any spurious line-wrapping
	
	mxmlSetWrapMargin(0);
	
	// initialize the paylod manager
	uitsPayloadManagerInit();
	
	// initialize the openssl functions
	uitsOpenSSLInit();

	return (OK);

}



/*
 * Function: uitsReadFile
 * Purpose:  Open, read, and close a file
 * Passed:   Name of file
 * Returns:  Pointer to data read from file
 *
 */

unsigned char *uitsReadFile (char *filename) 
{
	
	FILE *fp=NULL;
	int fileLen;
	unsigned char *fileData;	
	
	fp = fopen(filename, "r");	
	if (!fp) {
		snprintf(errStr, ERRSTR_LEN, "ERROR: Couldn't open file: %s\n", filename);
		uitsHandleErrorINT(moduleName, "uitsReadFile", ERROR, OK, ERR_FILE, errStr);
	}
	
	/* read the whole file into memory */
//	fseeko(fp, 0L, SEEK_END);  /* Position to end of file */
//	fileLen = ftell(fp);      /* Get file length */
//	rewind(fp);               /* Back to start of file */

	fileLen = uitsGetFileSize(fp);
	fileData = calloc(fileLen + 1, sizeof(char));
	
	if(!fileData){
		snprintf(errStr, ERRSTR_LEN,  "ERROR: Insufficient memory to read %s\n", filename);
		uitsHandleErrorINT(moduleName, "uitsReadFile", ERROR, OK, ERR_FILE, errStr);
	}
	
	fread(fileData, fileLen, 1, fp); /* Read the entire file into fileData */
	fclose(fp);
	return(fileData);
	
	
}

/*
 * Function: uitsGetFileSize
 * Purpose:  Seek from the current location to end of a file to find the remaining size of the file
 * Passed:   pointer to file
 * Returns:  File size, fp left at original location
 *
 */

int uitsGetFileSize (FILE *fp) 
{
	unsigned long fileSize;
	unsigned long saveSeek;
	
	saveSeek = ftello(fp);
	
	fseeko(fp, 0L, SEEK_END);  /* Position to end of file */
	fileSize = ftello(fp);
	fseeko(fp, saveSeek, SEEK_SET);
	
	return (fileSize);
	
}



// EOF
