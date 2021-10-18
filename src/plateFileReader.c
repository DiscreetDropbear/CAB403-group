FILE *fn;
long lSize;
char *buffer;

fn = fopen ( "plates.txt" , "rb" );
if( !fn ) perror("plates.txt"),exit(1);

fseek( fn , 0L , SEEK_END);
lSize = ftell( fn );
rewind( fn );

/* memory allocation */
buffer = calloc( 1, lSize+1 );
if( !buffer ) fclose(fn),fputs("memory alloc fails",stderr),exit(1);

/* copy file into the buffer */
if( 1!=fread( buffer , lSize, 1 , fn) )
  fclose(fn),free(buffer),fputs("entire read fails",stderr),exit(1);


fclose(fn);
free(buffer);