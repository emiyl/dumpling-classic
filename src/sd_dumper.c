#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/dirent.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "fs/fs_utils.h"
#include "main.h"

#define BUFFER_SIZE     0x80000

void console_printf(int newline, const char *format, ...);
int checkCancel(void);

const bool detailed_logs = false;

static int DumpFile(char *pPath, const char * output_path)
{
    char *pSlash = strchr(pPath, '/');
    if(pSlash && pSlash[1] == '/')
        pSlash++;

    char *pFilename = strrchr(pPath, '/');
    if(!pFilename)
        pFilename = pPath;
    else
        pFilename++;

    console_printf(1, "0x%X - %s", 0, pSlash);

    unsigned char* dataBuf = (unsigned char*)memalign(0x40, BUFFER_SIZE);
    if(!dataBuf) {
        return -1;
    }

    FILE *pReadFile = fopen(pPath, "rb");
    if(!pReadFile)
    {
        if (detailed_logs) console_printf(1, "Can't open file %s\n", pPath);
        if (!detailed_logs) console_printf(1, "Symlink file, skipping...\n", pPath);
        return -2;
    }

    FILE *pWriteFile = fopen(output_path, "wb");
    if(!pWriteFile)
    {
        if (detailed_logs) console_printf(1, "Can't open write file %s\n", output_path);
        fclose(pReadFile);
        return -3;
    }

    unsigned int size = 0;
    unsigned int ret;
    u32 passedMs = 1;
    u64 startTime = OSGetTime();

    // Copy rpl in memory
    while ((ret = fread(dataBuf, 0x1, BUFFER_SIZE, pReadFile)) > 0)
    {
        passedMs = (OSGetTime() - startTime) * 4000ULL / BUS_SPEED;
        if(passedMs == 0)
            passedMs = 1; // avoid 0 div

        fwrite(dataBuf, 0x01, ret, pWriteFile);
        size += ret;
        console_printf(0, " %s - 0x%X (%i kB/s) (%s)\r", pFilename, size, (u32)(((u64)size * 1000) / ((u64)1024 * passedMs)), pSlash);
    }

    fclose(pWriteFile);
    fclose(pReadFile);
    free(dataBuf);
    return 0;
}

int DumpDir(char *pPath, const char * target_path)
{
    struct dirent *dirent = NULL;
    DIR *dir = NULL;

    dir = opendir(pPath);
    if (dir == NULL)
    {
        if (detailed_logs) console_printf(1, "Can't open %s\n", pPath);
        return -1;
    }

    {
        char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
        if(!targetPath)
        {
            console_printf(1, "no memory\n");
            closedir(dir);
            return -1;
        }

        char *pSlash = strchr(pPath, '/');
        snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s%s", target_path, pSlash);
        CreateSubfolder(targetPath);
        free(targetPath);
    }

    while ((dirent = readdir(dir)) != 0)
    {
        if(strcmp(dirent->d_name, "..") == 0 || strcmp(dirent->d_name, ".") == 0)
            continue;

        if(CheckCancel())
            break;

        int len = strlen(pPath);
        snprintf(pPath + len, FS_MAX_FULLPATH_SIZE - len, "/%s", dirent->d_name);

        char *pSlash = strchr(pPath, '/');

        if(!pSlash)
        {
            console_printf(1, "Error on %s\n", pPath);
            continue;
        }
        if(pSlash[1] == '/')
            pSlash++;

        if(dirent->d_type & DT_DIR)
        {
            console_printf(1, "%s (%s)\n", dirent->d_name, pSlash);

            DumpDir(pPath, target_path);
        }
        else
        {
            char *targetPath = (char*)malloc(FS_MAX_FULLPATH_SIZE);
            snprintf(targetPath, FS_MAX_FULLPATH_SIZE, "%s%s", target_path, pSlash);

            DumpFile(pPath, targetPath);
            free(targetPath);
        }
        pPath[len] = 0;
    }

    closedir(dir);
    return 0;
}
