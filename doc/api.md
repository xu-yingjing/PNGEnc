# API contract

The header declares one callback table, one handle, two input structs and two functions. The library allocates nothing, keeps no state outside the handle and never touches a file system: every output byte goes through `pngenc_io_t.write`.

## Requirements on the target

- Little-endian. Four-byte lengths and CRCs are emitted by reversing a value in memory, which is only correct on a little-endian host; there is no endianness check and no byte-swap path. On a 64-bit host the length is a `size_t` and only its low four bytes are used.
- `uint8_t` and `uint16_t` are exactly 8 and 16 bits (C99 guarantees both).
- No floating point, no OS calls, no writable static data.

## Types

- `pngenc_io_t` — `write(ctx, data, len)` is called with consecutive pieces of the output. It must consume all `len` bytes and return `PNGENC_OK`; any other value aborts the encode. It is never asked to seek, and it may be called with any length, including 5-byte block headers.
- `pngenc_t` — holds the io pointer, the IDAT CRC/Adler accumulators and the 8,192-byte staging block (8,187 data bytes plus a 5-byte stored-block header). `sizeof(pngenc_t)` is 8,208 bytes on the target, so it does not belong on a small task stack. One handle may encode any number of images; the per-encode state is reset in `pngenc_encode()`.
- `pngenc_image_t` — `data`, `width`, `height`, `pixel_format`. Rows are tightly packed, stride = `width * 2`, no padding. `width` and `height` are `uint16_t` and must be non-zero.
- `pngenc_text_t` — `keyword` (NUL-terminated, non-NULL), `data` and `data_len`. `data` may be NULL only when `data_len` is 0, which produces a valid empty zlib stream.

## pngenc_init

```c
pngenc_status_t pngenc_init(pngenc_t *pngenc, const pngenc_io_t *io);
```

Rejects a NULL handle, a NULL table or a NULL `write`. Stores the table pointer, so the `pngenc_io_t` object must outlive every later `pngenc_encode()` call; a file-scope `static const` table is the usual shape.

## pngenc_encode

```c
pngenc_status_t pngenc_encode(pngenc_t *pngenc, const pngenc_image_t *image, const pngenc_text_t *text);
```

Writes, in order: the 8-byte signature, IHDR, zTXt (only when `text` is not NULL), one IDAT and IEND. Returns `PNGENC_ERROR` before writing anything when the handle or image is NULL, when a dimension is 0, when `text->keyword` is NULL, or when `text->data` is NULL with a non-zero `data_len`.

`text->keyword` is written verbatim. PNG restricts keywords to 1-79 Latin-1 characters without NUL; the library does not check this, so a malformed keyword produces a structurally valid chunk that viewers may ignore.

## Output layout

- IHDR: bit depth 8, colour type 2, compression 0, filter 0, interlace 0.
- zTXt data: keyword, one NUL, method 0, then a zlib stream (`78 01`, stored blocks, Adler-32) over the caller's bytes. There is no compression and no Base64.
- IDAT: one chunk. The 4-byte length is computed up front, which is possible only because stored blocks have a fixed size.
- Every scanline is prefixed with filter byte 0. RGB565 samples are expanded with bit replication (`(r << 3) | (r >> 2)`, `(g << 2) | (g >> 4)`, `(b << 3) | (b >> 2)`), so `0x1F` reaches `0xFF` rather than `0xF8`.

## Sizes

```
raw        = height * (1 + 3 * width)
blocks     = ceil(raw / 8187)
idat_len   = 2 + raw + 5 * blocks + 4
file       = 8 + 25 + (12 + idat_len) + 12
ztxt_len   = keyword_len + 1 + 1 + 2 + 5 * zblocks + payload_len + 4   (zblocks = max(1, ceil(payload_len / 65535)))
```

`raw` is computed in `size_t`, but the IDAT length is written into the four-byte chunk length field, which PNG caps at 2^31-1; nothing checks this. Keeping `6 + raw + 5 * ceil(raw / 8187)` inside the cap allows at most 2,146,172,921 raw bytes, so with the maximum width of 65,535 the tallest image is 10,916 rows. That bound keeps the 32-bit `size_t` arithmetic in range, so the chunk cap is what limits an encode. A 480x640 frame gives 922,240 raw bytes, 113 blocks and a 922,868-byte file.

## Failure behaviour

- Argument errors return before the first byte is written.
- A callback error aborts immediately and returns `PNGENC_ERROR`. The output already handed to the callback stays where the caller put it; the library does not delete, truncate or rewrite anything.
- After a failed encode the handle is not guaranteed to be clean; call `pngenc_encode()` again only to start a new file from the beginning.
