FILE *fp;
long lSize;
char *buffer;

fp = fopen ( "plates.txt" , "rb" );
if( !fp ) perror("plates.txt"),exit(1);

fseek( fp , 0L , SEEK_END);
lSize = ftell( fp );
rewind( fp );

/* memory allocation */
buffer = calloc( 1, lSize+1 );
if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

/* copy file into the buffer */
if( 1!=fread( buffer , lSize, 1 , fp) )
  fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);


fclose(fp);
free(buffer);