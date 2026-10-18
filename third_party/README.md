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

Not changed: the files are as upstream (see each `COPYING`). The libraries of the original that the game data does not use
(Assimp, Spine, Box2D bindings, curl, mysql++ ...) are not here.
