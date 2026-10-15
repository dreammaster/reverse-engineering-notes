// A standard RFC 1321 MD5 (the original is the usual md5 class with a Transform() per block), needed for
// TMemoryBuffer::Decrypt()/Encrypt(), which key a repeating-XOR cipher off the text of MD5(password).
#pragma once

// Confirmed (asm lines 555941-556215): the MD5 digest of the data as text - 32 lowercase hexadecimal digits, in a string
// made with strdup() (free() it). TMemoryBuffer::Decrypt() XORs with the first 16 characters of it.
char *MD5String(const char *data, unsigned long length);
