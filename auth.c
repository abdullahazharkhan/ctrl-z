#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Global variable for current user (malloc'd, or NULL)
char *currentUser = NULL;

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
    if (!currentUser)
        return NULL;
    return strdup(currentUser);
}

void login(int *isSomeoneLoggedIn)
{
    char user[50], pass[50];
    printf("Username: ");
    scanf(" %49s", user);
    printf("Password: ");
    scanf(" %49s", pass);

    char path[PATH_MAX];
    getUserDataPath(path);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        perror("fopen");
        return;
    }
    char buf[512];
    int found = 0;
    while (fgets(buf, sizeof(buf), f))
    {
        // parse: Name,username,password\n
        char *p1 = strchr(buf, ',');
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
    fclose(f);

    if (!found)
    {
        printf("Login failed: invalid username or password.\n");
    }
    else
    {
        // Set global currentUser
        if (currentUser)
            free(currentUser);
        currentUser = strdup(user);
        printf("Login successful. Welcome, %s!\n", user);
        *isSomeoneLoggedIn = 1;
    }
}

void logout(int *isSomeoneLoggedIn)
{
    if (currentUser)
    {
        free(currentUser);
        currentUser = NULL;
    }
    printf("Logged out successfully.\n");
    *isSomeoneLoggedIn = 0;
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

    char line[512];
    // No header to skip

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