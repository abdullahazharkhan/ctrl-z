#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

// Function prototypes
void browsePublicRepositories(void);
void publishRepository(void);
void pushFiles(void);
void pullFiles(void);
void addCollaborator(void);
void login(void);
void logout(void);
void registerUser(void);
void exitProgram(void);
void printProjectInfo(void);
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
    printf("User is %slogged in\n", isSomeoneLoggedIn ? "" : "not ");

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
                logout();
                isSomeoneLoggedIn = 0;
            }
            else
            {
                login();
                isSomeoneLoggedIn = 1;
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

// Dummy implementations
void browsePublicRepositories(void) { printf("Browsing public repositories...\n"); }
void publishRepository(void) { printf("Publishing repository...\n"); }
void pushFiles(void) { printf("Pushing files...\n"); }
void pullFiles(void) { printf("Pulling files...\n"); }
void addCollaborator(void) { printf("Adding collaborator...\n"); }
void login(void) { printf("Logging in...\n"); }
void logout(void) { printf("Logging out...\n"); }
void registerUser(void) { printf("Registering new user...\n"); }

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
        fprintf(stderr, "Could not get HOME directory\n");
        return -1;
    }

    // Build path: ~/Desktop/CtrlZ
    char dirpath[PATH_MAX];
    snprintf(dirpath, sizeof(dirpath), "%s/Desktop/CtrlZ", home);

    struct stat st = {0};
    if (stat(dirpath, &st) == -1)
    {
        // Directory doesn't exist → create it
        if (mkdir(dirpath, 0744) != 0)
        {
            perror("mkdir");
            return -1;
        }

        // Create UserData.txt inside it
        char filepath[PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/UserData.txt", dirpath);
        FILE *f = fopen(filepath, "w");
        if (!f)
        {
            perror("fopen");
            return -1;
        }

        // Decide header
        const char *loginUser = (*isSomeoneLoggedIn)
                                    ? getenv("USER")
                                    : "NoUserIsLoggedIn";
        if (!loginUser)
            loginUser = "NoUserIsLoggedIn";

        fprintf(f, "CurrentlyLoggedInUser:%s\n", loginUser);
        // You can seed additional sample users here if you like:
        // fprintf(f, "Alice,alice,pass123\nBob,bob,secret\n");

        fclose(f);
        // If we just created, honor the passed-in flag
        return 0;
    }

    // Directory already exists → read existing UserData.txt
    {
        char filepath[PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/UserData.txt", dirpath);
        FILE *f = fopen(filepath, "r");
        if (!f)
        {
            perror("fopen");
            return -1;
        }

        char line[512];
        if (fgets(line, sizeof(line), f))
        {
            // Check if it contains "NoUserIsLoggedIn"
            if (strstr(line, "NoUserIsLoggedIn"))
                *isSomeoneLoggedIn = 0;
            else
                *isSomeoneLoggedIn = 1;
        }
        fclose(f);
    }

    return 0;
}
