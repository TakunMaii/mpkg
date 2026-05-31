#ifndef MPKG_H
#define MPKG_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//MPG
#define MPKGMAGIC 0x0047504D
#define RES_LABEL_LENGTH 128

typedef struct {
    char *start_pointer;
    long length;
} Mpkg;

typedef struct {
    uint32_t magic;
    uint32_t item_count;
} MpkgHeader;

typedef struct {
    char path[RES_LABEL_LENGTH];
    long offset;
    long length;
} MpkgResLabel;

Mpkg OpenMpkg(const char *path);
char* FetchDataFromMpkg(Mpkg pkg, const char* path, long *length);
MpkgResLabel* FetchResLabels(Mpkg pkg, uint32_t *size);

#ifdef MPKG_IMPLEMENTATION

Mpkg OpenMpkg(const char *path)
{
    FILE *file = fopen(path, "rb");
    if(!file)
    {
        printf("Failed to open file %d\n", path);
        return (Mpkg){0};
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *data = (char*)malloc(length);
    size_t read_result = fread(data, length, 1, file);
    if(!read_result)
    {
        printf("Failed to read file %s\n", path);
        fclose(file);
        return (Mpkg){0};
    }

    fclose(file);
    return (Mpkg) {
        .start_pointer = data,
        .length = length
    };
}

char* FetchDataFromMpkg(Mpkg pkg, const char* path, long *length)
{
    char *pointer = pkg.start_pointer;
    MpkgHeader *header = (MpkgHeader*)pointer;
    pointer += sizeof(MpkgHeader);
    for(uint32_t i = 0; i < header->item_count ; i++)
    {
        MpkgResLabel *label = (MpkgResLabel*)pointer;
        if(!strcmp(path, label->path))
        {
            *length = label->length;
            return pkg.start_pointer + label->offset;
        }

        pointer += sizeof(MpkgResLabel);
    }

    printf("Cannot find %s\n", path);
    *length = 0;
    return NULL;
}

MpkgResLabel* FetchResLabels(Mpkg pkg, uint32_t *size)
{
    char *pointer = pkg.start_pointer;
    MpkgHeader *header = (MpkgHeader*)pointer;
    pointer += sizeof(MpkgHeader);

    *size = header->item_count;

    return (MpkgResLabel*) pointer;
}
#endif

#endif
