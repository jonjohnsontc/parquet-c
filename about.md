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
- I'm using Hex Fiend on my mac

- I'm using a new file in the test directory as the parquet to reference: `test/gen_data/sp500_esg_data.parquet`
- When i print out the file metadata using the current version of my program, here's what I see:

```
Length of metadata is 1567 bytes
Parquet Metadata:
duckdb_schema
             %symbol%
                     %  full_name%
                                  %
                                   gics_sector%
                                               %gics_sub_industry%
%       env_score
%
 social_score
%       gov_score
%       total_esg%highest_controversy% 
%
percentile%
           rating_year% %
                         rating_month% %
market_cap%$
%beta%
      overall_risk% ��&
                       symbol��/� <ZTSA(ZTSA&
                                                full_name��x�U&� <eBay3M(eBay3M&
                                                                                %
                                                                                 gics_sector���
                                                                                               &�x&�v   UtilitiesCommunication Services UtilitiesCommunication Services&
                     %gics_sub_industry��@�-&��&�#Wireless Telecommunication Services
                                                                                     Advertising�#Wireless Telecommunication Services
                                                                                                                                     Advertising&
        env_score��5�&ޯ{�G��8{�G��8&

social_score��5�&��{�G�z6R���Q�?{�G�z6R���Q�?&
        gov_score��5�&���G�zn3�G�z�@�G�zn3�G�z�@&
        total_esg��5�&���G�z�DR���Q@�G�z�DR���Q@&highest_controversy���
&��<(&

percentile��5�!&����Q�NWR���Q�?��Q�NWR���Q�?&
                                             rating_year���&��<��(��&
                                                                     rating_month���&��<        (       &
market_cap��5�)&���n�|�n�|&
beta��5� &���l����      �v��/���l����   �v��/��&
                                                overall_risk���:
                                                               &��<
(
��"�(DuckDB
bytes equal -123456
```

- In the metadata, I can see that a version is at the top of the metadata, and then a list of SchemaElement's comes afterwards.
- I'm assuming the version comes after the `duckdb_schema` string, so maybe it's between 'symbol' and it.
- Via the thrift definition, the first three fields of SchemaElement are optional
- Actually, it looks like all fields outside of the name are optional
  - Type is <u>not set</u> if the current element is a non-leaf node
    - All the nodes in the test parquet are leaf nodes
  - RepetitionType is also set for everything except for the 'root' of the schema.
- So a field _could_ look like (with 'o' denoting optional type):

```
(o type)(o type_length)(o repitition_type)(name)(o num_children)(o converted_type)(o scale)(o precision)(o field_id)
```

- I should also note that with `field_id`s being provided, a field can be declared in any order
- A SchemaElement is a struct and so I should note that:
  - Each struct entry is made up of a field-header and field-value
  - A stop field seems to conclude a struct
- The first four columns are varchar/strings
- Trying to figure out the byte layout of the duckdb parquet
  - I see the byte length before the string being passed as binary
  - Before that, I think it could either be:
    - ((field-id-delta)(field-type-id))
    - ((0000)(field-type-id))(field-id)
  - Looking at the 3 bytes before each bytestring: 
    - each byte is surrounded by ()
    - `duckdb_schema` (35)-(00)-(18) _This I think his the head of the schema, so not sure if it counts_
    - "symbol" (25)-(02)-(18)
    - "`full_name`" (25)-(02)-(18)
    - `gics_sector` (25)-(02)-(18)
    - `gics_sub_industry` (25)-(02)-(18)
    - `env_score` (25)-(02)-(18)
  - Looking at the _n_ bytes _after_ each byteststring (bytestring includes length):
  - It seems like there's 6 bytes inbetween each byestring (7 if you don't include the length)
    - "symbol" (25)-(00)-(00)-(15)-(0C)-(25)-(02)-(18)- `full_name`
    - `full_name` (25)-(00)-(00)-(15)-(0C)-(25)-(02)-(18)- `gics_sector`
    - `gics_sector` (25)-(00)-(00)
    - `gics_sub_industry` (25)-(00)-(00)
    - `env_score` (00)-(15)-(0A)
    - `social_score` (00)-(15)-(0A)
    - `gov_score` (00)-(15)-(0A) 
    - `total_esg` (00)-(15)-(02)
    - `highest_controversy` (25)-(20)-(00)
    - `percentile` (00)-(15)-(02)
    - `rating_year` (25)-(20)-(00)
    - `rating_month` (25)-(20)-(00)
    - `market_cap` (25)-(24)-(00)
    - `beta` (00)-(15)-(02)
    - `overall_risk` (25)-(20)-(00)-(16)-(D4)-(06)-(19)

- I created a python environment, and used parquet-tools to get some high-level detail about the parquet in the tests folder.
  - Number of rows from it is: 426
