# BCF Global Header

The **Global Header** is the entry point of a BCF stream.  
This data unit contains crucial information that will change how the next bytes are parsed.

## 1. Memory Layout (v.1.0)
The header is a fixed-size **16-byte** structure in version 1.0.

| Offset | Field      | Size (Bytes) | Category  |
| ------ | ---------- | ------------ | --------- |
| 0      | Magic Word | 4            | Fixed     |
| 4      | Major Ver  | 1            | Fixed     |
| 5      | Minor Ver  | 1            | Fixed     |
| 6      | Reserved   | 2            | Specific  | 
| 8      | Block size | 4            | Specific  | 
| 12     | CRC        | 4            | Fixed     | 

## 2. Field Definitions

### 2.1 Magic Word
This string is used for file identification. The parser must check these 4 bytes first.
If they do not match `BCF\0`, the file/stream is invalid.

### 2.2 Version Fields
- **Major**: If the decoder's supported major version is lower than the file's major version, the decoder must abort.
- **Minor**: If the decoder's minor version is lower than the file's, the decoder should proceed, without considering new fields.

### 2.3 Block Size
This field defines the maximum size of uncompressed data allowed in a single data block.
This allows the decompressor to pre-allocate a single buffer, optimizing memory usage.

### 2.4 CRC
The CRC is calculated over the first 12 bytes of the header.

## 3. Evolution Strategy

### 3.1 Fixed & Specifig Fields
The first 6 bytes are immutable, this means that in the future versions, these fields never will never move to maintain compatibility.
The fields starting from offset 6 up to the CRC contains version-specific data.
In v1.0, this "section" is 6 bytes long.
The last 4 bytes contains always the CRC calculated from the previous bytes.

### 3.2 Versioning
The version number in the global header defines the baseline specififation for the entire BCF stream. It is not limited to the global header layout but establishes the fundamental "contract" for all subsequent data (e.g. the structure of the common block header, integrity algorithms, data alignment).