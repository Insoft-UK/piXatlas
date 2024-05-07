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

#ifndef GFXAtlas_h
#define GFXAtlas_h

#ifndef color_t
#define color_t unsigned char
#endif

typedef struct {
    uint16_t bitmapOffset;  // Offset address to the bitmap data for this sprite.
    uint8_t  width, height; // Bitmap dimensions in pixels.
    color_t  maskColor;
    int8_t   dX;            // Used to position the sprite in the horizontal direction.
    int8_t   dY;            // Used to position the sprite in the vertical direction.
} GFXtexture;

typedef struct {
    color_t    *data;
    GFXtexture *texture;   // Parameter block data.
    uint8_t    _reserved;
    uint8_t     blocks;
    uint8_t     bitWidth;  // Bits per pixel for color depth, typically 4 or 8.
} GFXatlas;

#endif /* GFXAtlas_h */
