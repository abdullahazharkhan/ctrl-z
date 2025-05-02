#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "auth.c"
#include "stats.c"
#include "utils.c"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// mutex for browseRepositories()
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void browseRepositories(void);
void publishRepository(void);
void pushFiles(void);
void pullFiles(void);
void addCollaborator(void);
void removeCollaborator(void);
void exitProgram(void);
void printProjectInfo(void);

// Utils
int initProject(int *);

int main(int argc, char *argv[])
{
    printProjectInfo();

    int isSomeoneLoggedIn = 0;
    if (initProject(&isSomeoneLoggedIn) != 0)
    {
        fprintf(stderr, "Failed to initialize project\n");
        return 1;
    }

    while (1)
    {
        printf("--------------------------------------------------------\n");
        if (!isSomeoneLoggedIn)
        {
            printf("1. Browse public repositories\n");
            printf("2. Register as a new user\n");
            printf("3. Login\n");
            printf("4. Exit\n");
            printf("--------------------------------------------------------\n");

            int choice;
            printf("Enter your choice: ");
            if (scanf("%d", &choice) != 1)
            {
                printf("Invalid input. Please enter a number.\n");
                while (getchar() != '\n');
                continue;
            }

            switch (choice)
            {
            case 1:
                browseRepositories();
                break;
            case 2:
                registerUser();
                break;
            case 3:
                login(&isSomeoneLoggedIn);
                break;
            case 4:
                printf("Exiting the program...\n");
                exit(0);
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
            }
        }
        else
        {
            printf("1. Browse your repositories\n");
            printf("2. Publish your own repository\n");
            printf("3. Push your files\n");
            printf("4. Pull the files\n");
            printf("5. Add collaborator to your project\n");
            printf("6. Remove collaborator from your project\n");
            printf("7. Logout\n");
            printf("8. Exit\n");
            printf("--------------------------------------------------------\n");

            int choice;
            printf("Enter your choice: ");
            if (scanf("%d", &choice) != 1)
            {
                printf("Invalid input. Please enter a number.\n");
                while (getchar() != '\n');
                continue;
            }

            switch (choice)
            {
            case 1:
                browseRepositories();
                break;
            case 2:
                publishRepository();
                break;
            case 3:
                pushFiles();
                break;
            case 4:
                pullFiles();
                break;
            case 5:
                addCollaborator();
                break;
            case 6:
                removeCollaborator();
                break;
            case 7:
                logout(&isSomeoneLoggedIn);
                break;
            case 8:
                exitProgram();
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
            }
        }
    }

    return 0;
}

void publishRepository(void) {
    char repoName[100], privacy[10];
    
    // 1. Ask for repo name
    printf("Enter repository name: ");
    scanf(" %99s", repoName);

    // 2. Ask for privacy (public/private)
    while (1) {
        printf("Enter privacy (Public/Private): ");
        scanf(" %9s", privacy);
        if (strcasecmp(privacy, "Public") == 0 || strcasecmp(privacy, "Private") == 0) {
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
    if (lastSlash) *lastSlash = '\0';  // remove "/UserData.txt"

    // Build repo directory path
    char repoPath[PATH_MAX];
    snprintf(repoPath, sizeof(repoPath), "%s/%s", basePath, repoName);

    // Check if repo already exists
    if (access(repoPath, F_OK) == 0) {
        printf("Error: Repository '%s' already exists.\n", repoName);
        return;
    }

    // 4. Create the repo folder
    if (mkdir(repoPath, 0755) != 0) {
        perror("mkdir");
        return;
    }

    // 5. Create Config.txt file
    char configPath[PATH_MAX];
    snprintf(configPath, sizeof(configPath), "%s/Config.txt", repoPath);

    FILE *cfg = fopen(configPath, "w");
    if (!cfg) {
        perror("fopen config");
        return;
    }


    char *author = getLoggedInUser();
    if (!author) author = "Unknown";

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
    if (!hist) {
        perror("fopen history");
        return;
    }
    fclose(hist);

    printf("Repository '%s' created successfully with privacy '%s'.\n", repoName, privacy);
}
void pushFiles(void) {
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
    if (stat(repoPath, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("Repository '%s' does not exist in %s/CtrlZ.\n", repo, home);
        return;
    }

    // Open Config.txt
    char configPath[PATH_MAX];
    snprintf(configPath, PATH_MAX, "%s/Config.txt", repoPath);
    FILE *cfg = fopen(configPath, "r");
    if (!cfg) {
        perror("Could not open Config.txt");
        return;
    }

    char author[100] = "", collaborators[1024] = "", privacy[32] = "";
    char line[1024];
    while (fgets(line, sizeof(line), cfg)) {
        if (strncmp(line, "Author:", 7) == 0) {
            char *p = line + 7;
            while (*p == ' ' || *p == '\t') p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(author, p, sizeof(author) - 1);
        } else if (strncmp(line, "Collaborators:", 14) == 0) {
            char *p = line + 14;
            while (*p == ' ' || *p == '\t') p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(collaborators, p, sizeof(collaborators) - 1);
        } else if (strncmp(line, "Privacy:", 8) == 0) {
            char *p = line + 8;
            while (*p == ' ' || *p == '\t') p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(privacy, p, sizeof(privacy) - 1);
        }
    }
    fclose(cfg);

    // Get logged in user
    char *currentUser = getLoggedInUser();
    if (!currentUser) {
        printf("No user is currently logged in.\n");
        return;
    }

    // Check if user is author or collaborator
    int allowed = 0;
    if (strcmp(currentUser, author) == 0) {
        allowed = 1;
    } else if (strlen(collaborators) > 0) {
        char search[110];
        snprintf(search, sizeof(search), "%s,", currentUser);
        if (strstr(collaborators, search) != NULL) {
            allowed = 1;
        }
    }

    if (!allowed) {
        printf("You are not authorized to push to this repository.\n");
        free(currentUser);
        return;
    }

    printf("Repository '%s' found in %s/CtrlZ.\n", repo, home);

    char folderPath[PATH_MAX];
    printf("Enter the folder path to push files from: ");
    getchar(); // clear newline left by previous scanf
    if (fgets(folderPath, sizeof(folderPath), stdin) == NULL) {
        printf("Invalid folder path input.\n");
        free(currentUser);
        return;
    }
    folderPath[strcspn(folderPath, "\r\n")] = '\0'; // Remove newline

    // Resolve relative paths to absolute
    char absFolderPath[PATH_MAX];
    if (realpath(folderPath, absFolderPath) == NULL) {
        perror("Invalid folder path");
        free(currentUser);
        return;
    }

    printf("Resolved folder path: %s\n", absFolderPath);

    // Find the highest Version_N in the repo directory
    int maxVersion = 0;
    DIR *d = opendir(repoPath);
    if (!d) {
        perror("opendir repoPath");
        free(currentUser);
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d))) {
        if (strncmp(ent->d_name, "Version_", 8) == 0) {
            int vnum = atoi(ent->d_name + 8);
            if (vnum > maxVersion) maxVersion = vnum;
        }
    }
    closedir(d);

    // Create new version directory
    int newVersion = maxVersion + 1;
    char newVersionPath[PATH_MAX];
    snprintf(newVersionPath, PATH_MAX, "%s/Version_%d", repoPath, newVersion);

    if (mkdir(newVersionPath, 0777) != 0) {
        perror("mkdir new version");
        free(currentUser);
        return;
    }

    copyAll(absFolderPath, newVersionPath);

    printf("Pushed files to %s\n", newVersionPath);

    // --- New code for commit message and history tracking ---
    // 1. Take commit message and save to pushFiles.txt
    char message[1024];
    printf("Enter commit message: ");
    getchar(); // clear newline left by previous scanf
    if (fgets(message, sizeof(message), stdin) == NULL) {
        strcpy(message, "No message provided.");
    }
    message[strcspn(message, "\r\n")] = '\0'; // Remove newline

    char pushFilePath[PATH_MAX];
    snprintf(pushFilePath, PATH_MAX, "%s/pushFiles.txt", repoPath);
    FILE *pf = fopen(pushFilePath, "w");
    if (pf) {
        fprintf(pf, "%s\n", message);
        fclose(pf);
    }

    // 2. Append to History.txt in repo folder
    char historyPath[PATH_MAX];
    snprintf(historyPath, PATH_MAX, "%s/History.txt", repoPath);
    FILE *hf = fopen(historyPath, "a");
    if (!hf) {
        // Try to create if not exists
        hf = fopen(historyPath, "w");
    }
    if (hf) {
        // Version and user
        fprintf(hf, "Version_%d by %s\n", newVersion, currentUser);
        // Commit message
        fprintf(hf, "%s\n", message);
        // Date and time
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char datetime[64];
        strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(hf, "%s\n", datetime);
        fclose(hf);
    } else {
        printf("Warning: Could not write to History.txt\n");
    }
    // --------------------------------------------------------

    free(currentUser);
}
void pullFiles(void) {
    char repo[100];
    printf("Enter repository name: ");
    scanf(" %99s", repo);

    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "ERROR: HOME not set\n");
        return;
    }

    char repoPath[PATH_MAX];
    snprintf(repoPath, PATH_MAX, "%s/CtrlZ/%s", home, repo);

    struct stat st;
    if (stat(repoPath, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("Repository '%s' does not exist in %s/CtrlZ.\n", repo, home);
        return;
    }

    // Open Config.txt
    char configPath[PATH_MAX];
    snprintf(configPath, PATH_MAX, "%s/Config.txt", repoPath);
    FILE *cfg = fopen(configPath, "r");
    if (!cfg) {
        perror("Could not open Config.txt");
        return;
    }

    char author[100] = "", collaborators[1024] = "", privacy[32] = "";
    char line[1024];
    while (fgets(line, sizeof(line), cfg)) {
        if (strncmp(line, "Author:", 7) == 0) {
            char *p = line + 7;
            while (*p == ' ' || *p == '\t') p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(author, p, sizeof(author) - 1);
        } else if (strncmp(line, "Collaborators:", 14) == 0) {
            char *p = line + 14;
            while (*p == ' ' || *p == '\t') p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(collaborators, p, sizeof(collaborators) - 1);
        } else if (strncmp(line, "Privacy:", 8) == 0) {
            char *p = line + 8;
            while (*p == ' ' || *p == '\t') p++;
            p[strcspn(p, "\r\n")] = '\0';
            strncpy(privacy, p, sizeof(privacy) - 1);
        }
    }
    fclose(cfg);

    // Get logged in user
    char *currentUser = getLoggedInUser();
    if (!currentUser) {
        printf("No user is currently logged in.\n");
        return;
    }

    // Check if user is author, collaborator, or public
    int allowed = 0;
    if (strcmp(currentUser, author) == 0) {
        allowed = 1;
    } else if (strlen(collaborators) > 0) {
        char search[110];
        snprintf(search, sizeof(search), "%s,", currentUser);
        if (strstr(collaborators, search) != NULL) {
            allowed = 1;
        }
    }
    if (!allowed && (strcasecmp(privacy, "public") == 0)) {
        allowed = 1;
    }

    if (!allowed) {
        printf("You are not authorized to pull from this repository.\n");
        free(currentUser);
        return;
    }

    // Display available versions with messages
    char historyPath[PATH_MAX];
    snprintf(historyPath, PATH_MAX, "%s/History.txt", repoPath);
    FILE *hf = fopen(historyPath, "r");
    if (!hf) {
        printf("No history found for this repository.\n");
        free(currentUser);
        return;
    }

    // Parse History.txt for version info
    typedef struct { int version; char message[1024]; } VersionInfo;
    VersionInfo versions[100];
    int vcount = 0;
    char vline[1024], mline[1024], dline[1024];
    while (fgets(vline, sizeof(vline), hf) && fgets(mline, sizeof(mline), hf) && fgets(dline, sizeof(dline), hf)) {
        int vnum = 0;
        if (sscanf(vline, "Version_%d", &vnum) == 1) {
            versions[vcount].version = vnum;
            strncpy(versions[vcount].message, mline, sizeof(versions[vcount].message) - 1);
            versions[vcount].message[strcspn(versions[vcount].message, "\r\n")] = '\0';
            vcount++;
        }
    }
    fclose(hf);

    if (vcount == 0) {
        printf("No versions found in history.\n");
        free(currentUser);
        return;
    }

    printf("Available versions:\n");
    for (int i = 0; i < vcount; ++i) {
        printf("%d. Version_%d - %s\n", i + 1, versions[i].version, versions[i].message);
    }

    int choice = 0;
    printf("Enter the number of the version to pull: ");
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > vcount) {
        printf("Invalid version choice.\n");
        free(currentUser);
        return;
    }
    int selectedVersion = versions[choice - 1].version;

    // Ask for destination folder
    char destPath[PATH_MAX];
    printf("Enter the destination folder path: ");
    getchar(); // clear newline left by previous scanf
    if (fgets(destPath, sizeof(destPath), stdin) == NULL) {
        printf("Invalid destination path input.\n");
        free(currentUser);
        return;
    }
    destPath[strcspn(destPath, "\r\n")] = '\0'; // Remove newline

    // Resolve destination path
    char absDestPath[PATH_MAX];
    if (realpath(destPath, absDestPath) == NULL) {
        perror("Invalid destination path");
        free(currentUser);
        return;
    }

    // Source version path
    char srcVersionPath[PATH_MAX];
    snprintf(srcVersionPath, PATH_MAX, "%s/Version_%d", repoPath, selectedVersion);

    struct stat vst;
    if (stat(srcVersionPath, &vst) != 0 || !S_ISDIR(vst.st_mode)) {
        printf("Selected version directory does not exist.\n");
        free(currentUser);
        return;
    }

    // Copy files
    copyAll(srcVersionPath, absDestPath);

    printf("Pulled Version_%d to %s\n", selectedVersion, absDestPath);

    free(currentUser);
}
void exitProgram(void)
{
    printf("Exiting the program...\n");
    exit(0);
}

void printProjectInfo(void)
{
    printf("--------------------------------------------------------\n");
    printf("----------------------- CTRL + Z -----------------------\n");
    printf("---------------- Version Control System ----------------\n");
    printf("--------------------------------------------------------\n\n");
}

int initProject(int *isSomeoneLoggedIn)
{
    const char *home = getenv("HOME");
    if (!home)
    {
        fprintf(stderr, "ERROR: HOME not set\n");
        return -1;
    }

    // 1) mkdir ~/CtrlZ
    char dirpath[PATH_MAX];
    snprintf(dirpath, PATH_MAX, "%s/CtrlZ", home);

    struct stat st;
    if (stat(dirpath, &st) == -1)
    {
        if (mkdir(dirpath, 0755) != 0)
        {
            perror("mkdir CtrlZ");
            return -1;
        }
    }

    // 2) Ensure UserData.txt exists
    char filepath[PATH_MAX];
    getUserDataPath(filepath);

    if (stat(filepath, &st) == -1)
    {
        // File missing → create with header
        FILE *f = fopen(filepath, "w");
        if (!f)
        {
            perror("fopen UserData.txt");
            return -1;
        }
        // Decide what goes in the header
        const char *loginUser =
            (*isSomeoneLoggedIn) ? getenv("USER") : "NoUserIsLoggedIn";
        if (!loginUser)
            loginUser = "NoUserIsLoggedIn";

        // Note the space after colon
        fprintf(f, "CurrentlyLoggedInUser: %s\n", loginUser);
        fclose(f);

        // No users yet, so flag stays as passed in
        return 0;
    }

    // 3) File exists → read first line
    {
        FILE *f = fopen(filepath, "r");
        if (!f)
        {
            perror("fopen existing UserData.txt");
            return -1;
        }
        char line[512];
        if (fgets(line, sizeof(line), f))
        {
            if (strstr(line, "NoUserIsLoggedIn"))
                *isSomeoneLoggedIn = 0;
            else
                *isSomeoneLoggedIn = 1;
        }
        fclose(f);
    }

    return 0;
}

void browseRepositories(void)
{
    char ctrlz[PATH_MAX];
    const char *home = getenv("HOME");
    if (!home)
        home = "";
    snprintf(ctrlz, sizeof(ctrlz), "%s/CtrlZ", home);

    // 2) collect subdirectory names
    DIR *d = opendir(ctrlz);
    if (!d)
    {
        perror("opendir CtrlZ");
        return;
    }
    char **names = NULL;
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)))
    {
#ifdef DT_DIR
        if (ent->d_type == DT_DIR &&
            strcmp(ent->d_name, ".") &&
            strcmp(ent->d_name, ".."))
#else
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", ctrlz, ent->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode) &&
            strcmp(ent->d_name, ".") &&
            strcmp(ent->d_name, ".."))
#endif
        {
            names = realloc(names, sizeof(char *) * (count + 1));
            names[count++] = strdup(ent->d_name);
        }
    }
    closedir(d);

    if (count == 0)
    {
        printf("No repositories found in %s\n", ctrlz);
        free(names);
        return;
    }

    // Get current user (if any)
    char *currentUser = getLoggedInUser();

    // Scan each repo for privacy and user access
    int found = 0;
    for (int i = 0; i < count; i++)
    {
        char cfgpath[PATH_MAX + 50], line[512];
        snprintf(cfgpath, sizeof(cfgpath), "%s/%s/Config.txt", ctrlz, names[i]);
        FILE *f = fopen(cfgpath, "r");
        if (!f)
            continue;

        char author[100] = "", collaborators[1024] = "", privacy[32] = "";
        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, "Author:", 7) == 0)
            {
                char *p = line + 7;
                while (*p == ' ' || *p == '\t') p++;
                p[strcspn(p, "\r\n")] = '\0';
                strncpy(author, p, sizeof(author) - 1);
            }
            else if (strncmp(line, "Collaborators:", 14) == 0)
            {
                char *p = line + 14;
                while (*p == ' ' || *p == '\t') p++;
                p[strcspn(p, "\r\n")] = '\0';
                strncpy(collaborators, p, sizeof(collaborators) - 1);
            }
            else if (strncmp(line, "Privacy:", 8) == 0)
            {
                char *p = line + 8;
                while (*p == ' ' || *p == '\t') p++;
                p[strcspn(p, "\r\n")] = '\0';
                strncpy(privacy, p, sizeof(privacy) - 1);
            }
        }
        fclose(f);

        int show = 0;
        if (strcasecmp(privacy, "public") == 0)
        {
            show = 1;
        }
        else if (currentUser)
        {
            // Check if currentUser is author or in collaborators
            if (strcmp(currentUser, author) == 0)
                show = 1;
            else if (strlen(collaborators) > 0)
            {
                char search[110];
                snprintf(search, sizeof(search), "%s,", currentUser);
                if (strstr(collaborators, search) != NULL)
                    show = 1;
            }
        }
        if (show)
        {
            printf("%s repo: %s\n", strcasecmp(privacy, "public") == 0 ? "Public" : "Private", names[i]);
            found = 1;
        }
    }

    if (!found)
        printf("No accessible repositories found.\n");

    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
    if (currentUser)
        free(currentUser);
}

void addCollaborator(void)
{
    char repo[100], collab[50];
    printf("Enter repository name: ");
    scanf(" %99s", repo);
    printf("Enter collaborator's username: ");
    scanf(" %49s", collab);

    // 1) Check repo folder exists
    char home[PATH_MAX], repoDir[PATH_MAX], cfgPath[PATH_MAX];
    snprintf(home, PATH_MAX, "%s/CtrlZ", getenv("HOME"));
    snprintf(repoDir, PATH_MAX, "%s/%s", home, repo);
    struct stat st;
    if (stat(repoDir, &st) < 0 || !S_ISDIR(st.st_mode))
    {
        printf("Error: repository '%s' not found.\n", repo);
        return;
    }

    // 2) Load Config.txt and parse keys
    snprintf(cfgPath, PATH_MAX, "%s/Config.txt", repoDir);
    FILE *f = fopen(cfgPath, "r");
    if (!f)
    {
        perror("fopen Config.txt");
        return;
    }

    char buf[512];
    char *author = NULL, *collabList = NULL, *privacy = NULL;
    while (fgets(buf, sizeof(buf), f))
    {
        if (strncmp(buf, "Author:", 7) == 0)
        {
            char *p = buf + 7;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            author = strdup(p);
        }
        else if (strncmp(buf, "Collaborators:", 14) == 0)
        {
            char *p = buf + 14;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            collabList = strdup(p);
        }
        else if (strncmp(buf, "Privacy:", 8) == 0)
        {
            char *p = buf + 8;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            privacy = strdup(p);
        }
    }
    fclose(f);

    if (!author)
    {
        printf("Error: no Author in Config.txt\n");
        goto done;
    }
    if (!privacy)
        privacy = strdup("Private"); // default if missing
    if (!collabList)
        collabList = strdup(""); // start empty

    // 3) Check ownership
    char *current = getLoggedInUser();
    if (!current || strcmp(current, author) != 0)
    {
        printf("Error: you do not own this repository.\n");
        goto done;
    }

    // 4) Verify collaborator exists in UserData.txt
    char udPath[PATH_MAX];
    getUserDataPath(udPath);
    FILE *uf = fopen(udPath, "r");
    if (!uf)
    {
        perror("fopen UserData.txt");
        goto done;
    }
    fgets(buf, sizeof(buf), uf); // skip header
    int found = 0;
    while (fgets(buf, sizeof(buf), uf))
    {
        char *c1 = strchr(buf, ',');
        if (!c1)
            continue;
        char *usr = c1 + 1;
        char *c2 = strchr(usr, ',');
        if (c2)
            *c2 = '\0';
        if (strcmp(usr, collab) == 0)
        {
            found = 1;
            break;
        }
    }
    fclose(uf);
    if (!found)
    {
        printf("Error: user '%s' not found.\n", collab);
        goto done;
    }

    // 5) Append collaborator (ensure ending comma)
    size_t needed = strlen(collabList) + strlen(collab) + 2;
    collabList = realloc(collabList, needed);
    strcat(collabList, collab);
    strcat(collabList, ",");

    // 6) Rewrite Config.txt in required format
    f = fopen(cfgPath, "w");
    if (!f)
    {
        perror("fopen write Config.txt");
        goto done;
    }
    fprintf(f, "Author:%s\n", author);
    fprintf(f, "Collaborators:%s\n", collabList);
    fprintf(f, "Privacy:%s\n", privacy);
    fclose(f);

    printf("Added '%s' as collaborator to '%s'.\n", collab, repo);

done:
    free(author);
    free(collabList);
    free(privacy);
}

void removeCollaborator(void)
{
    char repo[100], collab[50];
    printf("Enter repository name: ");
    scanf(" %99s", repo);
    printf("Enter collaborator's username to remove: ");
    scanf(" %49s", collab);

    // 1) Check repo folder exists
    char home[PATH_MAX], repoDir[PATH_MAX], cfgPath[PATH_MAX];
    snprintf(home, PATH_MAX, "%s/CtrlZ", getenv("HOME"));
    snprintf(repoDir, PATH_MAX, "%s/%s", home, repo);
    struct stat st;
    if (stat(repoDir, &st) < 0 || !S_ISDIR(st.st_mode))
    {
        printf("Error: repository '%s' not found.\n", repo);
        return;
    }

    // 2) Load Config.txt and parse keys
    snprintf(cfgPath, PATH_MAX, "%s/Config.txt", repoDir);
    FILE *f = fopen(cfgPath, "r");
    if (!f)
    {
        perror("fopen Config.txt");
        return;
    }

    char buf[512];
    char *author = NULL, *collabList = NULL, *privacy = NULL;
    while (fgets(buf, sizeof(buf), f))
    {
        if (strncmp(buf, "Author:", 7) == 0)
        {
            char *p = buf + 7;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            author = strdup(p);
        }
        else if (strncmp(buf, "Collaborators:", 14) == 0)
        {
            char *p = buf + 14;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            collabList = strdup(p);
        }
        else if (strncmp(buf, "Privacy:", 8) == 0)
        {
            char *p = buf + 8;
            while (*p == ' ' || *p == '\t')
                p++;
            p[strcspn(p, "\r\n")] = '\0';
            privacy = strdup(p);
        }
    }
    fclose(f);

    if (!author)
    {
        printf("Error: no Author in Config.txt\n");
        goto done;
    }
    if (!privacy)
        privacy = strdup("Private");
    if (!collabList)
        collabList = strdup("");

    // 3) Verify ownership
    char *current = getLoggedInUser();
    if (!current || strcmp(current, author) != 0)
    {
        printf("Error: you do not own this repository.\n");
        goto done;
    }

    // 4) Remove collaborator occurrences of "username,"
    {
        char *dst = malloc(strlen(collabList) + 1), *d = dst;
        char *tok = collabList, *next;
        size_t keylen = strlen(collab) + 1; // include comma
        while ((next = strstr(tok, collab)))
        {
            // copy up to the match
            size_t chunk = next - tok;
            memcpy(d, tok, chunk);
            d += chunk;
            tok = next + keylen; // skip over "username,"
        }
        // copy remainder
        strcpy(d, tok);
        free(collabList);
        collabList = dst;
    }

    // 5) Rewrite Config.txt
    f = fopen(cfgPath, "w");
    if (!f)
    {
        perror("fopen write Config.txt");
        goto done;
    }
    fprintf(f, "Author:%s\n", author);
    fprintf(f, "Collaborators:%s\n", collabList);
    fprintf(f, "Privacy:%s\n", privacy);
    fclose(f);
    printf("Removed '%s' from collaborators of '%s'.\n", collab, repo);

done:
    free(author);
    free(collabList);
    free(privacy);
}
