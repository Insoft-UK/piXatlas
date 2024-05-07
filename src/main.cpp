/*
 Copyright (C) 2014 by Insoft
 
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


#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <fstream>
#include <iomanip>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#include "GFXAtlas.h"
#include "image.hpp"

#include "build.h"

bool verbose = false;

enum class MessageType {
    Warning,
    Error,
    Verbose
};

std::ostream& operator<<(std::ostream& os, MessageType type) {
    switch (type) {
        case MessageType::Error:
            os << "\033[91;1;1;1merror\033[0m: ";
            break;

        case MessageType::Warning:
            os << "\033[98;1;1;1mwarning\033[0m: ";
            break;
            
        case MessageType::Verbose:
            os << ": ";
            break;

        default:
            os << ": ";
            break;
    }

    return os;
}

void usage(void)
{
    std::cout << "Copyright (C) 2024 Insoft. All rights reserved.\n";
    std::cout << "Insoft GFX Pixel Sprite Atlas Creator.\n\n";
    std::cout << "Usage: pixsprite folder [-options]\n";
    std::cout << "\n";
    
    // TODO: Add verbose
    std::cout << " -v display detailed processing information\n";
    std::cout << "\tFlags:\n";
    std::cout << "\t\ta atlas\n";
    std::cout << "\t\tt texture\n\n";
    
    std::cout << " -b\tBits per pixel for color depth, 4 or 8(default).\n";
    std::cout << " -m\tColor that should be treated as transparent.\n";
    std::cout << "\n";
    std::cout << "Usage: pixatlas {-version | -help}\n";
}

void error(void)
{
    std::cout << "pixatlas: try 'pixatlas -help' for more information\n";
    exit(0);
}

void version(void) {
    std::cout
    << "Version: piXatlas "
    << (unsigned)__BUILD_NUMBER / 100000 << "."
    << (unsigned)__BUILD_NUMBER / 10000 % 10 << "."
    << (unsigned)__BUILD_NUMBER / 1000 % 10 << "."
    << std::setfill('0') << std::setw(3) << (unsigned)__BUILD_NUMBER % 1000
    << "\n";
    std::cout << "Copyright: (C) 2024 Insoft. All rights reserved.\n";
}

GFXtexture autoGFXblockSettings(TImage *image)
{
    GFXtexture block = {0, 0, 0, 0, 0, 0};
    int minX, maxX, minY, maxY;
    
    if (!image || !image->data) return block;
    
    uint8_t *p = (uint8_t *)image->data;
    
    maxX = 0;
    maxY = 0;
    minX = image->width - 1;
    minY = image->height - 1;
    
    block.maskColor = *p;
    
    for (int y=0; y<image->height; y++) {
        for (int x=0; x<image->width; x++) {
            if (p[x + y * image->width] != block.maskColor) continue;
            if (minX > x) minX = x;
            if (maxX < x) maxX = x;
            if (minY > y) minY = y;
            if (maxY < y) maxY = y;
        }
    }
    
    if (maxX < minX || maxY < minY) {
        return block;
    }
    
    block.bitmapOffset = 0;
    block.width = maxX - minX + 1;
    block.height = maxY - minY + 1;
    block.dX = minX;
    block.dY = minY;
    
    return block;
}

void concatenateImageData(TImage *image, std::vector<uint8_t> &data, uint8_t bitWidth)
{
    color_t *byte = (color_t *)image->data;
    int length = image->width / (8 / bitWidth) * image->height;
    for (int i = 0; i < length; i++) {
        if (bitWidth == 8)
            data.push_back(*byte++);
        
        if (bitWidth == 4) {
            data.push_back((byte[0] << 4) | byte[1]);
            byte+=2;
        }
    }
}

std::string addBlock(std::string &label, GFXtexture &texture, GFXatlas &atlas)
{
    std::ostringstream os;
    
    if (atlas.blocks) os << ",\n";
    os << "  { ";
    os << std::setw(5) << (int)texture.bitmapOffset << ","
       << std::setw(4) << (int)texture.width << ","
       << std::setw(4) << (int)texture.height << ","
       << std::setw(4) << (int)texture.maskColor << ","
       << std::setw(4) << (int)texture.dX << ","
       << std::setw(4) << (int)texture.dY << " } ";
    
    os << "/* " << std::setfill(' ') << std::setw(4) << (int)atlas.blocks << std::dec << " '" << label << "' */";

    
    return os.str();
}

void createFile(std::string &filename, std::ostringstream &os)
{
    std::ofstream outfile;
   
    outfile.open(filename, std::ios::out | std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(os.str().c_str(), os.str().length());
        outfile.close();
    }
}

void processAndCreateFile(std::string &filename, GFXatlas &atlas, std::vector<uint8_t> &data, std::ostringstream &osBlock, std::string &name)
{
    std::ostringstream os;
    
    os << "\
// Generated by piXatlas\n\
#ifndef PROGMEM\n\
    #define PROGMEM /* None Arduino */\n\
#endif\n\n\
#ifndef " << name << "_h\n\
#define " << name << "_h\n\n\
#include <stdint.h>\n\n\
const uint8_t " << name << "_Bitmaps[] PROGMEM = {\n  \
" << std::setfill('0') << std::setw(2) << std::hex;
    int col = 32;
    for (int n = 0; n < data.size(); n++) {
        if (n % col) os << " ";
        os << "0x" << std::setw(2) << (int)data.at(n);
        if (n < data.size()-1) os << ",";
        if (n % col == col - 1) os << "\n  ";
    }
    os << "\n};\n\n";
    
    os << "const GFXtexture " << name << "_Textures[] PROGMEM = {\n";
    os << osBlock.str() << std::dec << "\n\
};\n\
const GFXatlas " << name << " PROGMEM = {(uint8_t *) " << name << "_Bitmaps, (GFXtexture *) " << name << "_Textures, " << (int)atlas._reserved << ", " << (int)atlas.blocks << ", " << (int)atlas.bitWidth << "};\n\n\
#endif /* " << name << "_h */\n";
    
    createFile(filename, os);
}

// TODO: Add extended support for none monochrome glyphs.

bool customCompare(const std::string &a, const std::string &b)
{

    return atoi(a.c_str()) < atoi(b.c_str()) ? true : false;
}

void createNewAtlas(std::string &filename, std::string &path, std::string &name, GFXatlas &atlas, int maskColor)
{
    TImage *image;
    std::vector<uint8_t> data;
    std::vector<std::string> list;
    std::ostringstream osTexture;
    std::string label;
    
    DIR *dirp = opendir(path.c_str());
    dirent *dp;
    uint16_t offset = 0;

    while (( dp = readdir(dirp)) != NULL) {
        if (dp->d_type != DT_REG) continue;
        if (*dp->d_name == '.') continue;
        list.push_back(dp->d_name);
    }
    
    std::sort(list.begin(), list.end(), customCompare);
    
    for (auto it=list.begin(); it!=list.end(); ++it) {
        std::string pathname = path + "/" + it->data();
        image = loadBMPGraphicFile(pathname);
        if (image == nullptr) continue;
        
        convertPixmapTo8BitPixmapNoCopy(image); // Will not convert anything if allready 8-bit
        
        GFXtexture texture = autoGFXblockSettings(image);
        if (maskColor > -1)
            texture.maskColor = (color_t)maskColor;
        TImage *extractedImage = extractImageSectionMasked(image, texture.maskColor);
        reset(image);
        
        if (!extractedImage) {
            label = "";
            osTexture << addBlock(label, texture, atlas);
            continue;
        }
        
        concatenateImageData(extractedImage, data, atlas.bitWidth);
        texture.bitmapOffset = offset;
        offset += (extractedImage->width / (8 / atlas.bitWidth) * extractedImage->height);
        reset(extractedImage);
        
        // Using filename as lable, but without the file extention part.
        label = regex_replace(it->data(), std::regex(R"(\.\w+$)"), "");
        osTexture << addBlock(label, texture, atlas);
        atlas.blocks++;
        if (verbose)
            std::cout << MessageType::Verbose << "path: " << path << "/" << it->data() << "\n";
        
    }
    closedir(dirp);
    
    
    processAndCreateFile(filename, atlas, data, osTexture, name);
}

int main(int argc, const char * argv[])
{
    if ( argc == 1 ) {
        error();
        return 0;
    }
    
    std::string path, name, filename;
    GFXatlas atlas = {0, 0, 0, 0, 8};
    int maskColor = -1;
    
    for( int n = 1; n < argc; n++ ) {
        if (*argv[n] == '-') {
            std::string args(argv[n]);
            
            if (args == "-b") {
                if (++n > argc) error();
                atlas.bitWidth = atoi(argv[n]);
                continue;
            }
            
            if (args == "-m") {
                if (++n > argc) error();
                maskColor = atoi(argv[n]);
                continue;
            }
            
            if (args == "-o") {
                if (++n > argc) error();
                filename = argv[n];
                continue;
            }
            
            if (args == "-help") {
                usage();
                return 0;
            }
            
            if (args == "-version") {
                version();
                return 0;
            }
            
            error();
            return 0;
        }
        path = argv[n];
    }
    
    
    if (name.empty()) {
        name = regex_replace(path, std::regex(R"(\.\w+$)"), "");
        size_t pos = name.rfind("/");
        name = name.substr(pos + 1, name.length() - pos);
    }
    
    if (filename.empty()) {
        filename = path + ".h";
    }
    
    createNewAtlas(filename, path, name, atlas, maskColor);
    return 0;
}

