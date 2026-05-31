#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

//MPG
#define MPKGMAGIC 0x0047504D
#define RES_LABEL_LENGTH 128

typedef struct {
    uint32_t magic;
    uint32_t item_count;
} MpkgHeader;

typedef struct {
    char path[RES_LABEL_LENGTH];
    long offset;
    long length;
} MpkgResLabel;

void SortFiles(char **files, size_t file_count)
{
    if(file_count <= 1) return;
    size_t i = -1, j = file_count;
    size_t pivot = 0;
    while(1)
    {
        do{i++;}while(strcmp(files[i], files[pivot]) < 0);
        do{j--;}while(strcmp(files[j], files[pivot]) > 0);
        if(i >= j) break;

        char *temp = files[i];
        files[i] = files[j];
        files[j] = temp;
    }

    SortFiles(files, j + 1);
    SortFiles(files + j + 1, file_count - j - 1);
}

char** CollectFiles(char **file_pathes, size_t file_count, size_t *all_file_count)
{
    size_t capacity = file_count;
    size_t size = 0;

    char **files = (char**)malloc(sizeof(char*) * capacity);
    for(size_t i = 0; i < file_count; i++)
    {
        const char *path = file_pathes[i];
        struct stat st;
        if(stat(path, &st) == -1)
        {
            continue;
        }

        if(S_ISDIR(st.st_mode))
        {
            DIR *dir = opendir(path);
            if(!dir) continue;
#ifdef DEBUG
            printf("Enter Dir %s\n", path);
#endif

            size_t sub_capacity = 10;
            size_t sub_size = 0;
            char **sub_file_pathes = (char**)malloc(sizeof(char*) * sub_capacity);

            // add sub file or dirs to sub_file_pathes
            struct dirent *dp;
            while((dp = readdir(dir)))
            {
                if(!strcmp(dp->d_name, ".") ||
                   !strcmp(dp->d_name, "..")) continue;

                sub_file_pathes[sub_size] = (char*)malloc(RES_LABEL_LENGTH);
                snprintf(sub_file_pathes[sub_size], RES_LABEL_LENGTH,
                        "%s/%s", path, dp->d_name);

                // printf("    Encountered %s\n", sub_file_pathes[sub_size]);

                sub_size++;
                if(sub_size == sub_capacity)
                {
                    sub_capacity = sub_capacity == 0 ? 1 : sub_capacity * 2;
                    char **new_files = (char**)malloc(sizeof(char*) * sub_capacity);
                    memcpy(new_files, sub_file_pathes, sub_size * sizeof(char*));

                    free(sub_file_pathes);
                    
                    sub_file_pathes = new_files;
                }
            }

            // recursivly collect all files under the dir
            size_t sub_file_count;
            char **sub_files = CollectFiles(sub_file_pathes, sub_size, &sub_file_count);
            for(int n = 0;n < sub_size;n++)
            {
                free(sub_file_pathes[n]);
            }
            free(sub_file_pathes);

            // add all sub files to files
            for(size_t n = 0;n < sub_file_count;n++)
            {
                // printf("    Up passing %s\n", sub_files[n]);
                files[size] = (char*)malloc(RES_LABEL_LENGTH);
                strcpy(files[size], sub_files[n]);
                size ++;
                if(size == capacity)// if full, double capacity
                {
                    capacity = capacity == 0 ? 1 : capacity * 2;
                    char **new_files = (char**)malloc(sizeof(char*) * capacity);
                    memcpy(new_files, files, size * sizeof(char*));
    
                    free(files);
    
                    files = new_files;
                }
            }

            for(int n = 0;n < sub_file_count;n++)
            {
                free(sub_files[n]);
            }
            free(sub_files);

#ifdef DEBUG
            printf("Exit dir %s\n", path);
#endif
        }
        else if(S_ISREG(st.st_mode))
        {
            // add the path to files
            files[size] = (char*)malloc(RES_LABEL_LENGTH);
            strcpy(files[size], path);
#ifdef DEBUG
            printf("Collected %s\n", path);
#endif
            size ++;
            if(size == capacity)// if full, double capacity
            {
                capacity = capacity == 0 ? 1 : capacity * 2;
                char **new_files = (char**)malloc(sizeof(char*) * capacity);
                memcpy(new_files, files, size * sizeof(char*));

                free(files);

                files = new_files;
            }
        }
        else continue;
    }

    *all_file_count = size;
    return files;
}

// returns successfully packed file number
int MakeMpkg(const char *pkg_path, char **file_pathes, size_t file_count)
{
    FILE *pkg_file = fopen(pkg_path, "wb");
    if(!pkg_file)
    {
        return 0;
    }
    
    MpkgHeader header = (MpkgHeader){
        .magic = MPKGMAGIC,
        .item_count = file_count
    };

    MpkgResLabel *reslabels = (MpkgResLabel*)malloc(file_count * sizeof(MpkgResLabel));
    for(size_t i = 0; i < file_count; i++)
    {
        memset(reslabels[i].path, 0, RES_LABEL_LENGTH);
        strcpy(reslabels[i].path, file_pathes[i]);
    }

    fwrite(&header, sizeof(MpkgHeader), 1, pkg_file);
    // now the labels does not contain offset and size info
    fwrite(reslabels, sizeof(MpkgResLabel), file_count, pkg_file);
    
    size_t success_count = 0;
    long offset = sizeof(MpkgHeader) + file_count * sizeof(MpkgResLabel);
    for (size_t i = 0; i < file_count; i++)
    {
        FILE *file = fopen(file_pathes[i], "rb");
        if(!file)
        {
            printf("Failed to find file %s, jump!\n", file_pathes[i]);
            continue;
        }

        printf("Packing file %s...", file_pathes[i]);

        // seek file size
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);

        // save offset and size to labels
        reslabels[i].offset = offset;
        reslabels[i].length = size;
        offset += size;

        // read data
        char *data = (char*)malloc(size);
        size_t read_result = fread(data, size, 1, file);
        if(read_result != 1)
        {
            printf("\n    Failed to read file %s, expected %d, got %d\n",
                   file_pathes[i], 1, read_result);
            fclose(file);
            continue;
        }

        // write data to pkg
        size_t write_result = fwrite(data, size, 1, pkg_file);
        if(write_result != 1)
        {
            printf("\n    Failed to write file %s, expected %d, got %d\n",
                   file_pathes[i], 1, write_result);
            fclose(file);
            continue;
        }

        printf(", Success!\n");

        success_count++;

        fclose(file);
    }

    // rewrite labels with offset and size info
    fseek(pkg_file, sizeof(MpkgHeader), SEEK_SET);
    fwrite(reslabels, sizeof(MpkgResLabel), file_count, pkg_file);

    fclose(pkg_file);

    printf("Provided %d, Success %d.\n", file_count, success_count);

    return success_count;
}

int main(int argn, char **argv)
{
    if(argn < 2)
    {
        printf("Usage: %s [output.pak] [file1] [file2] ...\n", argv[0]);
        return 1;
    }

    size_t file_count;
    char **files = CollectFiles(argv + 2, argn - 2, &file_count);

#ifdef DEBUG
    printf("Collected %d files:\n", file_count);
    for(size_t i = 0; i < file_count; i++)
    {
        printf("    %s\n", files[i]);
    }
    printf("\n");
#endif

    SortFiles(files, file_count);

#ifdef DEBUG
    printf("Sorted %d files:\n", file_count);
    for(size_t i = 0; i < file_count; i++)
    {
        printf("    %s\n", files[i]);
    }
    printf("\n");
#endif

    int success_count = MakeMpkg(argv[1], files, file_count);
    return 0;
}
