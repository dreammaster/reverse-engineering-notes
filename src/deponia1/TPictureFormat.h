// Not yet assert-confirmed to a specific file; stays at the top level.
// TPictureIO::GetFormat picks one of these based on file magic bytes
// (checked first: "RIFF....WEBP" -> WebP, PNG signature -> PNG) or,
// failing that, a single-letter format-hint string ('P'->PNG, 'W'->WebP,
// 'J'->JPG, 'G'->GIF, and a final 'P' check that - per the disassembly -
// is only reachable after the first 'P' check already failed, so it can
// seemingly never match; reproduced as observed rather than "fixed" - see
// NOTES.md). None of the actual decoders are reversed (they wrap the real
// vendored libpng/libwebp/jpgd libraries - manifest/vendor_libraries.tsv -
// which should be linked directly rather than reimplemented).
#pragma once

class TPictureFormat {
public:
    virtual ~TPictureFormat() = default;
};

class TPicturePNG : public TPictureFormat {};
class TPictureWebP : public TPictureFormat {};
class TPictureJPG : public TPictureFormat {};
class TPictureGIF : public TPictureFormat {};
class TPicturePCX : public TPictureFormat {};
