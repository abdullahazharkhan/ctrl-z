#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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

    // Strip newline
    char *nl = strchr(p, '\n');
    if (nl)
        *nl = '\0';

    return strdup(p); // Caller must free
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

    // Read all lines into memory
    FILE *f = fopen(path, "r");
    if (!f)
    {
        perror("Error opening file");
        return;
    }

    char *lines[1000] = {0};
    int n = 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), f) && n < 1000)
    {
        lines[n++] = strdup(buf);
    }
    fclose(f);

    // Search for matching credentials
    int found = 0;
    for (int i = 1; i < n; i++)
    {
        char temp[512];
        strncpy(temp, lines[i], sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';

        // Parse line
        char *p1 = strchr(temp, ',');
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
        FILE *fw = fopen(path, "w");
        if (!fw)
        {
            perror("Error opening file for writing");
        }
        else
        {
            fprintf(fw, "CurrentlyLoggedInUser: %s,%s\n", user, pass);
            for (int i = 1; i < n; i++)
            {
                fputs(lines[i], fw);
            }
            fclose(fw);
            printf("Login successful. Welcome, %s!\n", user);
            *isSomeoneLoggedIn = 1;
        }
    }

    // Free memory
    for (int i = 0; i < n; i++)
    {
        free(lines[i]);
    }
}

void logout(int *isSomeoneLoggedIn)
{
    char path[PATH_MAX];
    getUserDataPath(path);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        perror("Error opening file");
        return;
    }

    char *lines[1000] = {0};
    int n = 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), f) && n < 1000)
    {
        lines[n++] = strdup(buf);
    }
    fclose(f);

    // Rewrite the file
    FILE *fw = fopen(path, "w");
    if (!fw)
    {
        perror("Error opening file for writing");
    }
    else
    {
        fprintf(fw, "CurrentlyLoggedInUser: NoUserIsLoggedIn\n");
        for (int i = 1; i < n; i++)
        {
            fputs(lines[i], fw);
        }
        fclose(fw);
        printf("Logged out successfully.\n");
        *isSomeoneLoggedIn = 0;
    }

    // Free memory
    for (int i = 0; i < n; i++)
    {
        free(lines[i]);
    }
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
        perror("Error opening file");
        return;
    }

    // Read all lines into memory
    char *lines[1000] = {0};
    int n = 0;
    char buf[512];
    fgets(buf, sizeof(buf), f); // Skip header
    while (fgets(buf, sizeof(buf), f) && n < 1000)
    {
        lines[n++] = strdup(buf);
    }
    fclose(f);

    // Check for duplicate username
    for (int i = 0; i < n; i++)
    {
        char temp[512];
        strncpy(temp, lines[i], sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';

        char *p1 = strchr(temp, ',');
        if (!p1)
            continue;
        char *p2 = strchr(p1 + 1, ',');
        if (!p2)
            continue;
        *p2 = '\0';

        if (strcmp(p1 + 1, user) == 0)
        {
            printf("Error: username '%s' already exists.\n", user);

            // Free memory
            for (int i = 0; i < n; i++)
            {
                free(lines[i]);
            }
            return;
        }
    }

    // Append the new user
    f = fopen(path, "a");
    if (!f)
    {
        perror("Error opening file for appending");
    }
    else
    {
        fprintf(f, "%s,%s,%s\n", fullName, user, pass);
        fclose(f);
        printf("User '%s' registered successfully.\n", user);
    }

    // Free memory
    for (int i = 0; i < n; i++)
    {
        free(lines[i]);
    }
}
