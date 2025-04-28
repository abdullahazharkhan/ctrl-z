#include <stdio.h>
#include <stdlib.h> // for exit()

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

int main(int argc, char *argv[])
{
    int isSomeoneLoggedIn = 0;

    printProjectInfo();

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
