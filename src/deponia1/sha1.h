// Confirmed names and signatures (Deponia_Linux.asm line 440310, lua_sha1): the usual third-party SHA-1 class with
// its two static functions.
#pragma once

class sha1 {
public:
	/** The 20 bytes SHA-1 hash of `bytelength` bytes. */
	static void calc(const void *src, int bytelength, unsigned char *hash);
	/** The 40 lower case hex digits of a hash (and a 0), `hexstring` has 41 characters. */
	static void toHexString(const unsigned char *hash, char *hexstring);
};
