#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static uint32_t png_crc_table[256];
static int png_crc_table_ready = 0;

static void make_png_crc_table(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            if (c & 1) c = 0xEDB88320L ^ (c >> 1);
            else c = c >> 1;
        }
        png_crc_table[n] = c;
    }
    png_crc_table_ready = 1;
}

static uint32_t update_png_crc(uint32_t crc, const uint8_t *buf, size_t len) {
    if (!png_crc_table_ready) make_png_crc_table();
    uint32_t c = crc;
    for (size_t n = 0; n < len; n++) {
        c = png_crc_table[(c ^ buf[n]) & 0xFF] ^ (c >> 8);
    }
    return c;
}

static uint32_t calc_png_crc(const uint8_t *buf, size_t len) {
    return update_png_crc(0xFFFFFFFFL, buf, len) ^ 0xFFFFFFFFL;
}

static uint32_t calc_adler32(const uint8_t *data, size_t len) {
    uint32_t a = 1;
    uint32_t b = 0;
    for (size_t i = 0; i < len; i++) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

uint8_t *png_encode_grayscale(const uint8_t *pixels, size_t width, size_t height, size_t *out_png_len) {
    if (!pixels || width == 0 || height == 0 || !out_png_len) return NULL;

    // Each row in uncompressed PNG scanline starts with filter byte 0x00
    size_t raw_scanlines_len = height * (1 + width);
    uint8_t *raw_scanlines = (uint8_t *)malloc(raw_scanlines_len);
    if (!raw_scanlines) return NULL;

    for (size_t r = 0; r < height; r++) {
        size_t offset = r * (1 + width);
        raw_scanlines[offset] = 0x00; // Filter None
        memcpy(raw_scanlines + offset + 1, pixels + r * width, width);
    }

    // Wrap in uncompressed Deflate blocks (type 00)
    // Max uncompressed block size is 65535 bytes
    size_t num_blocks = (raw_scanlines_len + 65534) / 65535;
    if (num_blocks == 0) num_blocks = 1;
    size_t zlib_len = 2 + (num_blocks * 5) + raw_scanlines_len + 4;
    uint8_t *zlib_data = (uint8_t *)malloc(zlib_len);
    if (!zlib_data) {
        free(raw_scanlines);
        return NULL;
    }

    // zlib header: CMF = 0x78 (deflate, 32K window), FLG = 0x01 (check bits)
    zlib_data[0] = 0x78;
    zlib_data[1] = 0x01;
    size_t z_cur = 2;

    size_t remaining = raw_scanlines_len;
    size_t in_cur = 0;

    for (size_t b = 0; b < num_blocks; b++) {
        size_t chunk_len = (remaining > 65535) ? 65535 : remaining;
        uint8_t bfinal = (b == num_blocks - 1) ? 0x01 : 0x00;
        zlib_data[z_cur++] = bfinal; // BFINAL + BTYPE=00 (uncompressed)

        uint16_t len16 = (uint16_t)chunk_len;
        uint16_t nlen16 = ~len16;

        zlib_data[z_cur++] = (uint8_t)(len16 & 0xFF);
        zlib_data[z_cur++] = (uint8_t)((len16 >> 8) & 0xFF);
        zlib_data[z_cur++] = (uint8_t)(nlen16 & 0xFF);
        zlib_data[z_cur++] = (uint8_t)((nlen16 >> 8) & 0xFF);

        memcpy(zlib_data + z_cur, raw_scanlines + in_cur, chunk_len);
        z_cur += chunk_len;
        in_cur += chunk_len;
        remaining -= chunk_len;
    }

    // Append Adler-32
    uint32_t adler = calc_adler32(raw_scanlines, raw_scanlines_len);
    zlib_data[z_cur++] = (uint8_t)((adler >> 24) & 0xFF);
    zlib_data[z_cur++] = (uint8_t)((adler >> 16) & 0xFF);
    zlib_data[z_cur++] = (uint8_t)((adler >> 8) & 0xFF);
    zlib_data[z_cur++] = (uint8_t)(adler & 0xFF);

    free(raw_scanlines);

    // Build complete PNG file
    // Signature (8) + IHDR chunk (25) + IDAT chunk (12 + z_cur) + IEND chunk (12)
    size_t total_png = 8 + 25 + (12 + z_cur) + 12;
    uint8_t *png = (uint8_t *)malloc(total_png);
    if (!png) {
        free(zlib_data);
        return NULL;
    }

    size_t p = 0;
    // 1. Signature
    const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    memcpy(png + p, sig, 8);
    p += 8;

    // 2. IHDR Chunk (13 bytes payload)
    png[p++] = 0; png[p++] = 0; png[p++] = 0; png[p++] = 13;
    size_t ihdr_start = p;
    png[p++] = 'I'; png[p++] = 'H'; png[p++] = 'D'; png[p++] = 'R';
    png[p++] = (uint8_t)((width >> 24) & 0xFF);
    png[p++] = (uint8_t)((width >> 16) & 0xFF);
    png[p++] = (uint8_t)((width >> 8) & 0xFF);
    png[p++] = (uint8_t)(width & 0xFF);
    png[p++] = (uint8_t)((height >> 24) & 0xFF);
    png[p++] = (uint8_t)((height >> 16) & 0xFF);
    png[p++] = (uint8_t)((height >> 8) & 0xFF);
    png[p++] = (uint8_t)(height & 0xFF);
    png[p++] = 8;    // Bit depth: 8
    png[p++] = 0;    // Color type: 0 (Grayscale)
    png[p++] = 0;    // Compression method: 0 (Deflate)
    png[p++] = 0;    // Filter method: 0 (Standard)
    png[p++] = 0;    // Interlace method: 0 (No interlace)
    uint32_t ihdr_crc = calc_png_crc(png + ihdr_start, 17);
    png[p++] = (uint8_t)((ihdr_crc >> 24) & 0xFF);
    png[p++] = (uint8_t)((ihdr_crc >> 16) & 0xFF);
    png[p++] = (uint8_t)((ihdr_crc >> 8) & 0xFF);
    png[p++] = (uint8_t)(ihdr_crc & 0xFF);

    // 3. IDAT Chunk
    png[p++] = (uint8_t)((z_cur >> 24) & 0xFF);
    png[p++] = (uint8_t)((z_cur >> 16) & 0xFF);
    png[p++] = (uint8_t)((z_cur >> 8) & 0xFF);
    png[p++] = (uint8_t)(z_cur & 0xFF);
    size_t idat_start = p;
    png[p++] = 'I'; png[p++] = 'D'; png[p++] = 'A'; png[p++] = 'T';
    memcpy(png + p, zlib_data, z_cur);
    p += z_cur;
    uint32_t idat_crc = calc_png_crc(png + idat_start, 4 + z_cur);
    png[p++] = (uint8_t)((idat_crc >> 24) & 0xFF);
    png[p++] = (uint8_t)((idat_crc >> 16) & 0xFF);
    png[p++] = (uint8_t)((idat_crc >> 8) & 0xFF);
    png[p++] = (uint8_t)(idat_crc & 0xFF);

    free(zlib_data);

    // 4. IEND Chunk
    png[p++] = 0; png[p++] = 0; png[p++] = 0; png[p++] = 0;
    png[p++] = 'I'; png[p++] = 'E'; png[p++] = 'N'; png[p++] = 'D';
    const uint8_t iend_type[4] = { 'I', 'E', 'N', 'D' };
    uint32_t iend_crc = calc_png_crc(iend_type, 4);
    png[p++] = (uint8_t)((iend_crc >> 24) & 0xFF);
    png[p++] = (uint8_t)((iend_crc >> 16) & 0xFF);
    png[p++] = (uint8_t)((iend_crc >> 8) & 0xFF);
    png[p++] = (uint8_t)(iend_crc & 0xFF);

    *out_png_len = p;
    return png;
}
