#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pngenc.h"

typedef struct {
    uint8_t *dst;
    size_t capacity;
    size_t used;
} sink_t;

static pngenc_status_t sink_write(void *ctx, const void *data, size_t len)
{
    sink_t *sink = (sink_t *)ctx;

    if (len > (sink->capacity - sink->used)) {
        return PNGENC_ERROR;
    }

    (void)memcpy(&sink->dst[sink->used], data, len);
    sink->used += len;

    return PNGENC_OK;
}

int main(void)
{
    static pngenc_t pngenc;
    static uint8_t png[128];
    static const uint16_t image[4] = { 0xF800U, 0x07E0U, 0x001FU, 0xFFFFU };
    static const uint8_t signature[8] = { 0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU };
    static const uint8_t iend_crc[4] = { 0xAEU, 0x42U, 0x60U, 0x82U };
    pngenc_io_t io;
    pngenc_image_t image_info;
    sink_t sink;
    int failures = 0;

    sink.dst = png;
    sink.capacity = sizeof(png);
    sink.used = 0U;

    io.write = sink_write;
    io.write_ctx = &sink;

    image_info.data = image;
    image_info.width = 2U;
    image_info.height = 2U;
    image_info.pixel_format = PNGENC_PIXEL_FORMAT_RGB565;

    if (pngenc_init(&pngenc, &io) != PNGENC_OK) {
        printf("FAIL pngenc_init\n");
        return 1;
    }

    if (pngenc_encode(&pngenc, &image_info, NULL) != PNGENC_OK) {
        printf("FAIL pngenc_encode\n");
        return 1;
    }

    printf("encoded %lu bytes\n", (unsigned long)sink.used);

    if (sink.used != 82U) {
        printf("FAIL expected 82 bytes for a 2x2 RGB565 image\n");
        failures++;
    }

    if (memcmp(png, signature, sizeof(signature)) != 0) {
        printf("FAIL signature\n");
        failures++;
    }

    if (memcmp(&png[sink.used - sizeof(iend_crc)], iend_crc, sizeof(iend_crc)) != 0) {
        printf("FAIL IEND CRC\n");
        failures++;
    }

    if (failures != 0) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }

    printf("all checks passed\n");
    return 0;
}
