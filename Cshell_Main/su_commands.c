#include "prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include "colors.h"
#ifdef _WIN32
#include <windows.h>
#endif

// good thing is, normus cant just check what commands exist by brute force checking like:
// sudo this sudo that, because they'll always get hit with "you must be a super user to do that" even if that command exists
// or doesnt exist
void handle_su_cmds(const char *arg) {
    if (arg == NULL || arg[0] == ' ' || arg[0] == '\0') {
        printf(COLOR_BRIGHT_RED "[!] Invalid entry\n" COLOR_RESET);
        return;
    }
    if (!super_user) {
        printf(COLOR_BRIGHT_RED "You must be a super user to do this!\n" COLOR_RESET);
        return;
    }
    // sudo commands:
    if (strcmp(arg, "help") == 0) {
        printf("1. csu - Change super user password\n");
        // add new commands here guys
    }
    else if (strcmp(arg, "csu") == 0) {
    
        printf("What shall the password of RootSU be?\n");
        printf("? >> ");
    
        char new_password[64];
        if (fgets(new_password, sizeof(new_password), stdin) == NULL) {
            printf(COLOR_BRIGHT_RED "\nDidn't get that. Try again.\n" COLOR_RESET);
            return;
        }
        new_password[strcspn(new_password, "\n")] = '\0';
    
        char exeDir[1024];
        get_exe_dir(exeDir, sizeof(exeDir));
    
        char path[1200];
        snprintf(path, sizeof(path), "%s/super_user.txt", exeDir);
    
        FILE *fp = fopen(path, "w"); // "w" overwrites — correct here, we're replacing the whole file
        if (fp == NULL) {
            printf(COLOR_BRIGHT_RED "[!] Couldn't write new password\n" COLOR_RESET);
            return;
        }
    
        fprintf(fp, "%s\n", new_password);
        fclose(fp);
    
        strncpy(password, new_password, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
    
        printf("Super User password updated.\n");
    }
    else {
        printf(COLOR_BRIGHT_RED "That command does not exist in sudo. Try sudo help\n" COLOR_RESET);
        return;
    }
}