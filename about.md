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
- When I run the parquet cli on my WSL Debian instance, I need to make sure to prepend the parquet-mr/parquet-cli path and be sure that I'm in the local directory of the parquet file
- On my WSL machine, I've been testing the file /home/jon/projects/parquet-mr/parquet-thrift/target/out/part-m-00000.parquet
- Metadata is encoded using TCompactProtocol
  - Described here: <https://github.com/apache/thrift/blob/master/doc/specs/thrift-compact-protocol.md/>
  - Uses ULEB128 encoding
    - Ints divided into groups of 7 bits
    - From LSB -> MSB
    - 8th bit is used to indicate whether more bytes are necessary
    - MSB == 1 = more data. MSB == 0 = no more data
  - I think the Metadata is mostly Enums in a list in a struct
  - Int encoding:
    Values of type int8 are encoded as one byte, rest are converted to int64, ZigZag'ed then encoded as varint
  - Struct encoding:
    - 0 or more fields followed by a stop field
      - each field contains a field-type and a field-id, because of this they can be listed in any order
  
- I added a CMakeLists.txt file so that CLion can build and run the project in the UI
- I downloaded HxD to read the hex of the test parquet file I've been using
  - It looks like the footer is 1047 bytes long