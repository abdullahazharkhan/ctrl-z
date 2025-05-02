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
#include "version.c"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// mutex for browseRepositories()
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void browseRepositories(void);
void addCollaborator(void);
void removeCollaborator(void);

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
        printf("\n----------------------- Main Menu ----------------------\n");
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
