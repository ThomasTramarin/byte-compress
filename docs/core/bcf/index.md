# BComp Format (BCF) Specification - v1.0

## Table Of Contents
1. [Overview](#overview)  
2. [Terminology](#terminology)  
3. [High-Level Diagram](#high-level-design)  
4. [Format Design Principles](#format-design-principles)  
5. [Global Header](#global-header)  
6. [Block Structure](#block-structure)  
7. [Versioning](#versioning)  

---

## Overview 
**BComp Format (BCF)** is a **binary container** designed for the **bcomp** compression tool. 
It is **stream-oriented** and encapsulates compressed data within a **multi-layered architecture**.

The main goal is to allow:
- Efficient reading/writing of compressed blocks
- Forward and backward compatibility via versioning
- Independent error detection (CRC)
- Easy extension of the format

## Terminology
- **BCF (BComp Format)**: The binary container format.
- **Global Header**: The first segment of the format, containing information to interpret the next blocks correctly.
- **Block**: A self-contained unit of data within the container. Each block is numerated sequentially and depending on the information that are stored inside it (e.g. compressed data, metadata, etc.) it has a number which defines the type of block.
- **CRC (Cyclic Redundancy Check)**: Checksum used to verify the integrity of headers/blocks.

## High-Level Design
This format is a container of blocks which are written sequentially.

The first bytes refers to the **Global Header**, which contains information related to the entire format. 

After the global header, there are *n* **blocks** in sequence. 

```
[Global Header]
[Block 0]  
[Block 1]  
[Block 2]  
[Block n]  
```

## Format Design Principles
- **Endianness**: All multi-byte integers are written in **Little-Endian** format.
- **Encapsulation**: Separate data transport from logic.
- **Data Integrity**: Independent error detection for both header and block payloads.
- **Alignment**: The format is aligned to **4 bytes**

## Global Header
The global header is the first part of the BCF container. It contains all the metadata necessary to interpret the following blocks, including version information and a CRC for integrity.  
For a detailed explaination of its structure, see: [Global Header](./global-header.md)

## Versioning
The format does not use a single global version. Instead, versioning is applied independently to different layers to ensure maximum flexibility and compatibility.

The **major** is increased only if the versioned "segment" changes in size.
For example if on version 1.0 a header is 16 bytes, version 2.0 will have
a different size (e.g. 12 or 24).

The **minor** instead, is updated only if the standard modification doesn't touch previous offsets or size but adds only new fields or flags.
This is possible for example when some reserved/padding bytes becomes
used but the size and offsets still remain the same.

This version system allows the core to always interpret old formats (**backtrack compatibility**) but it also can interpret newer format
that the core doesn't know with the limit that the format must have a major version equal to the latest major version supported by the program
(e.g. a 1.3 version can be interpreted by a program which latest version is 1.1).






## Structure
| Offset | Field            | Size (Bytes) | Description                                  |
| ------ | ---------------- | ------------ | -------------------------------------------- |
| 0      | Magic Word       | 4            | `0x42 0x43 0x46 0x00` ("BCF\0")              |
| 4      | Major Ver        | 1            | ...                                          |
| 5      | Minor Ver        | 1            | ...                                          |
| 6      | Version specific | N            | Version specific bytes (minor)               |
| 6 + N  | Header CRC       | 4            | CRC32 of offsets 0–11                        |

## v1.0 (16 bytes total)

| Offset | Field            |Size (Bytes)  | Description                                  |
| ------ | ---------------- | ------------ | -------------------------------------------- |
| 0      | Reserved         | 2            | Padding bytes                                |
| 2      | Block size       | 4            | Size of uncompressed data blocks             |