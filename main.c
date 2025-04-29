#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Function prototypes
void browsePublicRepositories(void);
void publishRepository(void);
void pushFiles(void);
void pullFiles(void);
void addCollaborator(void);
void login(int *);
void logout(int *);
void registerUser(void);
void exitProgram(void);
void printProjectInfo(void);

// Utils
int initProject(int *);
static void getUserDataPath(char *);
char *getLoggedInUser(void);

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
        printf("1. Browse public repositories\n");
        printf("2. Publish your own repository\n");
        printf("3. Push your files\n");
        printf("4. Pull the files\n");
        printf("5. Add collaborator to your project\n");
        if (!isSomeoneLoggedIn)
        {
            printf("6. Login\n");
            printf("7. Register as a new user\n");
            printf("8. Exit\n");
        }
        else
        {
            printf("6. Logout\n");
            printf("7. Exit\n");
        }
        printf("--------------------------------------------------------\n");

        int choice;
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n')
                ; // clear input buffer
            continue;
        }

        switch (choice)
        {
        case 1:
            browsePublicRepositories();
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
            if (isSomeoneLoggedIn)
            {
                logout(&isSomeoneLoggedIn);
            }
            else
            {
                login(&isSomeoneLoggedIn);
            }
            break;
        case 7:
            if (isSomeoneLoggedIn)
            {
                exitProgram();
            }
            else
            {
                registerUser();
            }
            break;
        case 8:
            if (!isSomeoneLoggedIn)
            {
                printf("Exiting the program...\n");
                exit(0);
            }
            else
            {
                printf("Invalid choice. Please try again.\n");
            }
            break;
        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
    }

    return 0;
}

void browsePublicRepositories(void) { printf("Browsing public repositories...\n"); }
void publishRepository(void) { printf("Publishing repository...\n"); }
void pushFiles(void) { printf("Pushing files...\n"); }
void pullFiles(void) { printf("Pulling files...\n"); }
void addCollaborator(void) { printf("Adding collaborator...\n"); }
void login(int *isSomeoneLoggedIn)
{
    char user[50], pass[50];
    printf("Username: ");
    scanf(" %49s", user);
    printf("Password: ");
    scanf(" %49s", pass);

    char path[PATH_MAX];
    getUserDataPath(path);

    // read all lines
    FILE *f = fopen(path, "r");
    if (!f)
    {
        perror("fopen");
        return;
    }
    char *lines[1000];
    int n = 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), f) && n < 1000)
    {
        lines[n++] = strdup(buf);
    }
    fclose(f);

    // search credentials in lines[1..]
    int found = 0;
    for (int i = 1; i < n; i++)
    {
        // line format: Name,username,password\n
        char *p1 = strchr(lines[i], ',');
        if (!p1)
            continue;
        char *usr = p1 + 1;
        char *p2 = strchr(usr, ',');
        if (!p2)
            continue;
        *p2 = '\0';
        char *pwd = p2 + 1;
        char *nl = strchr(pwd, '\n');
        if (nl)
            *nl = '\0';

        if (strcmp(usr, user) == 0 && strcmp(pwd, pass) == 0)
        {
            found = 1;
            break;
        }
    }
    if (!found)
    {
        printf("Login failed: invalid username or password.\n");
    }
    else
    {
        // rewrite file with updated header
        FILE *fw = fopen(path, "w");
        if (!fw)
        {
            perror("fopen");
        }
        else
        {
            fprintf(fw, "CurrentlyLoggedInUser:%s\n", user);
            for (int i = 1; i < n; i++)
                fputs(lines[i], fw);
            fclose(fw);
            printf("Login successful. Welcome, %s!\n", user);
        }
        *isSomeoneLoggedIn = 1;
    }

    // free buffers
    for (int i = 0; i < n; i++)
        free(lines[i]);
}
void logout(int *isSomeoneLoggedIn)
{
    char path[PATH_MAX];
    getUserDataPath(path);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        perror("fopen");
        return;
    }
    char *lines[1000];
    int n = 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), f) && n < 1000)
    {
        lines[n++] = strdup(buf);
    }
    fclose(f);

    // rewrite
    FILE *fw = fopen(path, "w");
    if (!fw)
    {
        perror("fopen");
    }
    else
    {
        fprintf(fw, "CurrentlyLoggedInUser:NoUserIsLoggedIn\n");
        for (int i = 1; i < n; i++)
            fputs(lines[i], fw);
        fclose(fw);
        printf("Logged out successfully.\n");
        *isSomeoneLoggedIn = 0;
    }
    for (int i = 0; i < n; i++)
        free(lines[i]);
}

void registerUser(void)
{
    char fullName[100], user[50], pass[50];
    printf("Enter full name: ");
    scanf(" %99[^\n]", fullName);
    printf("Enter username: ");
    scanf(" %49s", user);
    printf("Enter password: ");
    scanf(" %49s", pass);

    char path[PATH_MAX];
    getUserDataPath(path);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        perror("fopen");
        return;
    }

    // skip header
    char line[512];
    fgets(line, sizeof(line), f);

    // check uniqueness
    while (fgets(line, sizeof(line), f))
    {
        // parse username
        char *p1 = strchr(line, ',');
        if (!p1)
            continue;
        char *p2 = strchr(p1 + 1, ',');
        if (!p2)
            continue;
        *p2 = '\0';
        if (strcmp(p1 + 1, user) == 0)
        {
            printf("Error: username '%s' already exists.\n", user);
            fclose(f);
            return;
        }
    }
    fclose(f);

    // append new user
    f = fopen(path, "a");
    if (!f)
    {
        perror("fopen");
        return;
    }
    fprintf(f, "%s,%s,%s\n", fullName, user, pass);
    fclose(f);
    printf("User '%s' registered successfully.\n", user);
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

void getUserDataPath(char *outPath)
{
    const char *home = getenv("HOME");
    if (!home)
        home = "";
    snprintf(outPath, PATH_MAX, "%s/CtrlZ/UserData.txt", home);
}

/// Returns a malloc’d string containing the logged-in username,
/// or NULL if none or on error. Caller must free().
char *getLoggedInUser(void)
{
    char path[PATH_MAX];
    getUserDataPath(path);
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;
    char line[512];
    if (!fgets(line, sizeof(line), f))
    {
        fclose(f);
        return NULL;
    }
    fclose(f);

    // Parse after colon+space
    char *p = strchr(line, ':');
    if (!p)
        return NULL;
    p++; // skip colon
    if (*p == ' ')
        p++; // skip optional space
    // strip newline
    char *nl = strchr(p, '\n');
    if (nl)
        *nl = '\0';
    return strdup(p);
}
