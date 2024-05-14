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


#include "Atlas.hpp"

#include <regex>
#include <fstream>
#include <dirent.h>

std::string& ltrim(std::string &str)
{
  auto it2 =  std::find_if( str.begin() , str.end() , [](char ch){ return !std::isspace<char>(ch , std::locale::classic() ) ; } );
  str.erase( str.begin() , it2);
  return str;
}

std::string& rtrim(std::string &str) {
  auto it1 =  std::find_if( str.rbegin() , str.rend() , [](char ch){ return !std::isspace<char>(ch , std::locale::classic() ) ; } );
  str.erase( it1.base() , str.end() );
  return str;
}

std::string& trim(std::string &str) {
   return ltrim(rtrim(str));
}

std::string trim_copy(const std::string &str) {
   auto s = str;
   return ltrim(rtrim(s));
}

static GFXtexture getGFXtexture(TImage *image, uint16_t offset, int maskColor)
{
    GFXtexture texture = {0, 0, 0, 0, 0, 0};
    int minX, maxX, minY, maxY;
    
    if (!image || !image->data) return texture;
    
    uint8_t *p = image->data;
    texture.maskColor = static_cast<color_t>(maskColor > -1 ? maskColor : *p);
    
    maxX = 0;
    maxY = 0;
    minX = image->width - 1;
    minY = image->height - 1;
    
    for (int y=0; y<image->height; y++) {
        for (int x=0; x<image->width; x++) {
            if (p[x + y * image->width] == texture.maskColor) continue;
            if (minX > x) minX = x;
            if (maxX < x) maxX = x;
            if (minY > y) minY = y;
            if (maxY < y) maxY = y;
        }
    }
    
    if (maxX < minX || maxY < minY) {
        return texture;
    }
    
    texture.bitmapOffset = offset;
    texture.width = maxX - minX + 1;
    texture.height = maxY - minY + 1;
    texture.dX = minX;
    texture.dY = minY;
    
    return texture;
}

static bool customCompare(const std::string &a, const std::string &b)
{
    return atoi(a.c_str()) < atoi(b.c_str()) ? true : false;
}


static void getDirList(const std::string &path, std::vector<std::string> &list)
{
    DIR *dirp = opendir(path.c_str());
    dirent *dp;
    
    while (( dp = readdir(dirp)) != NULL) {
        if (dp->d_type != DT_REG) continue;
        if (*dp->d_name == '.') continue;
        list.push_back(dp->d_name);
    }
    closedir(dirp);
    
    std::sort(list.begin(), list.end(), customCompare);
}


void Atlas::createFile(std::string &filename)
{
    std::string s = generateH();
    std::ofstream outfile;
    
   
    outfile.open(filename, std::ios::out | std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(s.c_str(), s.length());
        outfile.close();
    }
}

std::string Atlas::generateH(void)
{
    std::ostringstream os;
    
    os << "\
// Generated by piXatlas\n\
#ifndef PROGMEM\n\
    #define PROGMEM /* None Arduino */\n\
#endif\n\n\
#ifndef " << _name << "_h\n\
#define " << _name << "_h\n\n\
#include <stdint.h>\n\n\
const uint8_t " << _name << "_Bitmaps[] PROGMEM = {\n  \
" << std::setfill('0') << std::setw(2) << std::hex;
    int col = 32;
    for (int n = 0; n < _data.size(); n++) {
        if (n % col) os << " ";
        os << "0x" << std::setw(2) << (int)_data.at(n);
        if (n < _data.size()-1) os << ",";
        if (n % col == col - 1) os << "\n  ";
    }
    os << "\n};\n\n";
    
    os << "const GFXtexture " << _name << "_Textures[] PROGMEM = {\n";
    for (auto it=_textureEnteries.begin(); it!=_textureEnteries.end(); ++it) {
        os << it->data();
         
        if (next(it) != _textureEnteries.end()) os << ",\n";
    }
    os << std::dec << "\n};\n\n\
const GFXatlas " << _name << " PROGMEM = {(uint8_t *) " << _name << "_Bitmaps, (GFXtexture *) " << _name << "_Textures, 0, " << (int)_textureEnteries.size() << ", " << (int)_atlas.bitWidth << "};\n\n\
#endif /* " << _name << "_h */\n";
    return os.str();
    
}

void Atlas::addTexture(GFXtexture &texture)
{
    std::ostringstream os;
    
    os << "  { "
       << std::setw(5) << (int)texture.bitmapOffset << ","
       << std::setw(4) << (int)texture.width << ","
       << std::setw(4) << (int)texture.height << ","
       << std::setw(4) << (int)texture.maskColor << ","
       << std::setw(4) << (int)texture.dX << ","
       << std::setw(4) << (int)texture.dY << " }";
    
    _textureEnteries.push_back(os.str());
}

static std::vector<std::string> parseArguments(const std::string &arguments)
{
    std::string s;
    std::regex r;
    std::vector<std::string> parameters;
    s = regex_replace(arguments, std::regex(R"( *\w+ *: *)"), ",");
    
    r = R"([^,"]+)";
    for(std::sregex_iterator it = std::sregex_iterator(s.begin(), s.end(), r); it != std::sregex_iterator(); ++it) {
        parameters.push_back(trim_copy(it->str()));
    }
    
    return parameters;
}

uint16_t Atlas::concatenateImageData(TImage *image)
{
    color_t *byte = (color_t *)image->data;
    uint16_t length = image->width / (8 / _atlas.bitWidth) * image->height;
    for (int i = 0; i < length; i++) {
        if (_atlas.bitWidth == 8)
            _data.push_back(*byte++);
        
        if (_atlas.bitWidth == 4) {
            _data.push_back((byte[0] << 4) | byte[1]);
            byte+=2;
        }
    }
    return length;
}


void Atlas::imageNamed(const std::string &arguments)
{
    std::string s;
    s = regex_replace(arguments, std::regex(R"( *\w+ *: *)"), ",");
    
    std::vector<std::string> parameters = parseArguments(arguments);
    if (parameters.size() != 1)
        return;

    s = _path + parameters.at(0);
    reset(_image);
    _image = loadBMPGraphicFile(s);
    if (_image->bitWidth != 8)
        convertPixmapTo8BitPixmapNoCopy(_image);
}

void Atlas::texture(const std::string &arguments)
{
    if (!_image) {
        std::cout << "error: no image!\n";
        return;
    }
    
    if (!_image->data) {
        std::cout << "error: no image data!\n";
        return;
    }
    
    std::vector<std::string> parameters = parseArguments(arguments);
    
   
    if (parameters.size() != 5)
        return;
    
    int x = atoi(parameters.at(0).c_str());
    int y = atoi(parameters.at(1).c_str());
    int w = atoi(parameters.at(2).c_str());
    int h = atoi(parameters.at(3).c_str());
    int maskColor = parameters.at(4) == "auto" ? -1 : atoi(parameters.at(4).c_str());
    
    if (w == 0)
        w = _image->width;
    
    if (h == 0)
        h = _image->height;
          
    
    
    TImage *grabedImage = createPixmap(w, h);
    if (!grabedImage || !grabedImage->data) return;
    copyPixmap(grabedImage, 0, 0, _image, x, y, w, h);

    GFXtexture texture = getGFXtexture(grabedImage, _data.size(), maskColor);
    
    TImage *extractedImage = extractImageSectionMasked(grabedImage, texture.maskColor);
    reset(grabedImage);

    if (!extractedImage) {
        return;
    }
    
    concatenateImageData(extractedImage);
    reset(extractedImage);
    
    addTexture(texture);
    
    _atlas.textures++;
}

void Atlas::textures(const std::string &arguments)
{
    if (!_image) {
        std::cout << "error: no image!\n";
        return;
    }
    
    if (!_image->data) {
        std::cout << "error: no image data!\n";
        return;
    }
    
    std::vector<std::string> parameters = parseArguments(arguments);
    
   
    if (parameters.size() != 3)
        return;
    
    int w = atoi(parameters.at(0).c_str());
    int h = atoi(parameters.at(1).c_str());
    int maskColor = parameters.at(2) == "auto" ? -1 : atoi(parameters.at(2).c_str());
    
    if (w == 0 || h == 0)
        return;
    
    TImage *grabedImage = createPixmap(w, h);
    if (!grabedImage || !grabedImage->data) return;
    
    for (int y=0; y<_image->height; y+=h) {
        for (int x=0; x<_image->width; x+=w) {
            copyPixmap(grabedImage, 0, 0, _image, x, y, w, h);
            GFXtexture texture = getGFXtexture(grabedImage, _data.size(), maskColor);
            TImage *extractedImage = extractImageSectionMasked(grabedImage, texture.maskColor);
            if (!extractedImage) {
                reset(grabedImage);
                return;
            }
            concatenateImageData(extractedImage);
            reset(extractedImage);
            
            addTexture(texture);
            _atlas.textures++;
        }
    }
    
    reset(grabedImage);
}


void Atlas::proccessDirectory(const std::string &directory, int maskColor)
{
    std::vector<std::string> list;
    TImage *image;
    uint16_t offset = 0;
    
    
    getDirList(directory, list);
    if (list.empty())
        return;
    
    for (auto it=list.begin(); it!=list.end(); ++it) {
        std::string pathname = directory + "/" + it->data();
        if ((image = loadBMPGraphicFile(pathname)) == nullptr)
            continue;
        
        if (image->bitWidth != 8)
            convertPixmapTo8BitPixmapNoCopy(image);
        
        GFXtexture texture = getGFXtexture(image, offset, maskColor);
            
        TImage *extractedImage = extractImageSectionMasked(image, texture.maskColor);
        reset(image);
    
        if (!extractedImage) {
            continue;
        }
        
        offset += concatenateImageData(extractedImage);
        reset(extractedImage);
        
        // Using filename as lable, but without the file extention part.
        std::string label = regex_replace(it->data(), std::regex(R"(\.\w+$)"), "");
        addTexture(texture);
        
        _atlas.textures++;
    }
    
}


void Atlas::proccessScript(const std::string &filename, int maskColor)
{
    std::ifstream infile;
    
    infile.open(filename,std::ios::in);
    if (!infile.is_open()) exit(2);
    
    _path = filename.substr(0, filename.rfind("/") + 1);
    
    
    std::string s;
    
    while(getline(infile, s)) {
        /*
         eg. fn:arguments;
         Group  0 fn:arguments;
                1 fn
                2 arguments
         */
        std::regex r;
        r = R"(([a-zA-Z]\w*) *: *(.*);)";
        std::sregex_token_iterator it = std::sregex_token_iterator {
            s.begin(), s.end(), r, {1, 2}
        };
        if (it != std::sregex_token_iterator()) {
            if (*it == "imageNamed") {
                imageNamed(*++it);
            }
            
            if (*it == "texture") {
                texture(*++it);
            }
            
            if (*it == "textures") {
                textures(*++it);
            }
        }
    }
    
    
    infile.close();
}

