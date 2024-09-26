/*
  Reads in footer of parquet file, and returns its length in bytes
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


/* 
 * Ganked from GCC leb128.h - code is GPL V2 Licensed
 * Decode the signed LEB128 constant at BUF into the variable pointed to
   by R, and return the number of bytes read.
   If we read off the end of the buffer, zero is returned,
   and nothing is stored in R.

   Note: The result is an int instead of a pointer to the next byte to be
   read to avoid const-vs-non-const problems.  */
static inline size_t
read_sleb128_to_int64 (const unsigned char *buf, const unsigned char *buf_end,
		       int64_t *r)
{
  const unsigned char *p = buf;
  unsigned int shift = 0;
  int64_t result = 0;
  unsigned char byte;

  while (1)
    {
      if (p >= buf_end)
	return 0;

      byte = *p++;
      result |= ((uint64_t) (byte & 0x7f)) << shift;
      shift += 7;
      if ((byte & 0x80) == 0)
	break;
    }
  if (shift < (sizeof (*r) * 8) && (byte & 0x40) != 0)
    result |= -(((uint64_t) 1) << shift);

  *r = result;
  return p - buf;
}

/*
  - Read from text file at argv[1]
  - Get length of footer
  -
*/
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "USAGE: tiny-p FILENAME\n");
    exit(1);
  }

  FILE* parquet = fopen(argv[1], "rb");
  if (parquet == NULL) {
    perror("Error opening file");
    exit(1);
  }

  fseek(parquet, 0, SEEK_END);
  long file_size = ftell(parquet);
  if (file_size < 8) {
    fprintf(stderr, "File is too small - def not a proper parquet file\n");
    fclose(parquet);
    exit(1);
  }
  
  // Go to the location of the length of the metadata file
  fseek(parquet, -8, SEEK_END);

  uint8_t buffer[4];
  if (fread(buffer, 1, 4, parquet) != 4) {
    perror("Error reading file");
    fclose(parquet);
    exit(1);
  }

  fclose(parquet);

  uint32_t result = (uint32_t)buffer[0] |
                    ((uint32_t)buffer[1] << 8) |
                    ((uint32_t)buffer[2] << 16) |
                    ((uint32_t)buffer[3] << 24);
  printf("Length of metadata is %d bytes\n", result);
  printf("Size of int64_t is %ld bytes\n", sizeof(int64_t));
  printf("Size of long is %ld bytes\n", sizeof(long));

  printf("Parsing zigzag encoded bytestring\n");
  const unsigned char bytes[] = { 0xC0, 0xBB, 0x78 };
  const unsigned char* end = bytes + 4;
  int64_t res = 0;
  read_sleb128_to_int64(bytes, end, &res);
  printf("bytes equal %ld\n", res);
  return 0;
}
