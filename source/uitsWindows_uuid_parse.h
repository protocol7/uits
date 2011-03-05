/*
 *  uitsWindows_uuid_parse.h
 *  uits-osx-xcode
 *
 *  Created by Chris Angelli on 1/27/11.
 *  Copyright 2011 UMG. All rights reserved.
 *
 */

#ifndef __uitsWindows_uuid_parse_h
#define __uitsWindows_uuid_parse_h
typedef	unsigned char __mingw_uuid_t[16];
typedef __mingw_uuid_t	uuid_t;
//typedef unsiged char uuid_t[16];
int	uuid_parse(const char *in, uuid_t uu);
#endif
