# Changelog

## 0.1.0

Initial public release. Extracted from an in-tree helper that writes an RGB565 framebuffer to a PNG carrying a zTXt metadata chunk.

- `pngenc_init()` and `pngenc_encode()` keep the in-tree contract: the caller supplies a write callback, the library allocates nothing, and the 8 KiB IDAT staging block lives inside the caller-owned handle.
- New for the standalone release: `README.md`, `doc/api.md`, `examples/pngenc_demo.c` and `build.sh`. The two files under `src/` are unchanged.
- Known gaps, by design: stored deflate blocks only (no compression), one IDAT chunk, RGB565 input only, little-endian targets only, no alpha/palette/16-bit/interlaced output, and no decoder.
