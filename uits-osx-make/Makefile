#
# Project: UMG Unique Identifier Technology Solution (UITS) Tool
# File:    Makefile
#
# This makefile will build the UITS Tool from the command line. It
# has been tested to run under MacOSX version 10.6.2.
#  
#  $Date$
#  $Revision$
# 

#
# Compiler tools definitions...
#

CC      = gcc
OPTIM   = -Os -g -arch i386
CFLAGS  = $(OPTIM) -I /usr/include/libxml2 -I openssl/include -I mxml/include
LDFLAGS = $(OPTIM)
OBJECTS = main.o uitsAudioFileManager.o uitsMP3Manager.o uitsPayloadManager.o uitsOpenSSL.o
RM = rm

#
# Rules...
#
#

# the source files are up a directory
vpath %.c ../source

#
# UITS_Tool Target
#

UITS_Tool: $(OBJECTS)
	$(CC) $(LDFLAGS) -o UITS_Tool   $(OBJECTS) mxml/lib/libmxml.a -lxml2 openssl/lib/libcrypto_1.0.0-beta4.a openssl/lib/libssl_1.0.0-beta4.a  


#
# UITS_Tool objects
#

.o : %.c
	$(CC) $(CFLAGS) -c $<


#
# Clean everything...
#

clean:
	$(RM) $(OBJECTS)

#
# End 
#
