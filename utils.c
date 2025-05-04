#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void printProjectInfo(void)
{
    printf("--------------------------------------------------------\n");
    printf("----------------------- CTRL + Z -----------------------\n");
    printf("---------------- Version Control System ----------------\n");
    printf("--------------------------------------------------------\n\n");
}

void exitProgram(void)
{
    printf("Exiting the program...\n");
    exit(0);
}

void copyAll(const char *src, const char *dst, unsigned char key)
{
    struct stat st;
    if (stat(src, &st) != 0)
        return;

    if (S_ISDIR(st.st_mode))
    {
        DIR *dir = opendir(src);
        if (!dir)
            return;
        mkdir(dst, 0777);

        struct dirent *entry;
        while ((entry = readdir(dir)))
        {
            if (!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, "..") ||
                !strcmp(entry->d_name, "Stats.txt"))
                continue;

            char srcFile[PATH_MAX], dstFile[PATH_MAX];
            snprintf(srcFile, PATH_MAX, "%s/%s", src, entry->d_name);
            snprintf(dstFile, PATH_MAX, "%s/%s", dst, entry->d_name);

            struct stat entry_st;
            if (stat(srcFile, &entry_st) == 0 && S_ISREG(entry_st.st_mode))
            {
                FILE *fsrc = fopen(srcFile, "rb");
                FILE *fdst = fopen(dstFile, "wb");
                if (!fsrc || !fdst)
                {
                    if (fsrc)
                        fclose(fsrc);
                    if (fdst)
                        fclose(fdst);
                    printf("Failed to copy file: %s\n", srcFile);
                    continue;
                }
                unsigned char buf[8192];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0)
                {
                    // XOR–encrypt before writing
                    for (size_t i = 0; i < n; i++)
                    {
                        buf[i] ^= key;
                    }
                    fwrite(buf, 1, n, fdst);
                }
                fclose(fsrc);
                fclose(fdst);
            }
        }
        closedir(dir);
    }
}