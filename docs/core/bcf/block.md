## BCF Block

## Overview
A **block** is a self-contained unit of data within the BCF container.
Each block has:
- A **common block header**
- A **payload**

Note: Every block must be read entirely into memory first. This ensures that the CRC can be verified before any interpretation of the payload.

## Common Block Header (20 bytes in v1)
The header is fixed for a given Major Version of the BCF format and is the same for every block.

| Offset | Size | Field           | Description                            |
| ------ | ---- | --------------- | -------------------------------------- |
| 0      | 4    | seq_num         | Sequential block number (starts at 0)  |
| 4      | 1    | block_type      | Defines how to interpret the payload   |
| 5      | 3    | reserved        | Padding                                |
| 8      | 4    | payload_size    | Size of payload (in bytes)             |
| 12     | 4    | payload_crc32   | CRC32 of payload                       |
| 16     | 4    | header_crc32    | CRC32 of bytes 0..15                   |

## Payload (TLV stream)
The payload is a sequence of one or more **Tag-Length-Value** (TLV) units, followed by optional padding.

| Component | Size       | Description                                            |
| --------- | ---------- | ------------------------------------------------------ |
| **Tag**   | 1 Byte     | Context-specific identifier (local to `block_type`)    |
| **Length**| 1-5 Bytes  | **Varint** encoded size of the Value field.            |
| **Value** | L Bytes    | Raw data                                               |

**Tag Scope**: Tags are specific to the block_type. Tag `0x04` in block_type `0x01` may have a different meaning than Tag `0x04` in block_type `0x02`. Each block type can define up to `256` unique tags.

## How does it work

### Varint encoding
The **TLV Length** is encoded using [Unsigned LEB128](https://en.wikipedia.org/wiki/LEB128#Unsigned_LEB128).
Without this encoding, defining a one byte field would use 1 (tag) + 4 (length) + 1 (value) = 6 bytes which is a waste of space.

The algorithm works like this:
1. **MSB (Most Significant Bit)**: the first bit of each byte is the **continuation bit**.
    - `1`: more bytes follow.
    - `0`: this is the last byte of the integer
2. **Lower 7 bits**: these bits carry the actual number, stored in **Little-Endian** order.

**Some examples:**
| Decimal | Binary Varint           | 
| ------- | ----------------------- | 
| 127     | `0 1111111`             | 
| 128     | `1 0000000` `0 0000001` | 
| 300     | `1 0101100` `0 0000010` |  

By using this encoding, a one byte field will use 3 bytes: 1 (tag) + 1 (length) + 1 (value), so if the length is short, the number will be represented with less bytes.

### Alignment and padding
To ensure that each block is aligned to 4 bytes, the payload size of a block must be a multiple of 4 bytes. If the TLV units do not fill this requirement, padding bytes are appended.

The **Null Tag**: the tag `0x00` is **reserved** across all block types as a padding/end-of-stream marker.

When a decoder expects a tag and reads `0x00`, it stops parsing the current block, so remaining bytes up to `payload_size` are discarded.


### Extensibility
The TLV system allows the format to evolve without breaking older decoders:
- **Skipping unknown Tags**: if a decoder encounters an unknown Tag, it reads the Length and skips the next `L` bytes to reach the next TLV unit.

### Dual-Layer Integrity
- **Header CRC**: verified first. If it fails, the decoder must stop to avoid invalid memory allocations (not sure that the payload_size is correct, because the header is corrupted).
- **Payload CRC**: verified once the full payload is in memory. This ensures the payload is not corrupted.

## Block Types
- [Data Block](./block_data.md)