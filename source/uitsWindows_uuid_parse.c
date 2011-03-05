/*
 *  uitsWindows_uuid_parse.c
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 1/27/11.
 *  Copyright 2011 UMG. All rights reserved.
 *
 *  The UITS project uses uuid's for MP4 files, however MinGW does not have a uuid library
 *  installed by default. This file implents the small set of uuid functionality required for UITS.
 *
 */

#include "uits.h"


/*
 *
 * Function: uuid_parse
 * Purpose:	 This is a complete hack to deal with the fact that my MinGW installation does not have
 *			 the uuid toolset installed and I don't have time to install it. Hard-code the uuid_parse
 *			 function to return the UITS uuid.
 *
 * Returns:   OK or ERROR
 */

/*
 * BASED ON gen_uuid.c --- generate a DCE-compatible uuid
 *
 * Copyright (C) 1996, 1997, 1998, 1999 Theodore Ts'o.
 *
 * %Begin-Header%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * %End-Header%
 */


int	uuid_parse(const char *in, uuid_t uu)
{
	int             i;
	const char      *cp;
	char            buf[3];
	long int		time_low;
	long int		time_mid;
	long int		time_hi_and_version;
	long int		clock_seq;
	long int		node[3];
	unsigned char	*out = uu;
	
	//	char *uitsUUIDString = "99454E27-963A-4B56-8E76-1DB68C899CD4";
	
	/* make sure string is valid uuid format */
	if (strlen(in) != 36)
		return ERROR;
	
	for (i=0, cp = in; i <= 36; i++,cp++) {
		if ((i == 8) || (i == 13) || (i == 18) ||(i == 23)) {
			if (*cp == '-')
				continue;
			else
				return ERROR;
		}
		if (i== 36)
			if (*cp == 0) continue;
		if (!isxdigit(*cp))	return ERROR;
	}	
	/* */
	time_low = strtoul(in, NULL, 16);
	out[3] = (unsigned char) time_low;
	time_low >>= 8;
	out[2] = (unsigned char) time_low;
	time_low >>= 8;
	out[1] = (unsigned char) time_low;
	time_low >>= 8;
	out[0] = (unsigned char) time_low;
	
	time_mid = strtoul(in+9, NULL, 16);
	out[5] = (unsigned char) time_mid;
	time_mid >>= 8;
	out[4] = (unsigned char) time_mid;
	
	time_hi_and_version = strtoul(in+14, NULL, 16);
	out[7] = (unsigned char) time_hi_and_version;
	time_hi_and_version >>= 8;
	out[6] = (unsigned char) time_hi_and_version;
	
	clock_seq = strtoul(in+19, NULL, 16);
	out[9] = (unsigned char) clock_seq;
	clock_seq >>= 8;
	out[8] = (unsigned char) clock_seq;
	
	cp = uu+24;
	buf[2] = 0;
	for (i=0; i < 6; i++) {
		buf[0] = *cp++;
		buf[1] = *cp++;
		node[i] = strtoul(buf, NULL, 16);
	}
	memcpy(out+10, node, 6);
	
	return OK;
}


// EOF
