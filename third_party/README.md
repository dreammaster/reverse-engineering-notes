# third_party

Source of the libraries of the original game that Deponia 1 needs, at the versions found in the original's executable
(`THIRD_PARTY.md` has the evidence). Each is the upstream release or tag, trimmed of its tests, documents and build files;
`tools/build_third_party.py` builds them as static libraries in `build/third_party/<bits>/`.

| Directory | Upstream | Tag | License |
|---|---|---|---|
| `libwebp` | https://github.com/webmproject/libwebp | v0.5.0 (37f0494) | BSD 3-clause |
| `opus` | https://github.com/xiph/opus | v1.1 (65471dd) | BSD 3-clause |
| `opusfile` | https://github.com/xiph/opusfile | v0.7 (51c5c36) | BSD 3-clause |
| `ogg` | https://github.com/xiph/ogg | v1.3.2 (36606a0) | BSD 3-clause |
| `harfbuzz` | https://github.com/harfbuzz/harfbuzz | 1.8.8 (63be5dc) | MIT (Old MIT) |
| `libvpx` | https://github.com/webmproject/libvpx | v1.7.0 (f80be22) | BSD 3-clause |
| `freetype` | https://github.com/freetype/freetype | VER-2-8 (2.8.0, a12a344) | FreeType License (FTL) |

Not changed: the files are as upstream (see each `COPYING`; FreeType: `FTL.TXT`). FreeType is the `include/` and `src/` of the release without its
`src/tools` and build files; all its modules are built (the default `ftmodule.h`). HarfBuzz is `src/` of the release (with the UCDN
Unicode functions and the FreeType glue, built with HAVE_OT, HAVE_FALLBACK, HAVE_UCDN, HAVE_FREETYPE). The libraries of the original that the game data does not use
(Assimp, Spine, Box2D bindings, curl, mysql++ ...) are not here.

`libvpx` is the VP8 decoder only (vp8/, vpx/, vpx_dsp/, vpx_mem/, vpx_scale/, vpx_util/ of the release, without the encoder, the assembler files and the files of
other CPUs). It is not a library of the original: its FFmpeg decodes VP8 with its own code. It stands in for that decoder (the movies are the same
VP8 files). The headers in `libvpx/generated/` are what `configure --target=generic-gnu --disable-vp9 --disable-vp8-encoder --disable-multithread`
and `build/make/rtcd.pl --arch=generic` make (`vpx_version.h` by hand, "v1.7.0"), so that the library is built without `make`.
