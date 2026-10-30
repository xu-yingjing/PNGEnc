#include "pngenc.h"
#include <stdbool.h>
#include <string.h>

#define ZLIB_BLOCK_MAX (65535U)

static const uint32_t crc32_table[256] = {0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU, 0x076DC419U, 0x706AF48FU, 0xE963A535U, 0x9E6495A3U, 0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91EU, 0x97D2D988U, 0x09B64C2BU, 0x7EB17CBDU, 0xE7B82D07U, 0x90BF1D91U, 0x1DB71064U, 0x6AB020F2U, 0xF3B97148U, 0x84BE41DEU, 0x1ADAD47DU, 0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U, 0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU, 0x14015C4FU, 0x63066CD9U, 0xFA0F3D63U, 0x8D080DF5U, 0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U, 0xA2677172U, 0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU, 0x35B5A8FAU, 0x42B2986CU, 0xDBBBC9D6U, 0xACBCF940U, 0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U, 0x26D930ACU, 0x51DE003AU, 0xC8D75180U, 0xBFD06116U, 0x21B4F4B5U, 0x56B3C423U, 0xCFBA9599U, 0xB8BDA50FU, 0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U, 0x2F6F7C87U, 0x58684C11U, 0xC1611DABU, 0xB6662D3DU, 0x76DC4190U, 0x01DB7106U, 0x98D220BCU, 0xEFD5102AU, 0x71B18589U, 0x06B6B51FU, 0x9FBFE4A5U, 0xE8B8D433U, 0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U, 0x7F6A0DBBU, 0x086D3D2DU, 0x91646C97U, 0xE6635C01U, 0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU, 0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U, 0x65B0D9C6U, 0x12B7E950U, 0x8BBEB8EAU, 0xFCB9887CU, 0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U, 0x4DB26158U, 0x3AB551CEU, 0xA3BC0074U, 0xD4BB30E2U, 0x4ADFA541U, 0x3DD895D7U, 0xA4D1C46DU, 0xD3D6F4FBU, 0x4369E96AU, 0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U, 0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U, 0x5005713CU, 0x270241AAU, 0xBE0B1010U, 0xC90C2086U, 0x5768B525U, 0x206F85B3U, 0xB966D409U, 0xCE61E49FU, 0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U, 0x59B33D17U, 0x2EB40D81U, 0xB7BD5C3BU, 0xC0BA6CADU, 0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU, 0xEAD54739U, 0x9DD277AFU, 0x04DB2615U, 0x73DC1683U, 0xE3630B12U, 0x94643B84U, 0x0D6D6A3EU, 0x7A6A5AA8U, 0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U, 0xF00F9344U, 0x8708A3D2U, 0x1E01F268U, 0x6906C2FEU, 0xF762575DU, 0x806567CBU, 0x196C3671U, 0x6E6B06E7U, 0xFED41B76U, 0x89D32BE0U, 0x10DA7A5AU, 0x67DD4ACCU, 0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U, 0xD6D6A3E8U, 0xA1D1937EU, 0x38D8C2C4U, 0x4FDFF252U, 0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU, 0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U, 0xDF60EFC3U, 0xA867DF55U, 0x316E8EEFU, 0x4669BE79U, 0xCB61B38CU, 0xBC66831AU, 0x256FD2A0U, 0x5268E236U, 0xCC0C7795U, 0xBB0B4703U, 0x220216B9U, 0x5505262FU, 0xC5BA3BBEU, 0xB2BD0B28U, 0x2BB45A92U, 0x5CB36A04U, 0xC2D7FFA7U, 0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU, 0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU, 0x9C0906A9U, 0xEB0E363FU, 0x72076785U, 0x05005713U, 0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU, 0x0CB61B38U, 0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U, 0x86D3D2D4U, 0xF1D4E242U, 0x68DDB3F8U, 0x1FDA836EU, 0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U, 0x88085AE6U, 0xFF0F6A70U, 0x66063BCAU, 0x11010B5CU, 0x8F659EFFU, 0xF862AE69U, 0x616BFFD3U, 0x166CCF45U, 0xA00AE278U, 0xD70DD2EEU, 0x4E048354U, 0x3903B3C2U, 0xA7672661U, 0xD06016F7U, 0x4969474DU, 0x3E6E77DBU, 0xAED16A4AU, 0xD9D65ADCU, 0x40DF0B66U, 0x37D83BF0U, 0xA9BCAE53U, 0xDEBB9EC5U, 0x47B2CF7FU, 0x30B5FFE9U, 0xBDBDF21CU, 0xCABAC28AU, 0x53B39330U, 0x24B4A3A6U, 0xBAD03605U, 0xCDD70693U, 0x54DE5729U, 0x23D967BFU, 0xB3667A2EU, 0xC4614AB8U, 0x5D681B02U, 0x2A6F2B94U, 0xB40BBE37U, 0xC30C8EA1U, 0x5A05DF1BU, 0x2D02EF8DU};
static const uint8_t png_signature[8] = {0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU};
static const uint8_t zlib_header[2] = {0x78U, 0x01U};
static const char ztxt_separator = '\0';
static const uint8_t ztxt_method = 0U;
static const uint8_t idat_filter = 0U;

static uint32_t crc32_update(uint32_t crc, const void *data, size_t len)
{
    const uint8_t *data_u8;

    data_u8 = (const uint8_t *)data;

    while (len-- > 0U) {
        crc = crc32_table[(crc ^ *data_u8++) & 0xFFU] ^ (crc >> 8);
    }

    return crc;
}

static uint32_t adler32_update(uint32_t adler, const uint8_t *data, size_t len)
{
    uint32_t a;
    uint32_t b;
 
    a = adler & 0xFFFFU;
    b = adler >> 16;
 
    while (len-- > 0U) {
        a += *data++;
        if (a >= 65521U) {
            a -= 65521U;
        }
        b += a;
        if (b >= 65521U) {
            b -= 65521U;
        }
    }
 
    return (b << 16) | a;
}

static void put_be32(uint8_t *dst, const uint8_t *src)
{
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
}

static pngenc_status_t io_write(const pngenc_t *pngenc, const uint8_t *data, size_t len)
{
    return pngenc->io->write(pngenc->io->write_ctx, data, len);
}

static pngenc_status_t write_signature(const pngenc_t *pngenc)
{
    pngenc_status_t ret;

    ret = io_write(pngenc, png_signature, sizeof(png_signature));
    if (ret != PNGENC_OK) {
        return ret;
    }

    return PNGENC_OK;
}

static pngenc_status_t write_chunk(const pngenc_t *pngenc, const char *type, const uint8_t *data, size_t len)
{
    pngenc_status_t ret;
    uint8_t chunk_len[4];
    uint8_t chunk_crc[4];
    uint32_t crc;

    put_be32(chunk_len, (const uint8_t *)&len);
    ret = io_write(pngenc, chunk_len, sizeof(chunk_len));
    if (ret != PNGENC_OK) {
        return ret;
    }

    ret = io_write(pngenc, (const uint8_t *)type, 4U);
    if (ret != PNGENC_OK) {
        return ret;
    }
    crc = crc32_update(0xFFFFFFFFU, type, 4U);

    if (len > 0U) {
        ret = io_write(pngenc, data, len);
        if (ret != PNGENC_OK) {
            return ret;
        }
        crc = crc32_update(crc, data, len);
    }

    crc ^= 0xFFFFFFFFU;

    put_be32(chunk_crc, (const uint8_t *)&crc);
    ret = io_write(pngenc, chunk_crc, sizeof(chunk_crc));
    if (ret != PNGENC_OK) {
        return ret;
    }

    return PNGENC_OK;
}

static pngenc_status_t write_ihdr(const pngenc_t *pngenc, uint32_t width, uint32_t height)
{
    pngenc_status_t ret;
    struct {
        uint8_t width[4];
        uint8_t height[4];
        uint8_t bit_depth;
        uint8_t colour_type;
        uint8_t compression_method;
        uint8_t filter_method;
        uint8_t interlace_method;
    } ihdr;

    put_be32(ihdr.width, (const uint8_t *)&width);
    put_be32(ihdr.height, (const uint8_t *)&height);
    ihdr.bit_depth = 8U;
    ihdr.colour_type = 2U;
    ihdr.compression_method = 0U;
    ihdr.filter_method = 0U;
    ihdr.interlace_method = 0U;
    ret = write_chunk(pngenc, "IHDR", (const uint8_t *)&ihdr, sizeof(ihdr));
    if (ret != PNGENC_OK) {
        return ret;
    }

    return PNGENC_OK;
}

static pngenc_status_t write_ztxt(const pngenc_t *pngenc, const char *keyword, const uint8_t *data, size_t data_len)
{
    pngenc_status_t ret;
    size_t keyword_len;
    size_t block_count;
    size_t zlib_len;
    size_t len;
    const char *type;
    uint32_t crc;
    size_t src_pos = 0U;
    size_t block_len;
    uint32_t adler;
    uint8_t chunk_len[4];
    uint8_t block_header[5];
    uint8_t zlib_adler[4];
    uint8_t chunk_crc[4];
 
    keyword_len = strlen(keyword);
    block_count = (data_len + ZLIB_BLOCK_MAX - 1U) / ZLIB_BLOCK_MAX;
    if (block_count == 0U) {
        block_count = 1U;
    }
    zlib_len = 2U + (5U * block_count) + data_len + 4U;
    len = keyword_len + 1U + 1U + zlib_len;
    type = "zTXt";
 
    put_be32(chunk_len, (const uint8_t *)&len);
    ret = io_write(pngenc, chunk_len, sizeof(chunk_len));
    if (ret != PNGENC_OK) {
        return ret;
    }

    ret = io_write(pngenc, (const uint8_t *)type, 4U);
    if (ret != PNGENC_OK) {
        return ret;
    }
    crc = crc32_update(0xFFFFFFFFU, (const uint8_t *)type, 4U);

    ret = io_write(pngenc, (const uint8_t *)keyword, keyword_len);
    if (ret != PNGENC_OK) {
        return ret;
    }
    crc = crc32_update(crc, (const uint8_t *)keyword, keyword_len);

    ret = io_write(pngenc, (const uint8_t *)&ztxt_separator, sizeof(ztxt_separator));
    if (ret != PNGENC_OK) {
        return ret;
    }
    crc = crc32_update(crc, (const uint8_t *)&ztxt_separator, sizeof(ztxt_separator));

    ret = io_write(pngenc, &ztxt_method, sizeof(ztxt_method));
    if (ret != PNGENC_OK) {
        return ret;
    }
    crc = crc32_update(crc, &ztxt_method, sizeof(ztxt_method));

    ret = io_write(pngenc, zlib_header, sizeof(zlib_header));
    if (ret != PNGENC_OK) {
        return ret;
    }
    crc = crc32_update(crc, zlib_header, sizeof(zlib_header));

    do {
        block_len = data_len - src_pos;
        if (block_len > ZLIB_BLOCK_MAX) {
            block_len = ZLIB_BLOCK_MAX;
        }

        block_header[0] = ((src_pos + block_len) == data_len) ? 0x01U : 0x00U;
        block_header[1] = (uint8_t)block_len;
        block_header[2] = (uint8_t)(block_len >> 8);
        block_header[3] = (uint8_t)(~block_len);
        block_header[4] = (uint8_t)((~block_len) >> 8);
        ret = io_write(pngenc, block_header, sizeof(block_header));
        if (ret != PNGENC_OK) {
            return ret;
        }
        crc = crc32_update(crc, block_header, sizeof(block_header));

        if (block_len > 0U) {
            ret = io_write(pngenc, &data[src_pos], block_len);
            if (ret != PNGENC_OK) {
                return ret;
            }
            crc = crc32_update(crc, &data[src_pos], block_len);
        }

        src_pos += block_len;
    } while (src_pos < data_len);

    adler = adler32_update(1U, data, data_len);
    put_be32(zlib_adler, (const uint8_t *)&adler);
    ret = io_write(pngenc, zlib_adler, sizeof(zlib_adler));
    if (ret != PNGENC_OK) {
        return ret;
    }
    crc = crc32_update(crc, zlib_adler, sizeof(zlib_adler));

    crc ^= 0xFFFFFFFFU;
    put_be32(chunk_crc, (const uint8_t *)&crc);
    ret = io_write(pngenc, chunk_crc, sizeof(chunk_crc));
    if (ret != PNGENC_OK) {
        return ret;
    }

    return PNGENC_OK;
}

static pngenc_status_t write_idat_begin(pngenc_t *pngenc, size_t len)
{
    pngenc_status_t ret;
    const char *type;
    uint8_t chunk_len[4];

    type = "IDAT";

    put_be32(chunk_len, (const uint8_t *)&len);
    ret = io_write(pngenc, chunk_len, sizeof(chunk_len));
    if (ret != PNGENC_OK) {
        return ret;
    }

    ret = io_write(pngenc, (const uint8_t *)type, 4U);
    if (ret != PNGENC_OK) {
        return ret;
    }
    pngenc->idat.crc = crc32_update(0xFFFFFFFFU, type, 4U);

    ret = io_write(pngenc, zlib_header, sizeof(zlib_header));
    if (ret != PNGENC_OK) {
        return ret;
    }
    pngenc->idat.crc = crc32_update(pngenc->idat.crc, zlib_header, sizeof(zlib_header));

    pngenc->idat.adler = 1U;
    pngenc->idat.block_data_used = 0U;

    return PNGENC_OK;
}

static pngenc_status_t idat_block_flush(pngenc_t *pngenc, bool final)
{
    pngenc_status_t ret;

    pngenc->idat.block.header = (final == false) ? 0x00U : 0x01U;
    pngenc->idat.block.len_lo = (uint8_t)(pngenc->idat.block_data_used & 0xFFU);
    pngenc->idat.block.len_hi = (uint8_t)((pngenc->idat.block_data_used >> 8) & 0xFFU);
    pngenc->idat.block.nlen_lo = (uint8_t)(~pngenc->idat.block_data_used & 0xFFU);
    pngenc->idat.block.nlen_hi = (uint8_t)((~pngenc->idat.block_data_used >> 8) & 0xFFU);

    ret = io_write(pngenc, (const uint8_t *)&pngenc->idat.block, 5U + pngenc->idat.block_data_used);
    if (ret != PNGENC_OK) {
        return ret;
    }
    pngenc->idat.crc = crc32_update(pngenc->idat.crc, (const uint8_t *)&pngenc->idat.block, 5U + pngenc->idat.block_data_used);

    pngenc->idat.adler = adler32_update(pngenc->idat.adler, pngenc->idat.block.data, pngenc->idat.block_data_used);
    pngenc->idat.block_data_used = 0U;

    return PNGENC_OK;
}

static pngenc_status_t idat_block_put_data(pngenc_t *pngenc, const uint8_t *data, size_t len)
{
    pngenc_status_t ret;
    size_t space;
    size_t chunk;

    while (len > 0U) {
        space = PNGENC_IDAT_BLOCK_SIZE - 5U - pngenc->idat.block_data_used;
        if (space == 0U) {
            ret = idat_block_flush(pngenc, false);
            if (ret != PNGENC_OK) {
                return ret;
            }

            space = PNGENC_IDAT_BLOCK_SIZE - 5U;
        }

        chunk = (len < space) ? len : space;
        (void)memcpy(&pngenc->idat.block.data[pngenc->idat.block_data_used], data, chunk);
        pngenc->idat.block_data_used += chunk;

        data += chunk;
        len -= chunk;
    }

    return PNGENC_OK;
}

static pngenc_status_t write_idat_scanlines_rgb565(pngenc_t *pngenc, const uint8_t *data, uint32_t width, uint32_t height)
{
    pngenc_status_t ret;
    uint32_t y;
    uint32_t x;
    uint8_t pixel[3];
    uint16_t value;
    uint32_t r;
    uint32_t g;
    uint32_t b;
 
    for (y = 0U; y < height; y++) {
        ret = idat_block_put_data(pngenc, &idat_filter, 1U);
        if (ret != PNGENC_OK) {
            return ret;
        }
 
        for (x = 0U; x < width; x++) {
            value = (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
            r = ((uint32_t)value >> 11) & 0x1FU;
            g = ((uint32_t)value >> 5) & 0x3FU;
            b = (uint32_t)value & 0x1FU;
 
            pixel[0] = (uint8_t)((r << 3) | (r >> 2));
            pixel[1] = (uint8_t)((g << 2) | (g >> 4));
            pixel[2] = (uint8_t)((b << 3) | (b >> 2));
 
            ret = idat_block_put_data(pngenc, pixel, 3U);
            if (ret != PNGENC_OK) {
                return ret;
            }
 
            data += 2U;
        }
    }
 
    return PNGENC_OK;
}

static pngenc_status_t write_idat_end(pngenc_t *pngenc)
{
    pngenc_status_t ret;
    uint8_t idat_adler[4];
    uint8_t idat_crc[4];

    ret = idat_block_flush(pngenc, true);
    if (ret != PNGENC_OK) {
        return ret;
    }

    put_be32(idat_adler, (const uint8_t *)&pngenc->idat.adler);
    ret = io_write(pngenc, idat_adler, sizeof(idat_adler));
    if (ret != PNGENC_OK) {
        return ret;
    }
    pngenc->idat.crc = crc32_update(pngenc->idat.crc, idat_adler, sizeof(idat_adler));

    pngenc->idat.crc ^= 0xFFFFFFFFU;
    put_be32(idat_crc, (const uint8_t *)&pngenc->idat.crc);
    ret = io_write(pngenc, idat_crc, sizeof(idat_crc));
    if (ret != PNGENC_OK) {
        return ret;
    }

    return PNGENC_OK;
}

static pngenc_status_t write_iend(pngenc_t *pngenc)
{
    pngenc_status_t ret;

    ret = write_chunk(pngenc, "IEND", NULL, 0U);
    if (ret != PNGENC_OK) {
        return ret;
    }

    return PNGENC_OK;
}

pngenc_status_t pngenc_init(pngenc_t *pngenc, const pngenc_io_t *io)
{
    if ((pngenc == NULL) ||
        (io == NULL) ||
        (io->write == NULL)) {
        return PNGENC_ERROR;
    }

    pngenc->io = io;

    return PNGENC_OK;
}

pngenc_status_t pngenc_encode(pngenc_t *pngenc, const pngenc_image_t *image, const pngenc_text_t *text)
{
    pngenc_status_t ret;
    size_t raw_len;
    size_t block_count;
    size_t idat_len;
    
    if ((pngenc == NULL) ||
        (image == NULL) ||
        (image->width == 0U) ||
        (image->height == 0U) ||
        ((text != NULL) &&
         ((text->keyword == NULL) ||
          ((text->data == NULL) &&
           (text->data_len != 0U))))) {
        return PNGENC_ERROR;
    }

    ret = write_signature(pngenc);
    if (ret != PNGENC_OK) {
        return ret;
    }

    ret = write_ihdr(pngenc, image->width, image->height);
    if (ret != PNGENC_OK) {
        return ret;
    }

    if (text != NULL) {
        ret = write_ztxt(pngenc, text->keyword, text->data, text->data_len);
        if (ret != PNGENC_OK) {
            return ret;
        }
    }

    raw_len = (size_t)image->height * (1U + (3U * (size_t)image->width));
    block_count = (raw_len + (PNGENC_IDAT_BLOCK_SIZE - 5U) - 1U) / (PNGENC_IDAT_BLOCK_SIZE - 5U);
    idat_len = 2U + raw_len + (5U * block_count) + 4U;

    ret = write_idat_begin(pngenc, idat_len);
    if (ret != PNGENC_OK) {
        return ret;
    }

    switch (image->pixel_format) {
        case PNGENC_PIXEL_FORMAT_RGB565:
            ret = write_idat_scanlines_rgb565(pngenc, image->data, image->width, image->height);
            break;

        default:
            ret = PNGENC_ERROR;
            break;
    }
    if (ret != PNGENC_OK) {
        return ret;
    }
 
    ret = write_idat_end(pngenc);
    if (ret != PNGENC_OK) {
        return ret;
    }

    ret = write_iend(pngenc);
    if (ret != PNGENC_OK) {
        return ret;
    }

    return PNGENC_OK;
}
