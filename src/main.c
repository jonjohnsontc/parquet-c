/*
  Reads in footer of parquet file, and returns its length in bytes
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
  return 0;
}