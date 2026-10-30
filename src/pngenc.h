#ifndef PNGENC_H
#define PNGENC_H

#include <stdint.h>
#include <stddef.h>

#define PNGENC_IDAT_BLOCK_SIZE (8192U)

typedef enum {
    PNGENC_OK = 0U,
    PNGENC_ERROR,
} pngenc_status_t;

typedef struct {
    pngenc_status_t (*write)(void *ctx, const void *data, size_t len);
    void *write_ctx;
} pngenc_io_t;

typedef struct {
    const pngenc_io_t *io;
    struct {
        uint32_t crc;
        uint32_t adler;
        struct {
            uint8_t header;
            uint8_t len_lo;
            uint8_t len_hi;
            uint8_t nlen_lo;
            uint8_t nlen_hi;
            uint8_t data[PNGENC_IDAT_BLOCK_SIZE - 5U];
        } block;
        size_t block_data_used;
    } idat;
} pngenc_t;

typedef enum {
    PNGENC_PIXEL_FORMAT_RGB565 = 0U,
} pngenc_pixel_format_t;

typedef struct {
    const void *data;
    uint16_t width;
    uint16_t height;
    pngenc_pixel_format_t pixel_format;
} pngenc_image_t;

typedef struct {
    const char *keyword;
    const uint8_t *data;
    size_t data_len;
} pngenc_text_t;

pngenc_status_t pngenc_init(pngenc_t *pngenc, const pngenc_io_t *io);

pngenc_status_t pngenc_encode(pngenc_t *pngenc, const pngenc_image_t *image, const pngenc_text_t *text);

#endif
