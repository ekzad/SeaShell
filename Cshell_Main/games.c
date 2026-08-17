#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif
void get_exe_dir(char *out, size_t out_size) {
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
#else
    char exePath[1024] = {0};
    readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
#endif

    // strip the filename off, keep just the folder
    char *lastSlash = strrchr(exePath, '\\');
    if (lastSlash == NULL) lastSlash = strrchr(exePath, '/');
    if (lastSlash != NULL) *lastSlash = '\0';

    strncpy(out, exePath, out_size - 1);
    out[out_size - 1] = '\0';
}
void games(const char *arg) {
    (void)arg;
    printf("Here are the games CShell\n");

    char exeDir[1024];
    get_exe_dir(exeDir, sizeof(exeDir));

    char regPath[1200];
    snprintf(regPath, sizeof(regPath), "%s/games_reg.txt", exeDir);

    FILE *fp = fopen(regPath, "r");
    if (fp == NULL) {
        printf("[!] An error occurred while reading games registry...\n");
        return;
    }

    if (fgetc(fp) == EOF) {
        printf("Registration is empty\n");
        fclose(fp);
        return;
    }

    rewind(fp);
    int c;
    while ((c = fgetc(fp)) != EOF) {
        putchar(c);
    }
    fclose(fp);

    printf("\nChoose a game from above\n");
    printf("ENTER THE CORRESPONDING NUMBER!\n");
    printf("Enter 50 to exit\n");
    printf(">> ");

    int opt;
    scanf("%d", &opt);
    getchar();
    if (opt == 50) {
        printf("Exiting GameShell\n");
        return;
    }

    // for now, game devs on CShell must integrate their games into THIS piece of code here
    // games must have a " enter to exit " at the end of them. all must have this
    // 1. Snake:
    if (opt == 1) {
        char gamePath[1200];
#ifdef _WIN32
        snprintf(gamePath, sizeof(gamePath), "%s/games/snake.exe", exeDir);
#else
        snprintf(gamePath, sizeof(gamePath), "%s/games/snake", exeDir);
#endif

        printf("Playing snake...\n");
        system(gamePath);
        printf("Games are terminal based. If you are seeing this, you most likely exited the game. Press enter to go back to the main menu\n");
        getchar();
        return;
    } else {
        printf("Invalid choice...\n");
    }
}