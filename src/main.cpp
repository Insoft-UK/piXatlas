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
#include "Atlas.hpp"
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


int main(int argc, const char * argv[])
{
    if ( argc == 1 ) {
        error();
        return 0;
    }
    
    std::string directory, name, out_filename, in_filename;
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
                out_filename = argv[n];
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
        in_filename = argv[n];
    }
    
    
    if (name.empty()) {
        name = regex_replace(in_filename, std::regex(R"(\.\w+$)"), "");
        size_t pos = name.rfind("/");
        name = name.substr(pos + 1, name.length() - pos);
    }
    
    
    if (out_filename.empty()) {
        size_t pos = in_filename.rfind(".atlas");
        if (pos != std::string::npos) {
            out_filename = in_filename.substr(0, pos) + ".h";
        } else {
            directory = in_filename;
            out_filename = directory + ".h";
        }
    }
    
    Atlas _atlas(atlas, name);
    
    if (in_filename.rfind(".atlas") != std::string::npos) {
        _atlas.proccessScript(in_filename, maskColor);
        _atlas.createFile(out_filename);
        return 0;
    }
    
    _atlas.proccessDirectory(directory, maskColor);
    _atlas.createFile(out_filename);
    
    return 0;
}

