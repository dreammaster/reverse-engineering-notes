// Original path confirmed via an x_assert() call: src/baselib/xmlCommon.cpp -
// see manifest/source_layout.tsv.
//
// The helpers that turn the text of an XML attribute (a [begin, end) range of
// characters, not NUL-terminated) into a typed value (Deponia_Linux.asm lines
// 527454-528206), plus dtol() and normalizepath() (asm lines 636139-636260),
// which they and TData::SerializeParam() share. dtol()/normalizepath() sit far
// from the others in the binary, so they probably belong to another file; their
// path isn't recovered.
#pragma once

#include "WxStub.h"

/** The decimal value of the characters (strtol, base 10). */
int ConvertToInt(const char *begin, const char *end);
/** The value of the characters read with strtod, as a float. */
float ConvertToFloat(const char *begin, const char *end);
/** One character: 'T'/'t' is true, 'F'/'f' false; anything else is
 *  reported through x_assert() and read as false. */
bool ConvertToBool(const char *begin, const char *end);
/** The characters as a (UTF-8 decoded) string. */
wxString ConvertToString(const char *begin, const char *end);
/** The characters as a file name, with any backslashes turned into forward
 *  slashes and the path normalized. */
wxFileName ConvertToFileName(const char *begin, const char *end);

/** Parses an optionally '-'-prefixed run of decimal digits; the characters
 *  aren't validated. */
long dtol(const char *text);
/** Turns every backslash into a forward slash, in place. */
void normalizepath(char *path);
