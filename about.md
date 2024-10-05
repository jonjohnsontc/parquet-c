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

---

- Today, I'm going to be attempting to install the parquet-cli locally (or use it at least) alongside a debugger to see exactly how it reads footer metadata.
- I just installed Bison, which is a dependency for installing thrift using its ./configure file
- Here is the end of install log:

```
bison is keg-only, which means it was not symlinked into /usr/local,
because macOS already provides this software and installing another version in
parallel can cause all kinds of trouble.

If you need to have bison first in your PATH, run:
  echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.zshrc

For compilers to find bison you may need to set:
  export LDFLAGS="-L/usr/local/opt/bison/lib"
==> Summary
🍺  /usr/local/Cellar/bison/3.8.2: 100 files, 3.7MB
==> Running `brew cleanup bison`...
Disable this behaviour by setting HOMEBREW_NO_INSTALL_CLEANUP.
Hide these hints with HOMEBREW_NO_ENV_HINTS (see `man brew`).

```
- Intalling bison worked, but now I'm seeing an error when running `make install`
- <strike>It might be better if I instead just git clone the repository and change it myself</strike> 
  - Forgot that the archived file I downloaded earlier was the source code
- Now I need to install Apache Thrift at version 0.19.0
- I ended up changing the code in the problematic file based on the PR that fixed the [issue](https://github.com/apache/thrift/commit/bc9c04d8049d7d5f5cf4e63a25226c1fb8c930bf)
- Afterward I resumed trying to build the parquet-cli library by running `mvn package -B -DskipTests`
- I was able to package all the libaries up-to `Parquet Scala` which includes  parquet-cli, so I think I'm going to try and use it now
- Notably, Parquet Scala is packaged before parquet-hadoop which parquet-cli normally depends on
- I was having an issue finding the class files initially, I think. But, after using a combined classpath, I was able to invoke the cli app
  - commands:

``` 
jonjohnson@Jons-MBP parquet-cli % mkdir parquet-cli                
jonjohnson@Jons-MBP parquet-cli % cp target/parquet-cli-1.14.2-runtime.jar parquet-cli/parquet-cli.jar
jonjohnson@Jons-MBP parquet-cli % cp -r target/dependency parquet-cli 
jonjohnson@Jons-MBP parquet-cli % java -cp 'parquet-cli/*:parquet-cli/dependency/*' 'org.apache.parquet.cli.Main'
```
- Now I'm having issues running it alongside the IntelliJ Debugger.
- where the above java command runs successfully, the below command results in a missing class:
```
/users/jonjohnson/library/java/javavirtualmachines/temurin-21.0.4/contents/home/bin/java -agentlib:jdwp=transport=dt_socket,address=127.0.0.1:61374,suspend=y,server=n -cp '/users/jonjohnson/dev/parquet-java/parquet-cli/*:/users/jonjohnson/dev/parquet-java/parquet-cli/dependency/*' -javaagent:/users/jonjohnson/library/caches/jetbrains/intellijidea2024.2/captureagent/debugger-agent.jar -dkotlinx.coroutines.debug.enable.creation.stack.trace=false -ddebugger.agent.enable.coroutines=true -dfile.encoding=utf-8 -dsun.stdout.encoding=utf-8 -dsun.stderr.encoding=utf-8 -jar /users/jonjohnson/dev/parquet-java/parquet-cli/parquet-cli/parquet-cli.jar
connected to the target VM, address: '127.0.0.1:61374', transport: 'socket'
Error: Could not find or load main class org.apache.parquet.cli.Main
Caused by: java.lang.NoClassDefFoundError: org/apache/hadoop/util/Tool
Disconnected from the target VM, address: '127.0.0.1:61374', transport: 'socket'
```

- I was able to get the debugger running alongside the CLI after consulting claude for a bit
- First, I needed to change invoking the application so that I didn't pass the -jar argument. That executes a program in a completely different manner, and disregards the -cp value if passed in.
- Then, I needed to make sure to surround the path I wanted to add with double quotes instead of single quotes

---

- Running the debugger, I've set a breakpoint on the `readFooter` method
- Eventually getting into the `ParquetFileReader` class and the readFooter method
- We validate the length of the file
- We read the length of the metadata
- We see if the file is encrypted
- We validate that PAR1 is on both sides of the file
- We read the footer into a byte buffer
- We flip the byteBuffer
     - Seems like it basically sets the pointer to the first byte in the buffer
- Wrap bytebuffer as InputStream
- call converter.readParquetMetadata (of type ParquetMetadataConverter)
- eventually call ParquetMetadataConverter.readParquetMetadata 
- eventually read Footer metadata by Instantiating FileMetaData class
- It first creates a mapping of high-level metadata, the key being an enum matched to the part of the parquet file, and the value being an instane of the FileMetaData class.
- The FileMetaData class does have a read method. It seems to be reading in the data within a TCompactProtocol class
      - The TCompactProtocol is wrapped by an InterningProtocol that interns strings
- TCompactProtocol is changed to a TTupleProtocol before reading in the `FileMetaData.version`
- First reads in version as I32 (read bytes as varint & then convert to Int via ZigZag)
- I don't think there's any issue reading in a varint as i64 instead
- After the version is read in the TTupleProtocol performs a `readListBegin((byte) 12)` call
  - and it reads in the size of the list as another varint 
- An ArrayList is instantiated to hold elements in the Schema to be parsed
- We start a for loop to go through all of the schema elements:
  - First, a Schema Element is instantiated for the first element in the list
    - The SchemaElement then uses its `read` method against the TTupleProtocol which holds the metadata
    - How the element is read in I'm not sure, it's linked to an abstract method
- Afterwards, the schema is set, and an Int64 is read in, which reads in the number of row groups

#### TCompactProtocol

- It reads the data through TIOStreamTransport protocol/class
  - The TIOStreamTransport reads data through a Java input stream
    - Instantiating the TIOStreamTransport class sets the java input stream as it's inputStream_ property
    - It has a LOGGER variable
- TCompactProtocol seems to be instantiated like the following, with the TIOStreamTransport class being passed as the transport arg:

```java
  public TCompactProtocol(TTransport transport, long stringLengthLimit, long containerLengthLimit) {
        super(transport);
        this.lastField_ = new ShortStack(15);
        this.lastFieldId_ = 0;
        this.booleanField_ = null;
        this.boolValue_ = null;
        this.temp = new byte[16];
        this.stringLengthLimit_ = stringLengthLimit;
        this.containerLengthLimit_ = containerLengthLimit;
    }
```

- It seems to be a class designed to help read or write bytes to the spec of TCompactProtocol

#### FileMetaData

##### Origins of File

- I'm still not quite sure where this file comes from. I _think_ now that it's created by the Thrift protocol when it reads in the parquet thrift format

##### About

- When the Util.read() method is being called, one of the arguments is a FileMetaData class is being instantiated.
- It's instantiated with 0 arguments, so I think it's instantiated like so:

```java
public FileMetaData() {
        this.__isset_bitfield = 0;
    }
```
- The Util.read() method uses the `base` arg passed in to read the inputStream. It's the FileMetaData object
  - `tbase.read(protocol((InputStream)from));`
- FileMetaData first seems to use the single arg read method, which gets a scheme value and then calls the 2 argument read method
- the two arg read method passes itself as the second argument, and the first is the Interned TCompactProtocol.

