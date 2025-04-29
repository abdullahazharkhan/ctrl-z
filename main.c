#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "auth.c"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Function prototypes
void browsePublicRepositories(void);
void publishRepository(void);
void pushFiles(void);
void pullFiles(void);
void addCollaborator(void);
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