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


void copyAll(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) != 0) return;
    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(src);
        if (!dir) return;
        mkdir(dst, 0777);
        struct dirent *entry;
        while ((entry = readdir(dir))) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            // Skip Stats.txt
            if (strcmp(entry->d_name, "Stats.txt") == 0)
                continue;
            char srcFile[PATH_MAX], dstFile[PATH_MAX];
            snprintf(srcFile, PATH_MAX, "%s/%s", src, entry->d_name);
            snprintf(dstFile, PATH_MAX, "%s/%s", dst, entry->d_name);

            // Only copy regular files, skip directories and others
            struct stat entry_st;
            if (stat(srcFile, &entry_st) == 0 && S_ISREG(entry_st.st_mode)) {
                FILE *fsrc = fopen(srcFile, "rb");
                FILE *fdst = fopen(dstFile, "wb");
                if (!fsrc || !fdst) {
                    if (fsrc) fclose(fsrc);
                    if (fdst) fclose(fdst);
                    printf("Failed to copy file: %s\n", srcFile);
                    continue;
                }
                char buf[8192];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
                    fwrite(buf, 1, n, fdst);
                }
                fclose(fsrc);
                fclose(fdst);
            }
            // else: skip subdirectories and non-regular files
        }
        closedir(dir);
    }
    // Do not copy if src is a file (only copy files from a directory, not a single file call)
}