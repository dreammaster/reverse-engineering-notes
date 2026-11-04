# Cadaver C reimplementation

Work in progress. Currently:

| File | |
|---|---|
| `engine/mjm.[ch]` | `mjm` container + PKWARE DCL explode + RNC1 unpacker |
| `engine/pc1.[ch]` | `.PC1` picture decoder |
| `engine/lng.[ch]` | `.LNG` language string tables (6-bit packed text) |
| `engine/gamefiles.[ch]` | detects Cadaver / Cadaver: The Payoff, resolves DOS-style paths case-insensitively, loads + unpacks files |
| `engine/screen.h` | 320×200 indexed framebuffer + palette |
| `main.c` | SDL2 front end (title / scroll pictures; Esc quits, any key switches picture) |
| `tools/cadaver_unpack.c` | command-line unpacker (`cadaver_unpack in out`, `-t` to test) |
| `tests/` | `test_formats` compares the decoders with vectors from the Python reference (`tools/gen_test_vectors.py`) |

```bash
cmake -S src -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/cadaver -g cadaver1 [--scene loading] [--screenshot out.bmp]
```

Gameplay is not implemented; see `docs/roadmap.md`.
