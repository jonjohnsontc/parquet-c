//
// Created by Jon Johnson on 10/6/24.
//

#include "parquet.h"

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
