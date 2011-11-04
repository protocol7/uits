#
# Project: UMG Unique Identifier Technology Solution (UITS) Tool
# File:    windows.mak
#
# This makefile will build a Windows version of the UITS Tool. This
# file should be run under MinGW. This makefile resides in a sub-directory
# of the project directory which contains all of the .c and .h files.
#  
# 5/13/10: MinGW does not support 'ftello' or 'fseeko', so in order to compile
#          the fseeko/ftello calls are replaced with calls to fseek/ftell via
#          compile time defines (-Dfseeko-fseek). This will cause problems with
#          audio files that are larger than 2GB.
#  $Date$
#  $Revision$
# 

#
# Compiler tools definitions...
#

CC      = gcc
OPTIM   = -Os -g -Dfseeko=fseek -Dftello=ftell -DNO_UUID
CFLAGS  = $(OPTIM) -I libxml2/include -I ssl/include -I mxml/include -I FLAC/include
LDFLAGS = $(OPTIM) -luuid
OBJECTS = main.o xmlManager.o cmePayloadManager.o uitsAudioFileManager.o uitsMP3Manager.o uitsMP4Manager.o uitsPayloadManager.o uitsOpenSSL.o uitsAIFFManager.o uitsFLACManager.o uitsWAVManager.o uitsError.o uitsGenericManager.o uitsHTMLManager.o uitsWindows_uuid_parse.o uitsWindows_strcasestr.o

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

UITS_Tool.exe :$(OBJECTS)
	$(CC) $(LDFLAGS) -o UITS_Tool.exe   $(OBJECTS) mxml/lib/libmxml.a libxml2/lib/libxml2.a FLAC/lib/libFLAC.a -lwsock32 ssl/lib/libcrypto.a -lgdi32 ssl/lib/libssl.a 


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
