#include <cstdio>
#include <cstring>
#include <vector>

#include "zlibShim.h"
#include "zipdata.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static bool inflates(const unsigned char *packed, unsigned long packedSize, const unsigned char *plain, unsigned long plainSize) {
	std::vector<unsigned char> out(plainSize + 16);
	unsigned long size = out.size();
	int result = zlibUncompress(out.data(), &size, packed, packedSize);

	return result == 0 && size == plainSize && std::memcmp(out.data(), plain, plainSize) == 0;
}

// The stand-in for zlib: streams made by the real zlib (a fixed block, dynamic blocks, a stored one) come out as they went
// in, bad streams are refused, and what it compresses itself comes back.
int main() {
	CHECK(inflates(kPacked_fixed, sizeof(kPacked_fixed), kPlain_fixed, sizeof(kPlain_fixed)));
	CHECK(inflates(kPacked_dynamic, sizeof(kPacked_dynamic), kPlain_dynamic, sizeof(kPlain_dynamic)));
	CHECK(inflates(kPacked_noise, sizeof(kPacked_noise), kPlain_noise, sizeof(kPlain_noise)));
	CHECK(inflates(kPacked_mixed, sizeof(kPacked_mixed), kPlain_mixed, sizeof(kPlain_mixed)));

	// the repeated one: long matches with a short distance
	std::vector<unsigned char> repeated(40000);

	for (size_t i = 0; i < repeated.size(); i++)
		repeated[i] = (unsigned char)("abcdefgh"[i % 8]);
	CHECK(inflates(kPacked_repeat, sizeof(kPacked_repeat), repeated.data(), repeated.size()));

	// refused: too short, a bad header, a stream cut off, a buffer too small
	std::vector<unsigned char> out(100000);
	unsigned long size = out.size();

	CHECK(zlibUncompress(out.data(), &size, kPacked_dynamic, 3) != 0);
	unsigned char bad[sizeof(kPacked_fixed)];

	std::memcpy(bad, kPacked_fixed, sizeof(bad));
	bad[0] = 0x79;
	size = out.size();
	CHECK(zlibUncompress(out.data(), &size, bad, sizeof(bad)) != 0);
	size = out.size();
	CHECK(zlibUncompress(out.data(), &size, kPacked_dynamic, sizeof(kPacked_dynamic) / 2) != 0);
	size = 100;
	CHECK(zlibUncompress(out.data(), &size, kPacked_dynamic, sizeof(kPacked_dynamic)) != 0);

	// its own compression comes back
	std::vector<unsigned char> packed(sizeof(kPlain_mixed) + 1000);
	unsigned long packedSize = packed.size();

	CHECK(zlibCompress(packed.data(), &packedSize, kPlain_mixed, sizeof(kPlain_mixed)) == 0);
	CHECK(inflates(packed.data(), packedSize, kPlain_mixed, sizeof(kPlain_mixed)));

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
