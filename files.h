/*
* FS_ChecksumAbsoluteFile
*/
unsigned FS_ChecksumAbsoluteFile( const char *filename );

/*
* FS_LoadPK3File
*.
* Takes an explicit (not game tree related) path to a pak file.
*.
* Loads the header and directory, adding the files at the beginning
* of the list so they override previous pack files.
*/
unsigned FS_LoadPK3File( const char *packfilename );
