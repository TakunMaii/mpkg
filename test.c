#define MPKG_IMPLEMENTATION
#include "mpkg.h"
#include <stdio.h>
#include <stdint.h>

int main()
{
    Mpkg mpkg = OpenMpkg("output.pak");

    uint32_t size;
    MpkgResLabel* labels = FetchResLabels(mpkg, &size);

    for(long i = 0; i < size; i++)
    {
        printf("=====Fetching %s=====\n", labels[i].path);
        
        long length;
        char *data = FetchDataFromMpkg(mpkg, labels[i].path, &length);
        char *str = (char*)malloc(length+1);
        memcpy(str, data, length);
        str[length] = 0;
        printf("%s", str);

        printf("=====END %s=====\n\n", labels[i].path);
    }
}
