# mpkg

This is a simple resources packing system.

+ `mpkg.c` - source code for the packer program.
+ `mpkg.h` - header file for unpacking usage in C/CXX code.
+ `assets/` - example files

## Usage

Packer program:
```bash
$ gcc mpkg.c -o mpkg
$ ./mpkg output.pak assets
```

Unpacking header:
```
#define MPKG_IMPLEMENTATION
#include "mpkg.h"

// used with raylib
Texture2D MpkgLoadTexture(Mpkg mpkg, const char* type, const char* path)
{
    long size;
    char *data = FetchDataFromMpkg(mpkg, path, &size);

    Image image = LoadImageFromMemory(type, data, (int)size);
    return LoadTextureFromImage(image);
}
```
