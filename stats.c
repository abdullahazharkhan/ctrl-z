#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#define PATH_MAX 4096
#define MAX_FILES 1024
#define MAX_FILENAME 256
#define MAX_FILE_SIZE 1024

// Function prototype
void getStats(char *oldRepo, char* newRepo); // Compare two directories called from main.c
                                             // path like /home/user/CtrlZ/Repo1 ...
void compareFiles(const char *file1, size_t size1, const char *file2, size_t size2, const char *filename);

// Thread argument for reading file data
typedef struct {
    char path[PATH_MAX];
    char buffer[MAX_FILE_SIZE];
    size_t size;
    int result; // 0 = success, -1 = fail
} FileReadArgs;

void *readFileData(void *arg) {
    FileReadArgs *args = (FileReadArgs *)arg;
    FILE *fp = fopen(args->path, "rb");
    if (!fp) {
        args->result = -1;
        return NULL;
    }
    args->size = fread(args->buffer, 1, MAX_FILE_SIZE, fp);
    fclose(fp);
    args->result = 0;
    return NULL;
}

void compareFiles(const char *buf1, size_t size1, const char *buf2, size_t size2, const char *filename) {
    // Split buffers into lines
    char *linesA[MAX_FILE_SIZE], *linesB[MAX_FILE_SIZE];
    int countA = 0, countB = 0;

    char *tmpA = malloc(size1 + 1);
    char *tmpB = malloc(size2 + 1);
    if (!tmpA || !tmpB) {
        printf("Memory allocation failed for diff.\n");
        free(tmpA); free(tmpB);
        return;
    }
    memcpy(tmpA, buf1, size1); tmpA[size1] = '\0';
    memcpy(tmpB, buf2, size2); tmpB[size2] = '\0';

    char *saveptrA, *saveptrB;
    char *line = strtok_r(tmpA, "\n", &saveptrA);
    while (line && countA < MAX_FILE_SIZE) {
        linesA[countA++] = line;
        line = strtok_r(NULL, "\n", &saveptrA);
    }
    line = strtok_r(tmpB, "\n", &saveptrB);
    while (line && countB < MAX_FILE_SIZE) {
        linesB[countB++] = line;
        line = strtok_r(NULL, "\n", &saveptrB);
    }

    // Simple Myers Diff
    int N = countA, M = countB;
    int max = N + M;
    int *V = calloc(2 * max + 1, sizeof(int));
    int *trace[max + 1];
    for (int d = 0; d <= max; d++) {
        trace[d] = malloc(sizeof(int) * (2 * max + 1));
    }

    int d, k, x, y, found = 0;
    for (d = 0; d <= max; d++) {
        for (k = -d; k <= d; k += 2) {
            if (k == -d || (k != d && V[k - 1 + max] < V[k + 1 + max])) {
                x = V[k + 1 + max];
            } else {
                x = V[k - 1 + max] + 1;
            }
            y = x - k;
            while (x < N && y < M && strcmp(linesA[x], linesB[y]) == 0) {
                x++; y++;
            }
            V[k + max] = x;
            trace[d][k + max] = x;
            if (x >= N && y >= M) {
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    // Backtrack to get the diff
    typedef struct { int type, idxA, idxB; } DiffOp;
    DiffOp ops[MAX_FILE_SIZE * 2];
    int opCount = 0;
    int currD = d, currK = N - M, currX = N, currY = M;
    while (currD > 0) {
        int prevK;
        if (currK == -currD || (currK != currD && trace[currD - 1][currK - 1 + max] < trace[currD - 1][currK + 1 + max])) {
            prevK = currK + 1;
        } else {
            prevK = currK - 1;
        }
        int prevX = trace[currD - 1][prevK + max];
        int prevY = prevX - prevK;
        while (currX > prevX && currY > prevY) {
            ops[opCount++] = (DiffOp){0, currX - 1, currY - 1}; // same
            currX--; currY--;
        }
        if (prevX < currX) {
            ops[opCount++] = (DiffOp){1, currX - 1, -1}; // delete
            currX--;
        } else if (prevY < currY) {
            ops[opCount++] = (DiffOp){2, -1, currY - 1}; // insert
            currY--;
        }
        currK = prevK;
        currD--;
    }
    while (currX > 0 && currY > 0) {
        ops[opCount++] = (DiffOp){0, currX - 1, currY - 1};
        currX--; currY--;
    }
    while (currX > 0) {
        ops[opCount++] = (DiffOp){1, currX - 1, -1};
        currX--;
    }
    while (currY > 0) {
        ops[opCount++] = (DiffOp){2, -1, currY - 1};
        currY--;
    }

    // Print diff
    int insertedLines = 0, deletedLines = 0, changedLines = 0;
    int insertedChars = 0, deletedChars = 0;
    printf("\n========== Diff for %s ==========\n", filename);
    for (int i = opCount - 1; i >= 0; i--) {
        if (ops[i].type == 0) {
            printf("    %s\n", linesA[ops[i].idxA]);
        } else if (ops[i].type == 1) {
            printf("  - %s\n", linesA[ops[i].idxA]);
            deletedLines++;
            deletedChars += strlen(linesA[ops[i].idxA]);
        } else if (ops[i].type == 2) {
            printf("  + %s\n", linesB[ops[i].idxB]);
            insertedLines++;
            insertedChars += strlen(linesB[ops[i].idxB]);
        }
    }
    // Count changed lines (delete followed by insert)
    for (int i = opCount - 1; i > 0; i--) {
        if (ops[i].type == 1 && ops[i-1].type == 2) {
            changedLines++;
        }
    }
    printf("----------------------------------\n");
    printf("Summary for %s:\n", filename);
    printf("  Insertions: %d lines, %d characters\n", insertedLines, insertedChars);
    printf("  Deletions:  %d lines, %d characters\n", deletedLines, deletedChars);
    printf("  Changes:    %d lines\n", changedLines);
    printf("==================================\n");

    // Cleanup
    for (int i = 0; i <= max; i++) free(trace[i]);
    free(V);
    free(tmpA);
    free(tmpB);
}

// Path like 
void getStats(char *oldRepo, char* newRepo) {
    struct stat statbuf1, statbuf2;
    if (stat(oldRepo, &statbuf1) != 0 || !S_ISDIR(statbuf1.st_mode)) {
        printf("Directory '%s' does not exist or is not a directory.\n", oldRepo);
        return;
    }
    if (stat(newRepo, &statbuf2) != 0 || !S_ISDIR(statbuf2.st_mode)) {
        printf("Directory '%s' does not exist or is not a directory.\n", newRepo);
        return;
    }

    // Read filenames from both directories
    char oldFiles[MAX_FILES][MAX_FILENAME];
    char newFiles[MAX_FILES][MAX_FILENAME];
    int oldCount = 0, newCount = 0;

    DIR *dir = opendir(oldRepo);
    struct dirent *entry;
    while (dir && (entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            strncpy(oldFiles[oldCount], entry->d_name, MAX_FILENAME-1);
            oldFiles[oldCount][MAX_FILENAME-1] = '\0';
            oldCount++;
        }
    }
    if (dir) closedir(dir);

    dir = opendir(newRepo);
    while (dir && (entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            strncpy(newFiles[newCount], entry->d_name, MAX_FILENAME-1);
            newFiles[newCount][MAX_FILENAME-1] = '\0';
            newCount++;
        }
    }
    if (dir) closedir(dir);

    // --- Print stats ---
    printf("\n========== Repository Stats ==========\n");
    printf("Old Repo: %s\n", oldRepo);
    printf("  Files: %d\n", oldCount);
    printf("New Repo: %s\n", newRepo);
    printf("  Files: %d\n", newCount);

    // --- Find removed, new, and common files ---
    int removedCount = 0, newFilesCount = 0, commonCount = 0;
    int removedIdx[MAX_FILES], newIdx[MAX_FILES], commonOldIdx[MAX_FILES], commonNewIdx[MAX_FILES];

    // Removed and common
    for (int i = 0; i < oldCount; ++i) {
        int found = 0;
        for (int j = 0; j < newCount; ++j) {
            if (strcmp(oldFiles[i], newFiles[j]) == 0) {
                found = 1;
                commonOldIdx[commonCount] = i;
                commonNewIdx[commonCount] = j;
                commonCount++;
                break;
            }
        }
        if (!found) {
            removedIdx[removedCount++] = i;
        }
    }
    // New files
    for (int j = 0; j < newCount; ++j) {
        int found = 0;
        for (int i = 0; i < oldCount; ++i) {
            if (strcmp(newFiles[j], oldFiles[i]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            newIdx[newFilesCount++] = j;
        }
    }

    printf("\n========== File Differences ==========\n");

    // Removed files
    printf("Removed files (present in oldRepo, not in newRepo):\n");
    if (removedCount == 0) {
        printf("  None\n");
    } else {
        for (int i = 0; i < removedCount; ++i) {
            printf("  - %s\n", oldFiles[removedIdx[i]]);
        }
    }

    // New files
    printf("\nNew files (present in newRepo, not in oldRepo):\n");
    if (newFilesCount == 0) {
        printf("  None\n");
    } else {
        for (int i = 0; i < newFilesCount; ++i) {
            printf("  + %s\n", newFiles[newIdx[i]]);
        }
    }

    // --- Common files ---
    printf("\n========== Common Files ==========\n");
    if (commonCount == 0) {
        printf("No common files to compare.\n");
    } else {
        for (int i = 0; i < commonCount; ++i) {
            printf("  %s\n", oldFiles[commonOldIdx[i]]);
        }
    }

    // --- Diff for common files ---
    printf("\n========== Detailed Differences for Common Files ==========\n");
    for (int i = 0; i < commonCount; ++i) {
        FileReadArgs args1, args2;
        snprintf(args1.path, sizeof(args1.path), "%s/%s", oldRepo, oldFiles[commonOldIdx[i]]);
        snprintf(args2.path, sizeof(args2.path), "%s/%s", newRepo, newFiles[commonNewIdx[i]]);
        pthread_t t1, t2;
        pthread_create(&t1, NULL, readFileData, &args1);
        pthread_create(&t2, NULL, readFileData, &args2);
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        if (args1.result == 0 && args2.result == 0) {
            compareFiles(args1.buffer, args1.size, args2.buffer, args2.size, oldFiles[commonOldIdx[i]]);
        } else {
            printf("Error reading file data for %s\n", oldFiles[commonOldIdx[i]]);
        }
    }
    printf("==================================\n");
}