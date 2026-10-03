// Not part of the original engine - a stand-in for zlib's compress() and
// uncompress() (which the binary links statically; see manifest/
// vendor_libraries.tsv), so TMemoryBuffer::Compress()/Uncompress() work in
// this build, where zlib isn't available. Both speak the real zlib stream
// format: zlibUncompress() is a complete inflate (stored, fixed and dynamic
// Huffman blocks - after Mark Adler's public-domain "puff"), and
// zlibCompress() emits a valid zlib stream of stored (uncompressed) blocks,
// so its output is correct but not smaller. Replace both with the real
// library when it's linked (ScummVM provides one).
#pragma once

// Return 0 on success, like zlib's Z_OK; anything else is an error
// (-3 corrupt data, -5 output buffer too small). `destLen` is the capacity on
// entry and the produced length on success.
int zlibCompress(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen);
int zlibUncompress(unsigned char *dest, unsigned long *destLen, const unsigned char *source, unsigned long sourceLen);
