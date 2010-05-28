/*
 *  uitsMP4Manager.c
 *  UITS_Tool
 *
 *  Created by Chris Angelli on 4/2/10.
 *  Copyright 2010 Universal Music Group. All rights reserved.
 *
 *  $Date$
 *  $Revision$
 *
 */

#include "uits.h"


char *mp4ModuleName = "uitsMP4Manager.c";


/*
 *
 * Function: mp4IsValidFile
 * Purpose:	 Determine if an audio file is an MP4 file
 *           Valid MP4 files start with 4-bytes containing the file size
 *           followed by 4-bytes containing 'ftyp'.
 *
 * Returns:   TRUE if MP4, FALSE otherwise
 */

int mp4IsValidFile (char *audioFileName) 
{
	FILE *audioFP;
	MP4_ATOM_HEADER *atomHeader = calloc(sizeof(MP4_ATOM_HEADER), 1);
		
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp4ModuleName, "mp4IsValidFile", audioFP, "Couldn't open audio file for reading\n");
	
	/* Read the first 8 bytes of the file and check to see if they represent a MP4 'ftyp' atom */
	/* The first 4 bytes are size, the second 4 bytes should be 'ftyp' */
	
	atomHeader = mp4ReadAtomHeader(audioFP);

	fclose (audioFP);	

 	if (strncmp(atomHeader->type, "ftyp", 4) == 0) {
		vprintf("Audio file is MP4\n");
		return (TRUE);
	} else {
		return (FALSE);
	}	
}

/*
 *
 * Function: mp4GetMediaHash
 * Purpose:	 Calcluate the media hash for an MP4 file
 *			 The MP4 file is parsed using the following algorithm:
 *			  1. Parse file until the 'mdat' atom is found
 *			  3. Hash the data in the 'mdat' atom
 *
 * Returns:   Pointer to the hashed frame data
 */

char *mp4GetMediaHash (char *audioFileName) 
{
	FILE			*audioFP;
	MP4_ATOM_HEADER *atomHeader = calloc(sizeof(MP4_ATOM_HEADER), 1);
	unsigned long	fileLength;
	unsigned long	audioFrameStart, audioFrameEnd, audioFrameLength;
	UITS_digest		*mediaHash;
	char			*mediaHashString;
	
	audioFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp4ModuleName, "mp4GetMediaHash", audioFP, "Couldn't open audio file for reading\n");
	
	/* get file size */
	fileLength = uitsGetFileSize(audioFP);

	atomHeader = mp4FindAtomHeader(audioFP, "mdat", fileLength);
	uitsHandleErrorPTR(mp4ModuleName, "mp4GetMediaHash", atomHeader, "Couldn't find 'mdat' atom in audio file\n");
	
	/* move fp to start of mdat atom */
	fseeko(audioFP, atomHeader->saveSeek, SEEK_SET);

	/*skip past size and type */
	fseeko(audioFP, 8L, SEEK_CUR);
		
	/* fp is (hopefully) at start of audio frame data */
	audioFrameStart = ftello(audioFP);
	
	/* if size is 0, need to calculate how long the audio frame data is by seeking to EOF and saving size  */
	if (atomHeader->size == 0) {
		fseeko(audioFP, 0L, SEEK_END);
		audioFrameEnd = ftello(audioFP);	
		fseeko(audioFP, audioFrameStart, SEEK_SET);
		audioFrameLength = audioFrameEnd - audioFrameStart;
	} else {
		audioFrameLength = atomHeader->size - 8;
	}
	
	mediaHash = uitsCreateDigestBuffered (audioFP, audioFrameLength, "SHA256") ;
	
	mediaHashString = uitsDigestToString(mediaHash);
	
	/* cleanup */
	fclose(audioFP);
	
	vprintf("Calculated media Hash string for MP4 file: %s\n", mediaHashString);

	return (mediaHashString);
	
}

/*
 *
 * Function: mp4EmbedPayload
 * Purpose:	 Embed the UITS payload into an MP4 file
 *			 The UITS payload is added in a top-level 'skip' atom.
 *			 After the UITS payload is embedded, the atom hierarchy will look something like this:
 *				'ftyp'
 *				'moov'
 *				'mdat'
 *				'skip'
 *					'udta'
 *						'UITS'
 *
 *
 * Returns:   OK or ERROR
 */

int mp4EmbedPayload  (char *audioFileName, 
					  char *audioFileNameOut, 
					  char *uitsPayloadXML,
					  int  numPadBytes) 
{
	FILE			*audioInFP, *audioOutFP;
	MP4_ATOM_HEADER *atomHeader = NULL;
	unsigned long	audioInFileSize;
	unsigned long   payloadXMLSize;
	unsigned long	bytesLeftInFile, bytesCopied;
	
	MP4_ATOM_HEADER *moovAtomHeader = NULL;
	MP4_ATOM_HEADER *udtaAtomHeader = NULL;
	unsigned long	endSeek;
	unsigned long	atomSize;
		
		
	vprintf("About to embed payload for %s into %s\n", audioFileName, audioFileNameOut);
	payloadXMLSize = strlen(uitsPayloadXML);
	
	/* open the audio input and output files */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp4ModuleName, "mp4EmbedPayload", audioInFP, "Couldn't open audio file for reading\n");
	
	audioOutFP = fopen(audioFileNameOut, "w+b");
	uitsHandleErrorPTR(mp4ModuleName, "mp4EmbedPayload", audioOutFP, "Couldn't open audio file for writing\n");

	/* calculate how long the input audio file is by seeking to EOF and saving size  */
	audioInFileSize = uitsGetFileSize(audioInFP);

#define UITS_AT_START_OF_udta			
#ifdef UITS_AT_START_OF_udta
	/* find the 'moov' atom header */	
	moovAtomHeader = mp4FindAtomHeader(audioInFP, "moov", audioInFileSize);
	uitsHandleErrorPTR(mp4ModuleName, "mp4EmbedPayload", moovAtomHeader, "Coudln't find 'moov' atom header\n");

	/* seek past the moov atom header */
	fseeko(audioInFP, moovAtomHeader->saveSeek, SEEK_SET);
	fseeko(audioInFP, 8, SEEK_CUR);

	atomSize = moovAtomHeader->size - 8;

	/* find the 'udta' atom header that is a child the 'moov' atom container */
	udtaAtomHeader = mp4FindAtomHeader(audioInFP, "udta", atomSize);
	uitsHandleErrorPTR(mp4ModuleName, "mp4EmbedPayload", udtaAtomHeader, "Coudln't find 'udta' atom header\n");

	/* now do the data copy and modification */
	rewind(audioInFP);

	/* copy from beginning of file to beginning of 'moov' atom */
	uitsAudioBufferedCopy(audioInFP, audioOutFP, moovAtomHeader->saveSeek);
	
	atomSize = moovAtomHeader->size + payloadXMLSize + 8;
	lswap(&atomSize);
	
	/* write the moov atom header */
	fwrite(&atomSize, 1, 4, audioOutFP);   /* 4-bytes size */
	fwrite("moov",    1, 4, audioOutFP);   /* 4-bytes ID */
	
	/* seek past the moov atom header */
	fseeko(audioInFP, moovAtomHeader->saveSeek, SEEK_SET);
	fseeko(audioInFP, 8, SEEK_CUR);
	
	/* write the moov atom until the start of the 'udta' atom */
	atomSize = udtaAtomHeader->saveSeek - ftello(audioInFP);
	uitsAudioBufferedCopy(audioInFP, audioOutFP, atomSize);

	/* write the udta atom header */
	atomSize = udtaAtomHeader->size + payloadXMLSize + 8;
	lswap(&atomSize);
	
	fwrite(&atomSize, 1, 4, audioOutFP);   /* 4-bytes size */
	fwrite("udta",    1, 4, audioOutFP);   /* 4-bytes ID */
	fseeko(audioInFP, 8, SEEK_CUR);
	
	/* write the UITS atom */
	atomSize = payloadXMLSize + 8;
	lswap(&atomSize);
	fwrite(&atomSize, 1, 4, audioOutFP);					/* 4-bytes size */
	fwrite("UITS",    1, 4, audioOutFP);					/* 4-bytes ID */
	fwrite(uitsPayloadXML, 1, payloadXMLSize, audioOutFP);	/* UITS payload */

	/* write the rest of the file */
	endSeek = audioInFileSize - ftello(audioInFP);			/* save the size of the rest of the data */
	uitsAudioBufferedCopy(audioInFP, audioOutFP, endSeek);
	
	/* the output file now has the UITS payload inserted into the 'udta' chunk */
	/* one last bit of housekeeping is reqyired: */
	/* update the chunk offset table to skip past the new chunk */
	atomSize = payloadXMLSize + 8;
	err = mp4UpdateChunkOffsetTable(audioOutFP, atomSize);
	uitsHandleErrorINT(mp4ModuleName, "mp4EmbedPayload", err, OK, "Couldn't update chunk offset table\n");
	
#endif

#ifdef UITS_AT_END_OF_udta
	/* find the 'moov' atom header */	
	moovAtomHeader = mp4FindAtomHeader(audioInFP, "moov", audioInFileSize);
	uitsHandleErrorPTR(mp4ModuleName, "mp4EmbedPayload", moovAtomHeader, "Coudln't find 'moov' atom header\n");
	
	/* seek past the moov atom header */
	fseeko(audioInFP, moovAtomHeader->saveSeek, SEEK_SET);
	fseeko(audioInFP, 8, SEEK_CUR);
	
	atomSize = moovAtomHeader->size - 8;
	
	/* find the 'udta' atom header that is a child the 'moov' atom container */
	udtaAtomHeader = mp4FindAtomHeader(audioInFP, "udta", atomSize);
	uitsHandleErrorPTR(mp4ModuleName, "mp4EmbedPayload", udtaAtomHeader, "Coudln't find 'udta' atom header\n");
	
	/* now do the data copy and modification */
	rewind(audioInFP);
	/* copy from beginning of file to beginning of 'moov' atom */
	uitsAudioBufferedCopy(audioInFP, audioOutFP, moovAtomHeader->saveSeek);
	
	atomSize = moovAtomHeader->size + payloadXMLSize  + 8;
	lswap(&atomSize);
	
	/* write the moov atom header */
	fwrite(&atomSize, 1, 4, audioOutFP);   /* 4-bytes size */
	fwrite("moov",    1, 4, audioOutFP);   /* 4-bytes ID */
	
	/* seek past the moov atom header */
	fseeko(audioInFP, moovAtomHeader->saveSeek, SEEK_SET);
	fseeko(audioInFP, 8, SEEK_CUR);
	
	/* write the moov atom until the start of the 'udta' atom */
	atomSize = udtaAtomHeader->saveSeek - ftello(audioInFP);
	uitsAudioBufferedCopy(audioInFP, audioOutFP, atomSize);	
	
	/* write the udta atom header */
	atomSize = udtaAtomHeader->size + payloadXMLSize + 8;
	lswap(&atomSize);
	
	fwrite(&atomSize, 1, 4, audioOutFP);   /* 4-bytes size */
	fwrite("udta",    1, 4, audioOutFP);   /* 4-bytes ID */
	
	/* seek past the udata header in input file */
	fseeko(audioInFP, 8, SEEK_CUR);

	/* write the rest of the udta atom */
	atomSize = udtaAtomHeader->size - 8;
	uitsAudioBufferedCopy(audioInFP, audioOutFP, atomSize);
	
	/* write the UITS atom */
	atomSize = payloadXMLSize + 8 ;
	lswap(&atomSize);
	fwrite(&atomSize, 1, 4, audioOutFP);					/* 4-bytes size */
	fwrite("UITS",    1, 4, audioOutFP);					/* 4-bytes ID */
	fwrite(uitsPayloadXML, 1, payloadXMLSize, audioOutFP);	/* UITS payload */
	
	/* write the rest of the file */
	endSeek = audioInFileSize - ftello(audioInFP);			/* save the size of the rest of the data */
	uitsAudioBufferedCopy(audioInFP, audioOutFP, endSeek);
#endif
		
	/* cleanup */
	fclose(audioInFP);
	fclose(audioOutFP);
	
	return(OK);
}

/*
 *
 * Function: mp4ExtractPayload
 * Purpose:	 Extract the UITS payload from an MP3 file
 *			 The payload is a leaf atom of type 'UITS' inside a top level 'udta' container atom
 *           Typically, the top level atoms look something like this:
 *					Atom ftyp @ 0 of size: 32, ends @ 32
 *					Atom moov @ 32 of size: 44130, ends @ 44162
 *					Atom mdat @ 51232 of size: 5466860, ends @ 5518092
 *					Atom udta @ 5518092 of size: 1205, ends @ 5519297
 *
 * Returns: pointer to payload or exit if error
 */

char *mp4ExtractPayload (char *audioFileName) 

{
	MP4_NESTED_ATOM nestedAtoms [] = {
		{ "moov", NULL },
		{ "udta", NULL },
		{ "UITS", NULL },
		{ NULL, 0}
	};
	
	FILE			*audioInFP;
	unsigned long	audioInFileSize;
	char			*payloadXML;
	unsigned long	atomSize;
	MP4_NESTED_ATOM *chunkTable = NULL;
	
	/* open the audio input file */
	audioInFP = fopen(audioFileName, "rb");
	uitsHandleErrorPTR(mp4ModuleName, "mp4ExtractPayload", audioInFP, "Couldn't open audio file for reading\n");

	audioInFileSize = uitsGetFileSize(audioInFP);

	/* populate the nested atom pointers */
	chunkTable = mp4FindAtomHeaderNested(audioInFP, nestedAtoms);
	
	/* find the chunk offset table within the nested atoms */
	while (	strcmp(chunkTable->atomType, "UITS") != 0) {
		chunkTable++;
	}
	
	/* seek past the udta atom header */
	fseeko(audioInFP, chunkTable->atomHeader->saveSeek, SEEK_SET);
	fseeko(audioInFP, 8, SEEK_CUR);
	
	atomSize = chunkTable->atomHeader->size - 8;
	
	/* this is a cheat. calloc 8 bytes more than we're going to read so that the payload XML */
	/* will be null-terminated when it's read from the file */
	payloadXML = calloc(chunkTable->atomHeader->size, 1);	
	atomSize = chunkTable->atomHeader->size - 8;

	err = fread(payloadXML, 1L, atomSize, audioInFP);
	uitsHandleErrorINT(mp4ModuleName, "mp4ExtractPayload", err, atomSize, "Couldn't read UITS atom data\n");
	
	return (payloadXML);
	
}

/*
 *
 * Function: mp4FindAtomHeaderNested
 * Purpose:	 Find an set of nested atoms
 
 *           
 * Passed:   File pointer (should point to file location to start search)
 *				The file pointer is returned to it's original position
 *			 NULL-terminted Array containing nested list of atoms
 * Returns:  Pointer to header structure or NULL if error
 */


MP4_ATOM_HEADER *mp4FindAtomHeaderNested (FILE *fpin, MP4_NESTED_ATOM *nestedAtoms)
{
	MP4_NESTED_ATOM *currAtom = nestedAtoms;
	unsigned int saveSeek;
	unsigned int atomSize;
	unsigned int filesize;
		
	saveSeek = ftello(fpin);
	rewind(fpin);
	
	atomSize = uitsGetFileSize(fpin);
	
	while (*currAtom->atomType) {
		currAtom->atomHeader = mp4FindAtomHeader(fpin, currAtom->atomType, atomSize);
		uitsHandleErrorPTR(mp4ModuleName, "mp4FindAtomHeaderNested", currAtom->atomHeader, 
						   "Couldn't find nested atom\n");
		/* move file pointer to just past current atom header*/
		fseeko(fpin, currAtom->atomHeader->saveSeek, SEEK_SET);
		fseeko(fpin, 8, SEEK_CUR);
		atomSize = currAtom->atomHeader->size - 8;
		currAtom++;
	}
	
	return (nestedAtoms);
	
}

/*
 *
 * Function: mp4FindAtomHeader
 * Purpose:	 Find an MP4 atom header within an MP4 file
 *           Atom header format is :
 *               size: 4-bytes (0=to EOF, 1=extended (64-bit) size)
 *               type: 4-bytes
 *
 *           The file pointer is returned to it's original position
 * Passed:   File pointer (should point to file location to start search)
 *			 Type of atom to find
 *			 Location to end searching
 * Returns:  Pointer to header structure or NULL if error
 */


MP4_ATOM_HEADER *mp4FindAtomHeader (FILE *fpin, char *atomType, unsigned long endSeek)
{
	MP4_ATOM_HEADER *atomHeader = calloc(sizeof(MP4_ATOM_HEADER), 1);
	unsigned long	saveSeek, bytesLeft;			

	saveSeek = ftello(fpin);
	
	bytesLeft = endSeek - saveSeek;
	
	/* read atoms until finding the atom type or end of search */
	atomHeader = mp4ReadAtomHeader(fpin);
	while (strncmp(atomHeader->type, atomType, 4) != 0) {
		if ( bytesLeft && (atomHeader->size)) {					
			bytesLeft -= atomHeader->size;
			fseeko(fpin, atomHeader->size, SEEK_CUR);
		} else {				// size of 0 means atom lasts until EOF, so we didn't find atom type
			/* return file pointer to original position */
			fseeko(fpin, saveSeek, SEEK_SET);
			return (NULL);		
		}
		atomHeader = mp4ReadAtomHeader(fpin);
	}
		
	fseeko(fpin, saveSeek, SEEK_SET);
    return (atomHeader);
	
}

/*
 *
 * Function: mp4ReadAtomHeader
 * Purpose:	 Read an MP4 atom header
 *           Atom header format is :
 *               size: 4-bytes (0=to EOF, 1=extended (64-bit) size)
 *               type: 4-bytes
 *
 *           The file pointer is returned to it's original position
 * Passed:   File pointer (should point to start of atom)
 * Returns:  Pointer to header structure or NULL if error
 */


MP4_ATOM_HEADER *mp4ReadAtomHeader (FILE *fpin)
{
	MP4_ATOM_HEADER *atomHeader = calloc(sizeof(MP4_ATOM_HEADER), 1);
	unsigned char	header[MP4_HEADER_SIZE];
	unsigned long   atomSize;

 
	atomHeader->saveSeek = ftello(fpin);
	err = fread(header, 1L, MP4_HEADER_SIZE, fpin);
	uitsHandleErrorINT(mp4ModuleName, "mp4ReadAtomHeader", err, MP4_HEADER_SIZE, "Couldn't read mp4 atom header\n");

	
//	printf("%c%c%c%c\n", header[4], header[5], header[6], header[7]);
	
	memcpy (atomHeader->type, &header[4], 4); 

	memcpy(&atomSize, &header[0], 4);
	lswap(&atomSize);
	atomHeader->size = atomSize;

    /* check for extended (64 bit) atom size */
    if (atomSize == 1)
    {
		uitsHandleErrorINT(mp4ModuleName, "mp4ReadAtomHeader", err, 1, "Unsupported extended size atom found in file\n");

	}
	
	/* return file pointer to original position */
	
	fseeko(fpin, atomHeader->saveSeek, SEEK_SET);
	
    return (atomHeader);
}

/*
 *
 * Function: mp4UpdateChunkOffsetTable
 * Purpose:	 Update the chunk offset table in the moov chunk to reflect
 *           the added payload XML.
 *           The chunk offset table is contained within the following
 *           hierarchy in the MP4 file:
 *           'moov' - Movie
 *              'trak'  - Track
 *                 'minf'  - Media Information
 *                    'stbl'  - Sample Table
 *                       'stco'  - Chunk Offset Table
 * Passed:   Input File pointer, size of UITS atom
 * Returns:  OK or ERROR
 */

int mp4UpdateChunkOffsetTable(FILE *fpout, int uitsAtomSize)
{
	MP4_NESTED_ATOM nestedAtoms [] = {
		{ "moov", NULL },
		{ "trak", NULL },
		{ "mdia", NULL },
		{ "minf", NULL },
		{ "stbl", NULL },
		{ "stco", NULL },
		{ NULL, 0}
	};
	MP4_NESTED_ATOM *chunkTable = NULL;
	unsigned int saveSeek;
	unsigned int atomSize;
	unsigned int audioOutFileSize;
	unsigned long numChunkEntries;
	unsigned long chunkOffset;
	int i;
	
	saveSeek = ftello(fpout);
	rewind(fpout);

	audioOutFileSize = uitsGetFileSize(fpout);
	
	/* populate the nested atom pointers */
	chunkTable = mp4FindAtomHeaderNested(fpout, nestedAtoms);
	
	/* find the chunk offset table within the nested atoms */
	while (	strcmp(chunkTable->atomType, "stco") != 0) {
		chunkTable++;
	}
	/* seek to the beginning of the chunk offset table */
	fseeko(fpout, chunkTable->atomHeader->saveSeek, SEEK_SET);
	fseeko(fpout, 8, SEEK_CUR); /* seek past the atom size and type */
	fseeko(fpout, 1, SEEK_CUR); /* seek past the version */
	fseeko(fpout, 3, SEEK_CUR);	/* seek past the flags bytes */
	fread(&numChunkEntries, 1, 4, fpout);
	lswap(&numChunkEntries);

	/* now, read each of the chunk sizes, add uits chunk size, and rewrite */
	for (i=0; i<numChunkEntries; i++) {
		fread(&chunkOffset, 1, 4, fpout);
		lswap(&chunkOffset);
		chunkOffset += uitsAtomSize;
		lswap(&chunkOffset);
		fseeko(fpout, -4, SEEK_CUR);	/* seek back to overwrite*/
		fwrite(&chunkOffset, 1, 4, fpout);
		fseeko(fpout, 0, SEEK_CUR);  /* must do a seek after write and before read */
	}
	
	/* return fp to original position */
	fseeko(fpout, saveSeek, SEEK_SET);
	return (OK);
}	
	

/*
 *
 * Function: mp4CopyAtom
 * Purpose:	 Copy an Atom from one file to another
 *           The file pointers are left at the end of the copied atoms
 * Passed:   Input File pointer (should point to start of atom)
 *           Output File pointer (should point to copy location)
 * Returns:  Number of bytes copied
 */


int mp4CopyAtom (FILE *fpin, FILE *fpout)
{
	MP4_ATOM_HEADER *atomHeader = NULL;
	unsigned int saveSeek;
	unsigned int atomSize;

	atomHeader = mp4ReadAtomHeader(fpin);
	atomSize = atomHeader->size;
	
	if (atomSize == 0) { /* size of 0 means atom goes to end of file */
		/* calculate how long the input audio file is by seeking to EOF and saving size  */
		saveSeek = ftello(fpin);
		fseeko(fpin, 0L, SEEK_END);
		atomSize = ftello(fpin);	
		fseeko(fpin, saveSeek, SEEK_SET); /* Back to start of atom */
		/* since there's a good chance we're going to be appending a */
		/* udta atom, write the actual size instead of copying the 0 size */
		lswap(&atomSize);
		fwrite(&atomSize, 1, 4, fpout);
		fseeko(fpin, 4, SEEK_CUR);		/* seek past the old size */
		uitsAudioBufferedCopy(fpin, fpout, (atomSize - 4));
	} else {
		uitsAudioBufferedCopy(fpin, fpout, atomSize);
	}

	return (atomSize);
	
}

// EOF