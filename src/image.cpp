/*
 Copyright © 2024 Insoft. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "image.hpp"

#include <fstream>

/* Windows 3.x bitmap file header */
typedef struct __attribute__((__packed__)) {
    char      bfType[2];   /* magic - always 'B' 'M' */
    uint32_t  bfSize;
    uint16_t  bfReserved1;
    uint16_t  bfReserved2;
    uint32_t  bfOffBits;    /* offset in bytes to actual bitmap data */
} BMPHeader;

/* Windows 3.x bitmap full header, including file header */

typedef struct __attribute__((__packed__)) {
    BMPHeader fileHeader;
    uint32_t  biSize;
    int32_t   biWidth;
    int32_t   biHeight;
    int16_t   biPlanes;           // Number of colour planes, set to 1
    int16_t   biBitCount;         // Colour bits per pixel. 1 4 8 24 or 32
    uint32_t  biCompression;      // *Code for the compression scheme
    uint32_t  biSizeImage;        // *Size of the bitmap bits in bytes
    int32_t   biXPelsPerMeter;    // *Horizontal resolution in pixels per meter
    int32_t   biYPelsPerMeter;    // *Vertical resolution in pixels per meter
    uint32_t  biClrUsed;          // *Number of colours defined in the palette
    uint32_t  biClImportant;      // *Number of important colours in the image
} BIPHeader;

static void flipImageVertically(const TImage *image)
{
    uint8_t *byte = (uint8_t *)image->data;
    int w = (int)image->width;
    int h = (int)image->height;
    for (int row = 0; row < h / 2; ++row)
        for (int col = 0; col < w; ++col)
            std::swap(byte[col + row * w], byte[col + (h - 1 - row) * w]);
    
}

TImage *loadBMPGraphicFile(std::string &filename)
{
    BIPHeader bip_header;
    
    std::ifstream infile;
    
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    infile.open(filename, std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
        free(image);
        return nullptr;
    }
    
    infile.read((char *)&bip_header, sizeof(BIPHeader));

    if (strncmp(bip_header.fileHeader.bfType, "BM", 2) != 0) {
        infile.close();
        free(image);
        return nullptr;
    }
    
    image->bitWidth = bip_header.biBitCount;
    image->data = (unsigned char *)malloc(bip_header.biSizeImage);
    if (!image->data) {
        free(image);
        infile.close();
        return nullptr;
    }
    
    image->width = abs(bip_header.biWidth);
    image->height = abs(bip_header.biHeight);
    int32_t bytesPerRow = bip_header.biWidth / (8 / bip_header.biBitCount);
    int32_t len = bytesPerRow * bip_header.biHeight;
    
    infile.seekg(bip_header.biClrUsed * 4, std::ios_base::cur);
    infile.read((char *)image->data, len);
    
    if (infile.gcount() != len)
        std::cout << filename << " Read failed!\n";
    infile.close();
    
    if (bip_header.biHeight > 0)
        flipImageVertically(image);
    
    return image;
}

TImage *loadPBMGraphicFile(std::string &filename)
{
    std::ifstream infile;
    
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    infile.open(filename, std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
        free(image);
        return nullptr;
    }
    
    std::string s;
    
    getline(infile, s);
    if (s != "P4") {
        infile.close();
        return image;
    }
    
    image->bitWidth = 1;
    
    getline(infile, s);
    image->width = atoi(s.c_str());
    
    getline(infile, s);
    image->height = atoi(s.c_str());
    
    size_t length = ((image->width + 7) >> 3) * image->height;
    image->data = (unsigned char *)malloc(length);
    
    if (!image->data) {
        free(image);
        infile.close();
        return nullptr;
    }
    infile.read((char *)image->data, length);
    
    infile.close();
    return image;
}

TImage *createBitmap(int w, int h)
{
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    w = (w + 7) & ~7;
    image->data = malloc(w * h / 8);
    if (!image->data) {
        free(image);
        return nullptr;
    }
    
    image->bitWidth = 1;
    image->width = w;
    image->height = h;
    
    return image;
}

TImage *createPixmap(int w, int h)
{
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    image->data = malloc(w * h);
    if (!image->data) {
        free(image);
        return nullptr;
    }
    
    image->bitWidth = 8;
    image->width = w;
    image->height = h;
    
    return image;
}

void copyPixmap(const TImage *dst, int dx, int dy, const TImage *src, int x, int y, uint16_t w, uint16_t h)
{
    uint8_t *d = (uint8_t *)dst->data;
    uint8_t *s = (uint8_t *)src->data;
    
    d += dx + dy * dst->width;
    s += x + y * src->width;
    while (h--) {
        for (int i=0; i<w; i++) {
            d[i] = s[i];
        }
        d += dst->width;
        s += src->width;
    }
}

TImage *convertMonochromeBitmapToPixmap(const TImage *monochrome)
{
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image)
        return nullptr;
    
    uint8_t *src = (uint8_t *)monochrome->data;
    uint8_t bitPosition = 1 << 7;
    
    image->bitWidth = 8;
    image->width = monochrome->width;
    image->height = monochrome->height;
    image->data = malloc(image->width * image->height);
    if (!image->data) return image;
    
    memset(image->data, 0, image->width * image->height);
    
    uint8_t *dest = (uint8_t *)image->data;
    
    int x, y;
    for (y=0; y<monochrome->height; y++) {
        bitPosition = 1 << 7;
        for (x=0; x<monochrome->width; x++) {
            *dest++ = (*src & bitPosition ? 1 : 0);
            if (bitPosition == 1) {
                src++;
                bitPosition = 1 << 7;
            } else {
                bitPosition >>= 1;
            }
        }
        if (x & 7) src++;
    }
    
    return image;
}

TImage *convertPixmapTo8BitPixmap(const TImage *pixmap)
{
    if (pixmap->bitWidth != 4)
        return nullptr;
    
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image)
        return nullptr;
    
    uint8_t *src = (uint8_t *)pixmap->data;
    
    image->bitWidth = 8;
    image->width = pixmap->width;
    image->height = pixmap->height;
    image->data = malloc(image->width * image->height);
    if (!image->data) return image;
    
    memset(image->data, 0, image->width * image->height);
    
    uint8_t *dest = (uint8_t *)image->data;
    
    int length = pixmap->width * pixmap->height;
    
    while (length--) {
        uint8_t byte = *src++;
        if (pixmap->bitWidth == 4) {
            *dest++ = byte >> 4;
            *dest++ = byte & 15;
        }
        if (pixmap->bitWidth == 2) {
            *dest++ = byte >> 6;
            *dest++ = byte >> 4 & 3;
            *dest++ = byte >> 2 & 3;
            *dest++ = byte & 3;
        }
    }
    
    return image;
}

void convertPixmapTo8BitPixmapNoCopy(TImage *pixmap)
{
    if (pixmap->bitWidth != 4 && pixmap->bitWidth != 2)
        return;
    
    uint8_t *dest = (uint8_t *)malloc(pixmap->width * pixmap->height);
    if (!dest)
        return;
    uint8_t *src = (uint8_t *)pixmap->data;
    
    int length = pixmap->width * pixmap->height;
    
    while (length--) {
        uint8_t byte = *src++;
        if (pixmap->bitWidth == 4) {
            *dest++ = byte >> 4;
            *dest++ = byte & 15;
        }
        if (pixmap->bitWidth == 2) {
            *dest++ = byte >> 6;
            *dest++ = byte >> 4 & 3;
            *dest++ = byte >> 2 & 3;
            *dest++ = byte & 3;
        }
    }
    
    free(pixmap->data);
    pixmap->data = dest;
    pixmap->bitWidth = 8;
}

void reset(TImage *&image)
{
    if (image) {
        if (image->data) free(image->data);
        free(image);
        image = nullptr;
    }
}

bool containsImage(const TImage *image, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (!image || !image->data) return false;
    if (x + w > image->width || y + h > image->height) return false;
    uint8_t *p = (uint8_t *)image->data;
    
    p += x + y * image->width;
    while (h--) {
        for (int i=0; i<w; i++) {
            if (p[i]) return true;
        }
        p+=image->width;
    }
    return false;
}

TImage *extractImageSection(TImage *image)
{
    TImage *extractedImage = nullptr;
    extractedImage = extractImageSectionMasked(image, 0);
    return extractedImage;
}

TImage *extractImageSectionMasked(TImage *image, uint8_t maskColor)
{
    TImage *extractedImage = nullptr;
    
    int minX, maxX, minY, maxY;
    
    if (!image || !image->data) return nullptr;
    
    uint8_t *p = (uint8_t *)image->data;
    
    maxX = 0;
    maxY = 0;
    minX = image->width - 1;
    minY = image->height - 1;
    
    for (int y=0; y<image->height; y++) {
        for (int x=0; x<image->width; x++) {
            if (p[x + y * image->width] != maskColor) continue;
            if (minX > x) minX = x;
            if (maxX < x) maxX = x;
            if (minY > y) minY = y;
            if (maxY < y) maxY = y;
        }
    }
    
    if (maxX < minX || maxY < minY)
        return nullptr;
    
    
    int width = maxX - minX + 1;
    int height = maxY - minY + 1;
    
    extractedImage = createPixmap(width, height);
    if (!extractedImage) return nullptr;
    copyPixmap(extractedImage, 0, 0, image, minX, minY, width, height);
    
    return extractedImage;
}
