#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "utils.c"
#include "auth.c"
#include "stats.c"

void publishRepository(void)
{
    char repoName[100], privacy[10];

    // 1. Ask for repo name
    printf("Enter repository name: ");
    scanf(" %99s", repoName);

    // 2. Ask for privacy (public/private)
    while (1)
    {
        printf("Enter privacy (Public/Private): ");
        scanf(" %9s", privacy);
        if (strcasecmp(privacy, "Public") == 0 || strcasecmp(privacy, "Private") == 0)
        {
            // Normalize to capitalized
            if (strcasecmp(privacy, "public") == 0)
                strcpy(privacy, "Public");
            else
                strcpy(privacy, "Private");
            break;
        }
        printf("Invalid input. Please enter 'Public' or 'Private'.\n");
    }

    // 3. Get base path using getUserDataPath
    char basePath[PATH_MAX];
    getUserDataPath(basePath); // gets ~/CtrlZ/UserData.txt

    // Replace filename with repo directory
    char *lastSlash = strrchr(basePath, '/');
    if (lastSlash)
        *lastSlash = '\0'; // remove "/UserData.txt"

    // Build repo directory path
    char repoPath[PATH_MAX];
    snprintf(repoPath, sizeof(repoPath), "%s/%s", basePath, repoName);

    // Check if repo already exists
    if (access(repoPath, F_OK) == 0)
    {
        printf("Error: Repository '%s' already exists.\n", repoName);
        return;
    }

    // 4. Create the repo folder
    if (mkdir(repoPath, 0755) != 0)
    {
        perror("mkdir");
        return;
    }

    // 5. Create Config.txt file
    char configPath[PATH_MAX];
    snprintf(configPath, sizeof(configPath), "%s/Config.txt", repoPath);

    FILE *cfg = fopen(configPath, "w");
    if (!cfg)
    {
        perror("fopen config");
        return;
    }

    char *author = getLoggedInUser();
    if (!author)
        author = "Unknown";

    fprintf(cfg, "Author:%s\n", author);
    fprintf(cfg, "Collaborators:\n");
    fprintf(cfg, "Privacy:%s\n", privacy);
    fclose(cfg);

    if (author && strcmp(author, "Unknown") != 0)
        free(author);

    // 6. Create empty History.txt file
    char historyPath[PATH_MAX];
    snprintf(historyPath, sizeof(historyPath), "%s/History.txt", repoPath);

    FILE *hist = fopen(historyPath, "w");
    if (!hist)
    {
        perror("fopen history");
        return;
    }
    fclose(hist);

    printf("Repository '%s' created successfully with privacy '%s'.\n", repoName, privacy);
}

void pushFiles(void)
{
    char repo[100];
    printf("Enter repository name: ");
    scanf(" %99s", repo);

    const char *home = getenv("HOME");
    if (!home)
    {
        fprintf(stderr, "ERROR: HOME not set\n");
        return;
    }

    char repoPath[PATH_MAX];
    snprintf(repoPath, PATH_MAX, "%s/CtrlZ/%s", home, repo);

    struct stat st;
    if (stat(repoPath, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        printf("Repository '%s' does not exist in %s/CtrlZ.\n", repo, home);
        return;
    }

    // Open Config.txt
    char configPath[PATH_MAX];
    snprintf(configPath, PATH_MAX, "%s/Config.txt", repoPath);
    FILE *cfg = fopen(configPath, "r");
    if (!cfg)
    {
        perror("Could not open Config.txt");
        return;
    }

    char author[100] = "", collaborators[1024] = "", privacy[32] = "";
    char line[1024];
    while (fgets(line, sizeof(line), cfg))
    {
        if (strncmp(line, "Author:", 7) == 0)
        {
            char *p = line + 7;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(author, p, sizeof(author) - 1);
        }
        else if (strncmp(line, "Collaborators:", 14) == 0)
        {
            char *p = line + 14;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(collaborators, p, sizeof(collaborators) - 1);
        }
        else if (strncmp(line, "Privacy:", 8) == 0)
        {
            char *p = line + 8;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(privacy, p, sizeof(privacy) - 1);
        }
    }
    fclose(cfg);

    // Get logged in user
    char *currentUser = getLoggedInUser();
    if (!currentUser)
    {
        printf("No user is currently logged in.\n");
        return;
    }

    // Check if user is author or collaborator
    int allowed = 0;
    if (strcmp(currentUser, author) == 0)
    {
        allowed = 1;
    }
    else if (strlen(collaborators) > 0)
    {
        char search[110];
        snprintf(search, sizeof(search), "%s,", currentUser);
        if (strstr(collaborators, search) != NULL)
        {
            allowed = 1;
        }
    }

    if (!allowed)
    {
        printf("You are not authorized to push to this repository.\n");
        free(currentUser);
        return;
    }

    printf("Repository '%s' found in %s/CtrlZ.\n", repo, home);

    char folderPath[PATH_MAX];
    printf("Enter the folder path to push files from: ");
    getchar(); // clear newline left by previous scanf
    if (fgets(folderPath, sizeof(folderPath), stdin) == NULL)
    {
        printf("Invalid folder path input.\n");
        free(currentUser);
        return;
    }
    folderPath[strcspn(folderPath, "\r\n")] = '\0'; // Remove newline

    // Resolve relative paths to absolute
    char absFolderPath[PATH_MAX];
    if (realpath(folderPath, absFolderPath) == NULL)
    {
        perror("Invalid folder path");
        free(currentUser);
        return;
    }

    printf("Resolved folder path: %s\n", absFolderPath);

    // Find the highest Version_N in the repo directory
    int maxVersion = 0;
    DIR *d = opendir(repoPath);
    if (!d)
    {
        perror("opendir repoPath");
        free(currentUser);
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)))
    {
        if (strncmp(ent->d_name, "Version_", 8) == 0)
        {
            int vnum = atoi(ent->d_name + 8);
            if (vnum > maxVersion)
                maxVersion = vnum;
        }
    }
    closedir(d);

    // Create new version directory
    int newVersion = maxVersion + 1;
    char newVersionPath[PATH_MAX];
    snprintf(newVersionPath, PATH_MAX, "%s/Version_%d", repoPath, newVersion);

    if (mkdir(newVersionPath, 0777) != 0)
    {
        perror("mkdir new version");
        free(currentUser);
        return;
    }

    copyAll(absFolderPath, newVersionPath);

    printf("Pushed files to %s\n", newVersionPath);

    // --- Commit message input and append to History.txt ---
    char message[1024];
    printf("Enter commit message: ");
    // Only flush stdin if previous input left a newline
    fflush(stdin);

    if (fgets(message, sizeof(message), stdin) == NULL)
    {
        strcpy(message, "No message provided.");
    }
    message[strcspn(message, "\r\n")] = '\0';

    // Append to History.txt in repo folder
    char historyPath[PATH_MAX];
    snprintf(historyPath, PATH_MAX, "%s/History.txt", repoPath);
    FILE *hf = fopen(historyPath, "a");
    if (hf)
    {
        char *msg_ptr = message;
        while (*msg_ptr == ' ' || *msg_ptr == '\t')
            msg_ptr++;
        fprintf(hf, "Version_%d by %s: %s\n", newVersion, currentUser, msg_ptr);
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char datetime[64];
        strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(hf, "%s\n", datetime);
        fclose(hf);
    }
    else
    {
        printf("Warning: Could not write to History.txt\n");
    }

    free(currentUser);

    // --- Stats generation and display ---
    char prevVersionPath[PATH_MAX] = "";
    if (newVersion > 1)
    {
        snprintf(prevVersionPath, PATH_MAX, "%s/Version_%d", repoPath, newVersion - 1);
    }
    char statsPath[PATH_MAX];
    snprintf(statsPath, PATH_MAX, "%s/Stats.txt", newVersionPath);

    // Save current stdout file descriptor
    int saved_stdout = dup(fileno(stdout));
    FILE *statsFile = fopen(statsPath, "w");
    if (statsFile)
    {
        fflush(stdout);
        dup2(fileno(statsFile), fileno(stdout));
        if (newVersion > 1)
        {
            getStats(prevVersionPath, newVersionPath);
        }
        else
        {
            printf("No previous version to compare for stats.\n");
        }
        fflush(stdout);
        fclose(statsFile);
        dup2(saved_stdout, fileno(stdout));
        close(saved_stdout);
    }
    else
    {
        printf("Could not create Stats.txt for version %d\n", newVersion);
    }

    // Ask user if they want to display stats
    char showStats[10];
    printf("Do you want to display stats for this push? (yes/no): ");
    if (fgets(showStats, sizeof(showStats), stdin) == NULL)
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        showStats[0] = '\0';
    }
    showStats[strcspn(showStats, "\r\n")] = '\0';
    if (strcasecmp(showStats, "yes") == 0 || strcasecmp(showStats, "y") == 0)
    {
        FILE *statsFile = fopen(statsPath, "r");
        if (statsFile)
        {
            char buf[512];
            printf("\n========== Stats for Version_%d ==========\n", newVersion);
            while (fgets(buf, sizeof(buf), statsFile))
            {
                fputs(buf, stdout);
            }
            printf("==========================================\n");
            fclose(statsFile);
        }
        else
        {
            printf("Stats.txt not found for this version.\n");
        }
    }
}

void pullFiles(void)
{
    char repo[100];
    printf("Enter repository name: ");
    scanf(" %99s", repo);

    const char *home = getenv("HOME");
    if (!home)
    {
        fprintf(stderr, "ERROR: HOME not set\n");
        return;
    }

    char repoPath[PATH_MAX];
    snprintf(repoPath, PATH_MAX, "%s/CtrlZ/%s", home, repo);

    struct stat st;
    if (stat(repoPath, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        printf("Repository '%s' does not exist in %s/CtrlZ.\n", repo, home);
        return;
    }

    // Open Config.txt
    char configPath[PATH_MAX];
    snprintf(configPath, PATH_MAX, "%s/Config.txt", repoPath);
    FILE *cfg = fopen(configPath, "r");
    if (!cfg)
    {
        perror("Could not open Config.txt");
        return;
    }

    char author[100] = "", collaborators[1024] = "", privacy[32] = "";
    char line[1024];
    while (fgets(line, sizeof(line), cfg))
    {
        if (strncmp(line, "Author:", 7) == 0)
        {
            char *p = line + 7;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(author, p, sizeof(author) - 1);
        }
        else if (strncmp(line, "Collaborators:", 14) == 0)
        {
            char *p = line + 14;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(collaborators, p, sizeof(collaborators) - 1);
        }
        else if (strncmp(line, "Privacy:", 8) == 0)
        {
            char *p = line + 8;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(privacy, p, sizeof(privacy) - 1);
        }
    }
    fclose(cfg);

    // Get logged in user
    char *currentUser = getLoggedInUser();
    if (!currentUser)
    {
        printf("No user is currently logged in.\n");
        return;
    }

    // Check if user is author, collaborator, or public
    int allowed = 0;
    if (strcmp(currentUser, author) == 0)
    {
        allowed = 1;
    }
    else if (strlen(collaborators) > 0)
    {
        char search[110];
        snprintf(search, sizeof(search), "%s,", currentUser);
        if (strstr(collaborators, search) != NULL)
        {
            allowed = 1;
        }
    }
    if (!allowed && (strcasecmp(privacy, "public") == 0))
    {
        allowed = 1;
    }

    if (!allowed)
    {
        printf("You are not authorized to pull from this repository.\n");
        free(currentUser);
        return;
    }

    // Display available versions with messages
    char historyPath[PATH_MAX];
    snprintf(historyPath, PATH_MAX, "%s/History.txt", repoPath);
    FILE *hf = fopen(historyPath, "r");
    if (!hf)
    {
        printf("No history found for this repository.\n");
        free(currentUser);
        return;
    }

    // Instead of reading History.txt, scan for Version_* directories
    DIR *dir = opendir(repoPath);
    if (!dir) {
        perror("opendir repoPath");
        free(currentUser);
        return;
    }

    typedef struct { int version; char name[100]; } VersionInfo;
    VersionInfo versions[100];
    int vcount = 0;
    struct dirent *entry;

    while ((entry = readdir(dir))) {
        // look for names starting with "Version_"
        if (strncmp(entry->d_name, "Version_", 8) == 0) {
            int vnum;
            if (sscanf(entry->d_name + 8, "%d", &vnum) == 1) {
                versions[vcount].version = vnum;
                strncpy(versions[vcount].name, entry->d_name, 100-1);
                versions[vcount].name[100-1] = '\0';
                vcount++;
            }
        }
    }
    closedir(dir);

    if (vcount == 0) {
        printf("No versions found in repository folder.\n");
        free(currentUser);
        return;
    }

    printf("Available versions:\n");
    for (int i = 0; i < vcount; ++i) {
        printf("%d. %s\n", i + 1, versions[i].name);
    }

    int choice = 0;
    printf("Enter the number of the version to pull: ");
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > vcount) {
        printf("Invalid version choice.\n");
        free(currentUser);
        return;
    }
    // now you have versions[choice-1].version
    int selectedVersion = versions[choice - 1].version;

    // Ask for destination folder
    char destPath[PATH_MAX];
    printf("Enter the destination folder path: ");
    getchar(); // clear newline left by previous scanf
    if (fgets(destPath, sizeof(destPath), stdin) == NULL)
    {
        printf("Invalid destination path input.\n");
        free(currentUser);
        return;
    }
    destPath[strcspn(destPath, "\r\n")] = '\0'; // Remove newline

    // Resolve destination path
    char absDestPath[PATH_MAX];
    if (realpath(destPath, absDestPath) == NULL)
    {
        perror("Invalid destination path");
        free(currentUser);
        return;
    }

    // Source version path
    char srcVersionPath[PATH_MAX];
    snprintf(srcVersionPath, PATH_MAX, "%s/Version_%d", repoPath, selectedVersion);

    struct stat vst;
    if (stat(srcVersionPath, &vst) != 0 || !S_ISDIR(vst.st_mode))
    {
        printf("Selected version directory does not exist.\n");
        free(currentUser);
        return;
    }

    // Only copy regular files from srcVersionPath to absDestPath
    DIR *srcDir = opendir(srcVersionPath);
    if (!srcDir)
    {
        perror("opendir version folder");
        free(currentUser);
        return;
    }
    entry = NULL;
    struct stat entry_st;
    char srcFile[PATH_MAX], dstFile[PATH_MAX];
    while ((entry = readdir(srcDir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(srcFile, sizeof(srcFile), "%s/%s", srcVersionPath, entry->d_name);
        if (stat(srcFile, &entry_st) == 0 && S_ISREG(entry_st.st_mode))
        {
            snprintf(dstFile, sizeof(dstFile), "%s/%s", absDestPath, entry->d_name);
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
            char buf[8192];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0)
            {
                fwrite(buf, 1, n, fdst);
            }
            fclose(fsrc);
            fclose(fdst);
        }
    }
    closedir(srcDir);

    printf("Pulled Version_%d to %s\n", selectedVersion, absDestPath);

    free(currentUser);
}
