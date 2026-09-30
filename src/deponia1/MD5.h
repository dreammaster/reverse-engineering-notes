// Not yet assert-confirmed to a specific file; stays at the top level.
// A standard RFC 1321 MD5 implementation - needed for real
// TMemoryBuffer::Decrypt()/Encrypt(), which key a repeating-XOR cipher off
// MD5(password). MD5 itself is a public, fully-specified algorithm, not
// proprietary engine logic, so this is a faithful implementation rather
// than a recovered one.
#pragma once

// Confirmed call shape (TMemoryBuffer::Decrypt, Deponia_Linux.asm line
// 551743): returns a malloc()'d 16-byte raw binary digest (matching the
// free() the caller applies to it), not a hex string.
unsigned char *MD5String(const char *data, unsigned long length);
