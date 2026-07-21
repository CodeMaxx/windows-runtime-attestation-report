//
// Helpers.h
//
// Shared helpers for the report decoders: overflow-safe bounds checks, the
// hash-algorithm table, and output formatting. Report structures are overlaid
// directly on the package buffer with Overlay. The pointers are UNALIGNED
// because the package packs its sections with no padding, so a structure may
// begin at an unaligned offset; UNALIGNED keeps field access correct on x64 and
// ARM64. Every read from the untrusted package must be guarded by an InRange
// check first.
//

#pragma once

#include <windows.h>

// Returns true if [offset, offset + length) fits within a buffer of 'total'
// bytes, without integer overflow.
inline bool
InRange(size_t offset, size_t length, size_t total)
{
    return offset <= total && length <= total - offset;
}

// Overlays a structure on the buffer at the given offset and returns a pointer
// to it. The pointer is UNALIGNED because a structure may begin at an unaligned
// offset in the packed package; UNALIGNED makes field access alignment-safe on
// x64 and ARM64. The caller must bounds-check the fields it reads with InRange
// first.
template <typename T>
inline const T UNALIGNED*
Overlay(const BYTE* base, size_t offset)
{
    return reinterpret_cast<const T UNALIGNED*>(base + offset);
}

struct HashAlg {
    ALG_ID id;
    const char* name;
    ULONG digestLength;
};

// Looks up a hash algorithm by its wire identifier. Returns nullptr if unknown.
const HashAlg*
FindHashAlg(UINT16 algorithm);

// Prints 'length' bytes as lowercase hex with no separators.
void
PrintHex(const BYTE* data, size_t length);

// Prints a horizontal rule made of the given fill character.
void
PrintRule(char fill);

// Prints a titled section banner framed by '=' rules, preceded by a blank line.
void
PrintBanner(const char* title);

// Prints a titled closing footer framed by '-' rules, preceded by a blank line.
void
PrintFooter(const char* title);

// Prints one hash blob identified by its hash algorithm and its offset from the
// start of the containing report.
void
PrintBlob(const char* label, const BYTE* report, UINT32 reportSize,
          UINT16 algorithm, UINT32 offset);
