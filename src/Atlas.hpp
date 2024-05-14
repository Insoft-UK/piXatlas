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

#ifndef Atlas_hpp
#define Atlas_hpp

#include <stdio.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "GFXAtlas.h"
#include "image.hpp"

class Atlas {
private:
    
    std::vector<std::string> _textureEnteries;
    std::vector<uint8_t> _data;
    std::string _path, _name;
    TImage *_image = nullptr;
    GFXatlas _atlas;
    
    uint16_t concatenateImageData(TImage *image);
    void imageNamed(const std::string &arguments);
    void texture(const std::string &arguments);
    void textures(const std::string &arguments);
    void parseLine(const std::string& str);
    std::string generateH(void);
    void addTexture(GFXtexture &texture);
    
public:
    Atlas(const GFXatlas &atlas, const std::string &name)
    {
        _atlas = atlas;
        _name = name;
    }
    void proccessDirectory(const std::string &directory, int maskColor);
    void proccessScript(const std::string &filename, int maskColor);
    void createFile(std::string &filename);
    
    
};

#endif /* Atlas_hpp */
