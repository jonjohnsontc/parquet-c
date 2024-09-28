/*
  Reads in footer of parquet file, and returns its length in bytes
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
/*
   Ganked from GCC leb128.h - code is GPL V2 Licensed
   Decode the signed LEB128 constant at BUF into the variable pointed to
   by R, and return the number of bytes read.
   If we read off the end of the buffer, zero is returned,
   and nothing is stored in R.

   Note: The result is an int instead of a pointer to the next byte to be
   read to avoid const-vs-non-const problems.  */
static inline size_t
read_sleb128_to_int64(const unsigned char *buf, const unsigned char *buf_end,
                      int64_t *r);
int zigzag_decode(const unsigned char * buf, size_t length, char*dest);
void safe_read_byte(void *buffer, FILE *stream, size_t no_items);
void read_meta(char *meta, char *res, FILE *stream, size_t length);

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

    FILE *parquet = fopen(argv[1], "rb");
    if (parquet == NULL) {
        perror("Error opening file");
        exit(1);
    }

    fseek(parquet, 0, SEEK_END);
    long file_size = ftell(parquet);

    if (file_size < 8) {
        fprintf(stderr, "File is too small - def not a proper parquet file\n");
        fclose(parquet);
    }

    // Go to the location of the length of the metadata file
    fseek(parquet, -8, SEEK_END);

    uint8_t buffer[4];
    safe_read_byte(buffer, parquet, 4);

    long no_items = (uint32_t) buffer[0] |
                    ((uint32_t) buffer[1] << 8) |
                    ((uint32_t) buffer[2] << 16) |
                    ((uint32_t) buffer[3] << 24);
    printf("Length of metadata is %ld bytes\n", no_items);

    //  We'll copy and invoke the read_footer function
    fseek(parquet, -(no_items + 8), SEEK_END);

    char *meta = malloc(no_items);
    if (meta == NULL) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    char *dest = malloc(no_items);
    if (dest == NULL) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    read_meta(meta, dest, parquet, no_items);
    fclose(parquet);

    const unsigned char bytes[] = {0xC0, 0xBB, 0x78};
    const unsigned char *end = bytes + 4;
    int64_t res = 0;
    read_sleb128_to_int64(bytes, end, &res);
    printf("bytes equal %ld\n", res);

    free(meta);
    free(dest);
    return 0;
}

static inline size_t
read_sleb128_to_int64(const unsigned char *buf, const unsigned char *buf_end,
                      int64_t *r) {
    const unsigned char *p = buf;
    unsigned int shift = 0;
    int64_t result = 0;
    unsigned char byte;

    while (1) {
        if (p >= buf_end)
            return 0;

        byte = *p++;
        result |= ((uint64_t) (byte & 0x7f)) << shift;
        shift += 7;
        if ((byte & 0x80) == 0)
            break;
    }
    if (shift < (sizeof(*r) * 8) && (byte & 0x40) != 0)
        result |= -(((uint64_t) 1) << shift);

    *r = result;
    return p - buf;
}

void safe_read_byte(void *buffer, FILE *stream, size_t no_items) {
    size_t len;
    if ((len = fread(buffer, 1, no_items, stream)) != no_items) {
        perror("Error reading file");
        printf("Bytes read: %zu\n", len);
        printf("File is currently at: %ld\n", ftell(stream));
        printf("Contents of buffer: %s\n", (char *) buffer);
        fclose(stream);
        exit(1);
    }
}

void read_meta(char *meta, char *res, FILE *stream, size_t length) {
    safe_read_byte(meta, stream, length);
    int chars_written = zigzag_decode((const unsigned char*)meta, length, res);
    printf("Chars written when decoding %d\n", chars_written / 4);
    printf("Parquet Metadata:\n");
    printf("%s\n", res);
    for (int i = 0; i < length; i++)
        printf("%c", meta[i]);
    printf("\n");
}

int zigzag_decode(const unsigned char * buf, size_t length, char*dest) {
    const unsigned char * cursor = buf;
    int written = 0;
    int64_t res;
    while (cursor < buf + length) {
        const size_t amt = read_sleb128_to_int64(cursor, cursor + 4, &res);
        cursor += amt;
        written++;
        *dest++ = res;
    }
    return written;
}
