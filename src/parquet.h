/*
 * Parquet parsing library code
 */
#ifndef PARQUET_H
#define PARQUET_H
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

#endif //PARQUET_H
