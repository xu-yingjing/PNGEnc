# pngenc

Streaming PNG writer for C99 targets with no OS, no HAL, no dynamic allocation and no writable static data.

The library writes a valid PNG in one pass: signature, IHDR, an optional zTXt chunk, one IDAT and IEND. Every deflate block is stored (BTYPE 00), so the output length is a closed-form function of the image size and the IDAT length field can be written before the pixels exist. Nothing is compressed; the price is about 3 bytes per pixel plus 0.06 % of block headers.

## Scope

Supported:

- 8-bit RGB output: colour type 2, bit depth 8, filter type 0 on every scanline, non-interlaced.
- RGB565 input, little-endian 16-bit samples, expanded to 8 bits with bit replication.
- One IDAT chunk, streamed to the caller's write callback in 8 KiB pieces.
- An optional zTXt chunk written between IHDR and IDAT: keyword, method 0 and the caller's bytes wrapped in a stored zlib stream. The payload is neither compressed nor encoded; a Base64 layer, if one is needed, belongs to the caller.
- Repeated encodes on one handle.

Not supported, by design:

- Compression (stored blocks only), multiple IDAT chunks, or any output other than colour type 2 at bit depth 8.
- Alpha, palette, 16-bit channels, interlacing, scanline filters other than 0.
- Decoding or verifying PNG files.
- Big-endian targets: lengths and CRCs are emitted by reversing their in-memory bytes, which is only correct on a little-endian host.

## Quick start

```c
#include "pngenc.h"

static pngenc_t pngenc;   /* holds the 8 KiB staging block: keep it out of the stack */

static pngenc_status_t write_cb(void *ctx, const void *data, size_t len)
{
    /* append len bytes to the file or buffer that ctx points at */
    return PNGENC_OK;
}

void save_frame(const uint16_t *frame, uint16_t width, uint16_t height)
{
    static const pngenc_io_t io = { write_cb, NULL };
    pngenc_image_t image = { frame, width, height, PNGENC_PIXEL_FORMAT_RGB565 };

    if (pngenc_init(&pngenc, &io) != PNGENC_OK) {
        return;
    }
    if (pngenc_encode(&pngenc, &image, NULL) != PNGENC_OK) {
        /* the callback failed; a partial file is left behind */
    }
}
```

The full contract, including the callback rules and the size formulas, is in [doc/api.md](doc/api.md). `examples/pngenc_demo.c` encodes a 2x2 image and checks the 82-byte result.

## Sizes

```
raw    = height * (1 + 3 * width)          filter byte + RGB triples
blocks = ceil(raw / 8187)                  stored blocks of at most 8187 data bytes
IDAT   = 12 + 2 + raw + 5 * blocks + 4     chunk envelope + zlib header + blocks + Adler-32
file   = 8 + 25 + IDAT + 12                signature + IHDR + IEND
```

A 480x640 frame is 922,240 raw bytes in 113 blocks, a 922,868-byte file. A zTXt chunk, when present, adds `12 + keyword_len + 1 + 1 + 2 + 5 * zblocks + payload_len + 4` bytes.

## Verification

`sh build.sh` compiles the library and the example with every warning enabled (`-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wundef`, `-Wwrite-strings`, `-Werror`). The release was checked with arm-none-eabi-gcc 12.3 and produced no warnings.

Build and run the example on a host:

```sh
cc -std=c99 -I src src/pngenc.c examples/pngenc_demo.c -o pngenc_demo && ./pngenc_demo
```

## License

BSD 3-Clause, see [LICENSE](LICENSE).
