#include <cmath>
#include <cstdio>
#include <sys/stat.h>
#include <cstring>

/*
* Structure that defines the old tar header (Tar V7)
*/
typedef struct posix_header
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char filler[255];             /* 257 */
  /* ENDS AT 512 */
} TARV7_HEADER;

// The header is 512 bytes, as per the TARV7 standard
#define HEADER_SIZE 512
// This means the data size is also 512 bytes to make the 1024 byte minimum
#define DATA_SIZE 512

/*
* Convert a number to the octal number base
*/
long toOctal(long n) {
  long oct = 0, j = 0;
  while(n > 0) {
    oct += (pow(10,j++) * (n % 8));
    n /= 8;
  }
  
  return oct;
}

int main(int argc, char * argv[]) {
  // basic command line exception
  if(argc != 3) {
    printf("usage: FastTarV7 [archive{.tar}] [file]\n");
    return 0;
  }
  
  size_t sz = 0; // holds the size
  
  // get input file size
  FILE * in = fopen(argv[2], "rb");
  if(in == NULL) {printf("ERROR: Input file doesn't exist.\n"); return -1;} // check if file exists
  fseek(in, 0L, SEEK_END); // seek to end
  sz = toOctal(ftell(in)); // get the file size in octal
  fclose(in);
  
  // get file stats
  struct stat info;
  stat(argv[2], &info);
  
  // create the TarV7 header
  TARV7_HEADER tarv7header = {0}; // initialize all values to null
  strcpy(tarv7header.name, argv[2]); // copy target file name into header
  sprintf(tarv7header.mode, "%07u", info.st_mode); // file mode
  sprintf(tarv7header.uid, "%07u", info.st_uid); // file UID
  sprintf(tarv7header.gid, "%07u", info.st_gid); // file GID
  sprintf(tarv7header.size, "%011lu", sz); // file size
  sprintf(tarv7header.mtime, "%011ld", toOctal(info.st_mtime)); // last modification time
  strcpy(tarv7header.chksum, "        "); // checksum is initialized to spaces by default!
  tarv7header.typeflag = '0';
  
  int checksum = 0;
  
  // calculate the checksum of the header!
  for(int i = 0;i < sizeof(tarv7header);i++) {
    checksum += ((unsigned char *)&tarv7header)[i];
  }
  
  // convert the checksum to octal format
  checksum = toOctal(checksum);
  
  sprintf(tarv7header.chksum, "%07d", checksum); // copy the octal checksum into the checksum part of the header
  
  char footer[HEADER_SIZE * 2] = {0}; // we need this footer on the TAR file for it to be proper!
  // it's a full chunk
  
  char buf[DATA_SIZE]; // holds the data, we work in DATA_SIZE byte chunks since it's very convenient
  
  // create/update the tar archive
  FILE * tar = fopen(argv[1], "rb+");
  if(tar == NULL) tar = fopen(argv[1], "wb"); // this means we have to create a new tar
  
  fseek(tar, -(HEADER_SIZE * 2), SEEK_END); // over-write the EOF indicator with each creation/update
  fwrite(&tarv7header, sizeof(tarv7header), 1, tar); // write ALL of the header
  
  // time to write the data in
  in = fopen(argv[2], "rb"); // open the target file
  for(;;) { // start reading
    fread(buf, sizeof(char), DATA_SIZE, in);
    fwrite(buf, sizeof(char), DATA_SIZE, tar);
    if(feof(in)) {
      break;
    }
  }
  fclose(in); // close the file
  
  fwrite(footer, sizeof(char), (HEADER_SIZE * 2), tar); //lastly, the EOF footer
  fclose(tar); // close the tar
  
  return 0;
}