#include <stdlib.h>
#include <stdio.h>
#include "prototypes.h"
#include "colors.h"
void app_handle(const char *arg) {
    if (arg == NULL || arg[0] == '\0') {
        printf(COLOR_YELLOW "To launch apps: apps <app_name>\n" COLOR_RESET);
        names(arg);
    }
    else {
        execute(arg);
    }
}
void names(const char *arg) {
    (void)arg;

    char exeDir[1024];
    get_exe_dir(exeDir, sizeof(exeDir));

    char path[1200];
    snprintf(path, sizeof(path), "%s/apps/registry.txt", exeDir);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf(COLOR_BRIGHT_RED "[!] Couldn't load app registry\n" COLOR_RESET);
        return;
    }

    printf("Current available applications on CShell:\n");

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        char *eq = strchr(line, '=');
        if (eq == NULL) continue;
        *eq = '\0';
        printf(" - %s\n", line); // just the name
    }

    fclose(fp);
}
void execute(const char *arg) {
    // no need to check for an empty arg in my opinion
    // since we do that in app_handle()
    char exeDir[1024];
    get_exe_dir(exeDir, sizeof(exeDir));

    char regPath[1200];
    snprintf(regPath, sizeof(regPath), "%s/apps/registry.txt", exeDir);

    FILE *fp = fopen(regPath, "r");
    if (fp == NULL) {
        printf(COLOR_BRIGHT_RED "[!] Couldn't load app registry\n" COLOR_RESET);
        return;
    }

    char line[256];
    bool found = false;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        char *eq = strchr(line, '=');
        if (eq == NULL) continue;

        *eq = '\0';
        char *name = line;
        char *exeFilename = eq + 1;

        if (strcmp(name, arg) == 0) {
            found = true;

            char fullPath[1400];
            snprintf(fullPath, sizeof(fullPath), "%s/apps/exes/%s", exeDir, exeFilename);

            printf("Launching %s...\n", name);
            system(fullPath);
            break;
        }
    }

    fclose(fp);

    if (!found) {
        printf(COLOR_BRIGHT_RED "[!] No app named '%s' found. Run with no arguments to see available apps.\n" COLOR_RESET, arg);
    }
}