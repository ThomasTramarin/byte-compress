#ifndef BCOMP_VERSION_H
#define BCOMP_VERSION_H

/**
 * @file version.h
 * @brief Semantic Versioning definitions.
 */

/* BCF FORMAT VERSION (Data Specification)
 *
 * Tracks the binary layout of the .bcf files.
 * - MAJOR: Breaking changes. Old software CANNOT read new files.
 * - MINOR: New features (e.g., new block types) that are backwards compatible.
 */
#define BCOMP_VER_FORMAT_MAJOR 1
#define BCOMP_VER_FORMAT_MINOR 0

/* CORE ENGINE VERSION (Library Logic)
 *
 * Tracks the "libbcomp" logic, algorithms, and API.
 * - MAJOR: API refactoring or total engine rewrite.
 * - MINOR: New compression algorithms or public functions added.
 * - PATCH: Bug fixes, security patches, or performance optimizations.
 */
#define BCOMP_VER_CORE_MAJOR 1
#define BCOMP_VER_CORE_MINOR 0
#define BCOMP_VER_CORE_PATCH 0

/* CLI VERSION (Frontend Interface)
 *
 * Tracks the command-line tool features and user experience.
 * - MAJOR: Complete overhaul of the command syntax or UX.
 * - MINOR: New flags, commands, or output modes.
 * - PATCH: UI bug fixes, improved error messages, or wording updates.
 */
#define BCOMP_VER_CLI_MAJOR 1
#define BCOMP_VER_CLI_MINOR 0
#define BCOMP_VER_CLI_PATCH 0

#endif