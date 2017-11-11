#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

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
// These are all defined to allow for a custom implementation if it's so desired to have big chunks

// we define all of this stuff for compatiblity with Windows, but technically you could get the stats and figure all of this out on a unix/Posix system (TODO: ADD WIN VS NIX BUILD)
#define FILE_MODE "0000664"
#define FILE_UID "0001750"
#define FILE_GID "0001750"

/*
* Convert a number to the octal number base
*/
int toOctal(int n) {
  int oct = 0, j = 0;
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
  
  size_t sz = 0;
  int dp = 1; // data pointer
  char * data = (char *)calloc(DATA_SIZE * dp, sizeof(char)); // allocate 512 bytes for data
  
  // let's get the input file's size + data
  FILE * in = fopen(argv[2], "rb");
  fseek(in, 0L, SEEK_END);
  sz = ftell(in); // don't need octal yet since we're still reading
  rewind(in); // rewind the file
  // let's get the data now
  while(sz > (DATA_SIZE * dp)) { // this means we need more space
    free(data); // free up the OLD memory pointer
    data = (char *)calloc(DATA_SIZE * (++dp), sizeof(char)); // allocate MORE data!!
  }
  fread(data, sizeof(char), sz, in);
  fclose(in);
  
  sz = toOctal(sz); // we need to turn it into octal before it ends up on the header
  
  // first thing's first, let's prepare the header
  TARV7_HEADER tarv7header = {0}; // start everything NULLED out
  strcpy(tarv7header.name, argv[2]); // copy the target file into the new header
  strcpy(tarv7header.mode, FILE_MODE);
  strcpy(tarv7header.uid, FILE_UID);
  strcpy(tarv7header.gid, FILE_GID);
  sprintf(tarv7header.size, "%011d", (int)sz);
  strcpy(tarv7header.mtime, "13201450112");
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
  
  
  //strcpy(data, "This is a file that I want to tar up"); // get the data in there!
  
  char footer[HEADER_SIZE * 2] = {0}; // we need this footer on the TAR file for it to be proper!
  // it's a full chunk
  
  // create/update the tar archive
  FILE * tar = fopen(argv[1], "rb+");
  if(tar == NULL) tar = fopen(argv[1], "wb"); // this means we have to create a new tar
  
  fseek(tar, -(HEADER_SIZE * 2), SEEK_END); // over-write the EOF indicator with each creation/update
  fwrite(&tarv7header, sizeof(tarv7header), 1, tar); // write ALL of the header
  fwrite(data, sizeof(char), DATA_SIZE * dp, tar); // write ALL of the data!
  fwrite(footer, sizeof(char), (HEADER_SIZE * 2), tar); //lastly, the EOF footer
  fclose(tar);
  
  // free resources
  free(data);
  return 0;
}