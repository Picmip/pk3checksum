/*
Copyright (C) 2016 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <zlib.h>
#include "md5.h"

#define FS_ZIP_BUFREADCOMMENT	    0x00000400
#define FS_ZIP_SIZELOCALHEADER	    0x0000001e
#define FS_ZIP_SIZECENTRALDIRITEM   0x0000002e

#define FS_ZIP_LOCALHEADERMAGIC	    0x04034b50
#define FS_ZIP_CENTRALHEADERMAGIC   0x02014b50
#define FS_ZIP_ENDHEADERMAGIC	    0x06054b50

static inline unsigned int LittleLongRaw( const uint8_t *raw )
{
	return ( raw[3] << 24 ) | ( raw[2] << 16 ) | ( raw[1] << 8 ) | raw[0];
}

static inline unsigned short LittleShortRaw( const uint8_t *raw )
{
	return ( raw[1] << 8 ) | raw[0];
}

/*
* FS_FileLength
*/
static int FS_FileLength( FILE *f )
{
	size_t pos, end;

	pos = ftell( f );
	fseek( f, 0, SEEK_END );
	end = ftell( f );
	fseek( f, pos, SEEK_SET );

	return end;
}

/*
* FS_ChecksumAbsoluteFile
*/
unsigned FS_ChecksumAbsoluteFile( const char *filename )
{
	uint8_t buffer[128*1024];
	int left, length;
	md5_byte_t digest[16];
	md5_state_t state;
	FILE *file;

	md5_init( &state );

	file = fopen( filename, "rb" );
	if( !file )
		return 0;

	left = FS_FileLength( file );
	while( ( length = fread( buffer, 1, sizeof( buffer ), file ) ) )
	{
		left -= length;
		md5_append( &state, (md5_byte_t *)buffer, length );
	}

	fclose( file );
	md5_finish( &state, digest );

	if( left != 0 )
		return 0;

	return md5_reduce( digest );
}

/*
* FS_ChecksumPK3File
*/
static unsigned FS_ChecksumPK3File( const char *filename, int numPakFiles, int *checksums )
{
	md5_byte_t digest[16];
	md5_state_t state;
	int pakFileInd;

	md5_init( &state );

	for( pakFileInd = 0; pakFileInd < numPakFiles; pakFileInd++ )
	{
		int fixedChecksum = checksums[pakFileInd];
		md5_append( &state, (md5_byte_t *)&fixedChecksum, sizeof( fixedChecksum ) );
	}

	md5_finish( &state, digest );

	return md5_reduce( digest );
}

/*
* FS_PK3SearchCentralDir
*/
static unsigned FS_PK3SearchCentralDir( FILE *fin )
{
	unsigned fileSize, backRead;
	unsigned maxBack = 0xffff; // maximum size of global comment
	unsigned char buf[FS_ZIP_BUFREADCOMMENT+4];

	if( fseek( fin, 0, SEEK_END ) != 0 )
		return 0;

	fileSize = ftell( fin );
	if( maxBack > fileSize )
		maxBack = fileSize;

	backRead = 4;
	while( backRead < maxBack )
	{
		unsigned i, readSize, readPos;

		if( backRead + FS_ZIP_BUFREADCOMMENT > maxBack )
			backRead = maxBack;
		else
			backRead += FS_ZIP_BUFREADCOMMENT;

		readPos = fileSize - backRead;
		readSize = backRead;
		if( readSize > FS_ZIP_BUFREADCOMMENT + 4 )
			readSize = FS_ZIP_BUFREADCOMMENT + 4;
		if( readSize < 4 )
			continue;

		if( fseek( fin, readPos, SEEK_SET ) != 0 )
			break;
		if( fread( buf, 1, readSize, fin ) != readSize )
			break;

		for( i = readSize - 3; i--; )
		{
			// check the magic
			if( LittleLongRaw( buf + i ) == FS_ZIP_ENDHEADERMAGIC )
				return readPos + i;
		}
	}

	return 0;
}


/*
* FS_PK3GetFileInfo
* 
* Get Info about the current file in the zipfile, with internal only info
*/
static unsigned FS_PK3GetFileInfo( FILE *f, unsigned pos, unsigned byteBeforeTheZipFile, int *crc )
{
	size_t sizeRead;
	unsigned compressed;
	unsigned char infoHeader[46]; // we can't use a struct here because of packing

	if( fseek( f, pos, SEEK_SET ) != 0 )
		return 0;
	if( fread( infoHeader, 1, sizeof( infoHeader ), f ) != sizeof( infoHeader ) )
		return 0;

	// check the magic
	if( LittleLongRaw( &infoHeader[0] ) != FS_ZIP_CENTRALHEADERMAGIC )
		return 0;

	compressed = LittleShortRaw( &infoHeader[10] );
	if( compressed && ( compressed != Z_DEFLATED ) )
		return 0;

	if( crc )
		*crc = LittleLongRaw( &infoHeader[16] );

	sizeRead = ( size_t )LittleShortRaw( &infoHeader[28] );
	if( !sizeRead )
		return 0;

	return FS_ZIP_SIZECENTRALDIRITEM + ( unsigned )LittleShortRaw( &infoHeader[28] ) +
		( unsigned )LittleShortRaw( &infoHeader[30] ) + ( unsigned )LittleShortRaw( &infoHeader[32] );
}

/*
* FS_LoadPK3File
* 
* Takes an explicit (not game tree related) path to a pak file.
* 
* Loads the header and directory, adding the files at the beginning
* of the list so they override previous pack files.
*/
unsigned FS_LoadPK3File( const char *packfilename )
{
	int i;
	int *checksums = NULL;
	int numFiles;
	FILE *fin = NULL;
	unsigned char zipHeader[20]; // we can't use a struct here because of packing
	unsigned offset, centralPos, sizeCentralDir, offsetCentralDir, byteBeforeTheZipFile;
	void *handle = NULL;
	unsigned checksum;
	unsigned silent = 0;

	fin = fopen( packfilename, "rb" );
	if( fin == NULL )
	{
		if( !silent ) fprintf( stderr, "Error opening PK3 file: %s\n", packfilename );
		goto error;
	}
	centralPos = FS_PK3SearchCentralDir( fin );
	if( centralPos == 0 )
	{
		if( !silent ) fprintf( stderr, "No central directory found for PK3 file: %s\n", packfilename );
		goto error;
	}
	if( fseek( fin, centralPos, SEEK_SET ) != 0 )
	{
		if( !silent ) fprintf( stderr, "Error seeking PK3 file: %s\n", packfilename );
		goto error;
	}
	if( fread( zipHeader, 1, sizeof( zipHeader ), fin ) != sizeof( zipHeader ) )
	{
		if( !silent ) fprintf( stderr, "Error reading PK3 file: %s\n", packfilename );
		goto error;
	}

	// total number of entries in the central dir on this disk
	numFiles = LittleShortRaw( &zipHeader[8] );
	if( !numFiles )
	{
		if( !silent ) fprintf( stderr,"%s is not a valid pk3 file\n", packfilename );
		goto error;
	}
	if( LittleShortRaw( &zipHeader[10] ) != numFiles || LittleShortRaw( &zipHeader[6] ) != 0
		|| LittleShortRaw( &zipHeader[4] ) != 0 )
	{
		if( !silent ) fprintf( stderr, "%s is not a valid pk3 file\n", packfilename );
		goto error;
	}

	// size of the central directory
	sizeCentralDir = LittleLongRaw( &zipHeader[12] );

	// offset of start of central directory with respect to the starting disk number
	offsetCentralDir = LittleLongRaw( &zipHeader[16] );
	if( centralPos < offsetCentralDir + sizeCentralDir )
	{
		if( !silent ) fprintf( stderr, "%s is not a valid pk3 file\n", packfilename );
		goto error;
	}
	byteBeforeTheZipFile = centralPos - offsetCentralDir - sizeCentralDir;

	// allocate temp memory for files' checksums
	checksums = ( int* )malloc( ( numFiles + 1 ) * sizeof( *checksums ) );

	// add all files to the trie
	for( i = 0, centralPos = offsetCentralDir + byteBeforeTheZipFile; i < numFiles; i++, centralPos += offset )
	{
		offset = FS_PK3GetFileInfo( fin, centralPos, byteBeforeTheZipFile, &checksums[i] );
	}

	fclose( fin );
	fin = NULL;

	checksums[numFiles] = 0x1234567; // add some pseudo-random stuff
	checksum = FS_ChecksumPK3File( packfilename, numFiles + 1, checksums );

	if( !checksum )
	{
		if( !silent ) fprintf( stderr, "Couldn't generate checksum for pk3 file: %s\n", packfilename );
		goto error;
	}

	free( checksums );

	return checksum;

error:
	if( fin )
		fclose( fin );
	if( checksums )
		free( checksums );

	return 0;
}


