/*
  Reads in parquet file and returns it's parsed metadata
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
void safe_read_bytes(void *buffer, FILE *stream, size_t no_items);
void read_meta(const unsigned char *meta, FILE *stream, size_t length);
int64_t zigzag_to_long(uint64_t n);
size_t tc_read_to_long(const unsigned char *buf, const unsigned char *buf_end, int64_t *res);

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
    safe_read_bytes(buffer, parquet, 4);

    long no_items = (uint32_t) buffer[0] |
                    ((uint32_t) buffer[1] << 8) |
                    ((uint32_t) buffer[2] << 16) |
                    ((uint32_t) buffer[3] << 24);
    printf("Length of metadata is %ld bytes\n", no_items);

    //  We'll copy and invoke the read_footer function
    fseek(parquet, -(no_items + 8), SEEK_END);
    printf("Parquet Metadata Index: %ld\n", ftell(parquet));

    const unsigned char *meta = malloc(no_items);
    if (meta == NULL) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    char *dest = malloc(no_items);
    if (dest == NULL) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    read_meta(meta, parquet, no_items);
    fclose(parquet);

    // TODO: I want to replace this with trying to read the first bytes in the metadata as varint
    // TODO: also I want to include the initial example in a test for read_sleb128_to_int64
    const unsigned char bytes[] = {0xC0, 0xBB, 0x78};
    const unsigned char *end = bytes + 4;
    int64_t res = 0;
    read_sleb128_to_int64(bytes, end,&res);
#ifdef __linux__
    printf("bytes equal %ld\n", res);
    printf("zig zag bytes equal %ld\n", zig_zag);
#endif
#ifdef __APPLE__
    printf("bytes equal %lld\n", res);
#endif
    free((void *)meta);
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

    // decode SLEB128
    while (1) {
        if (p >= buf_end)
            return 0;

        byte = *p++;
        result |= ((uint64_t) (byte & 0x7f)) << shift;
        shift += 7;
        if ((byte & 0x80) == 0)
            break;
    }
    // handle sign extension
    if (shift < (sizeof(*r) * 8) && (byte & 0x40) != 0)
        result |= -(((uint64_t) 1) << shift);

    *r = result;
    return p - buf;
}

void safe_read_bytes(void *buffer, FILE *stream, size_t no_items) {
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

void read_meta(const unsigned char *meta, FILE *stream, size_t length) {
    int64_t version, schema_length;
    size_t chars_read;
    const unsigned char *end_of_buffer;

    safe_read_bytes((void*)meta, stream, length);
    end_of_buffer = meta + length;

    // Get Version
    // I want to be able to read the version from the byte sequence and
    // advance the buffer to the next character.
    chars_read = tc_read_to_long(meta, end_of_buffer, &version);
    printf("Parquet Version: %lld\n", version);

    // Begin reading in SchemaElement list
    const unsigned char *after_version_read = meta + chars_read;
    chars_read = tc_read_to_long(after_version_read, end_of_buffer, &schema_length);
    printf("Length of schema: %lld\n", schema_length);

//    printf("Parquet Metadata:\n");
//    for (int i = 0; i < length; i++)
//        printf("%c", meta[i]);
    printf("\n");
}

int64_t zigzag_to_long(uint64_t n) {
    return (int64_t)((n >> 1) ^ (-(n & 1)));
}

/*
 * Reads in SLEB128 as a long int and zigzag decodes it after, per the TCompactProtocol. Stores
 * the result in pointer passed through, and returns the number of characters read from the input
 * buffer.
 */
size_t tc_read_to_long(const unsigned char *buf, const unsigned char *buf_end, int64_t *res) {
   int64_t container;
   size_t chars_read;
   chars_read = read_sleb128_to_int64(buf, buf_end, &container);
   if (chars_read == 0) {
       fprintf(stderr, "Zero bytes read\n");
       exit(1);
   }
   printf("Container is %lld\n", container);
   *res = zigzag_to_long(container);
   return chars_read;
}
