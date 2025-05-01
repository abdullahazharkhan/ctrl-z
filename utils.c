#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

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
            char srcFile[PATH_MAX], dstFile[PATH_MAX];
            snprintf(srcFile, PATH_MAX, "%s/%s", src, entry->d_name);
            snprintf(dstFile, PATH_MAX, "%s/%s", dst, entry->d_name);
            copyAll(srcFile, dstFile);
        }
        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        FILE *fsrc = fopen(src, "rb");
        FILE *fdst = fopen(dst, "wb");
        if (!fsrc || !fdst) {
            if (fsrc) fclose(fsrc);
            if (fdst) fclose(fdst);
            printf("Failed to copy file: %s\n", src);
            return;
        }
        char buf[8192];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
            fwrite(buf, 1, n, fdst);
        }
        fclose(fsrc);
        fclose(fdst);
    }
}