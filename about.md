# About

I'm creating this as an exercise to read a parquet file using C. When I say file, I'm not yet sure if this applies to entire file, or just the file metadata.

Goals:

- I would like to gain a better understanding of using C in exploratory programming
- To practice using C more
- To understand how to read standards or reference documentation
- Get practice at dealing with byte data

Non-Goals:

- Create an open source parquet library that everyone loves n uses

## Program Design

I would like some sort of basic thread of what I'm building before slapping down some code:

- I would like to be able to read parquet-metadata and return it to the console
- Column names, sizes of data, counts?
- Perhaps some sort of rudimentary query functionality, but that's probably crazy

As a first implementation, I would like to print each column name to stdout delimited by newline.

### Log

- I'm starting to look into the parquet file format from their docs at <https://parquet.incubator.apache.org/docs/file-format/>
  - The file metadata's length is stored in the 2nd to last 4-bytes of the document
  - The file metadata is stored in the bytes prior to that
  