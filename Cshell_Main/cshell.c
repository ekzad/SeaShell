#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <signal.h>
#include "prototypes.h"
#include "colors.h"
volatile sig_atomic_t interrupt_requested =0;
#ifdef _WIN32 // if ur on windows, this will be chosen

#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
BOOL WINAPI console_handler(DWORD sig) {
    if (sig == CTRL_C_EVENT) {
        interrupt_requested = 1;
        return true; // meaning, it handled it successfully
    }
    if (sig == CTRL_CLOSE_EVENT || sig == CTRL_LOGOFF_EVENT || sig == CTRL_SHUTDOWN_EVENT) {
        WSACleanup();      // real cleanup happens HERE, on this thread
        return TRUE;       // tell Windows: handled, don't force-kill immediately
    }
    return false;
}
#else // other os
void handle_signal(int sig) {
    (void)sig;
    interrupt_requested = 1;
}
#endif
// background/declarations/helper stuff
bool windcmd = false;
bool super_user = false;
char password[64];
// pre declarations so that the list knows what the hell we're talking about
// commands list
Command commands[] = {
    {"hello",   hello,        "hello",                     "Greets you"},
    {"goodbye", Goodbye,      "goodbye",                   "Says goodbye"},
    {"clean",   clear_screen, "clean",                     "Wipes the shell"},
    {"clear",   clear_screen, "clear",                     "Wipes the shell"},
    {"cls",     clear_screen, "cls",                       "Wipes the shell"},
    {"echo",    echo,         "echo <message>",            "Echoes a message"},
    {"cwd",     print_cwd,    "cwd",                        "Display the directory you're at"},
    {"mkdir",   make_directory,     "mkdir <directory_name>",     "Creates a directory"},
    {"mkfile",  mkfile,    "mkfile <file_name.format>",  "Creates a file"},
    {"chdir",   cd,           "chdir <directory>",          "Changes directory"},
    {"read",     cat,          "read <file_name>",            "Reads file content"},
    {"ldir",    ldir,     "ldir [directory_name]",      "Lists files/folders in a directory"},
    {"calc",    calc,         "calc <expression>",          "Built-in calculator, e.g. calc 2+3*6/4"},
    {"help",    help,         "help",                       "Shows this list"},
    {"mist",    mist,         "mist <arg>",                 "Enter 'mist help' for more"},
    {"netfo",   netfo,        "netfo",                      "Gives you info about your internet connection"},
    {"ping",    ping,         "ping <address>",             "Pings the destination"},
    {"get",     download,     "get <GITHUB_link>",          "Pulls files from a github repository link"},
    {"games",   games,        "games",                      "Play CShell games"},
    {"sudo", handle_su_cmds,  "sudo <action>",              "sudo help for more"},
    {"apps", app_handle,      "apps <app_name>",            "'app <app_name>' will start the app, 'app' will show the list of available apps"},
    {"cpuinfo", cpuinfo,      "cpuinfo",                    "gives you information about your current CPU"},
    {"larp",   larp,          "larp",                       "Local ARP Scanner"},
    {"rdp",    rdp,           "rdp <ip>",                   "Remote Desktop Protocol check for destination"},
    {"scan",   checkports,    "scan <ip>",                  "Scans common ports"}
};

const size_t command_count = sizeof(commands) / sizeof(commands[0]);
// calculator
const char *p;


Command *find_command(const char *name)
{
    size_t command_count = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }

    return NULL;
}

char *get_command(char *input) {
    return strtok(input, " ");
}

void check_sus(void) {
    char exeDir[1024];
    get_exe_dir(exeDir, sizeof(exeDir));

    char path[1200];
    snprintf(path, sizeof(path), "%s/super_user.txt", exeDir);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("[Shell Error] Couldn't fetch super user\n");
        return;
    }
    int empty_check;
    if ((empty_check = fgetc(fp)) == EOF) {
        printf("No super users in this shell session\n");
        fclose(fp); // this was also missing before, small leak fix
    }
    else {
        rewind(fp);
        if (fgets(password, sizeof(password), fp) == NULL) {
            printf("[Shell Error] No superuser codes found\n");
            fclose(fp);
            return;
        }
        password[strcspn(password, "\n")] = '\0';
        fclose(fp);
    }
}

// for the ascii art
void print_banner(void) {
    char exeDir[1024];
    get_exe_dir(exeDir, sizeof(exeDir));

    char path[1200];
    snprintf(path, sizeof(path), "%s/banner.txt", exeDir);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf(COLOR_BRIGHT_RED "[!] Couldn't load banner\n" COLOR_RESET);
        return;
    }
    int c;
    while ((c = fgetc(fp)) != EOF) {
        putchar(c);
    }
    fclose(fp);
}

int main(void) {
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("CShell experienced an error initializing networking...\n");
        return 1;
    }
    enable_ansi(); // we enable ansi codes ourselves just in case
    SetConsoleCtrlHandler(console_handler, TRUE);
    #else
    signal(SIGINT, handle_signal); // wow i had this so wrong
    #endif
    print_banner();
    printf("CShell 2026 - Developed by Ekzad - v1.30.220\n");
    // exit function:
    /* typing exit will quit by breaking the main loop and cleaning up WSA
       if you press ctrl+c or do the exit event/log off event, it will be handled on the top of this code
       clean up WSA -> exit */
    printf(COLOR_CYAN "V1 Dev note: This was pretty hard to make LOL. im getting used to it slowly. this thing is my first project in C so dont bully if its not up to your standard :p\n" COLOR_RESET);
    check_sus();
    printf("Enter command 'mist su' to go superuser\n");
    printf("\n");
    char input[1024];
    
    while (1) {
        if (windcmd) {
            printf("Notice: windcmd is switched on");
        }
        if (super_user == true) {
            printf("SU>> ");
        }
        else {
            printf(">> ");
        }
        fflush(stdout);
                         // fgets return NULL when it fails. it doesnt return ints
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("CShell experienced an error...\n");
            return 0;
        }
        
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "exit") == 0 || strcmp(input, "Exit") ==0) {
            printf("Exitting...\n");
            break;
        }
        if (input[0] == '\0') {
            continue; // if the first char is \0, it means input was empty...
        }
        // windcmd exit handling
        if (windcmd) {
            if (strcmp(input, "mist windcmd") == 0) {
                // pass it onto cshell command handler
            }
            else {
            system(input);
            continue;
            }
        }
        // ----
        char *command = get_command(input);
        Command *cmd = find_command(command);
        const char *rest = strtok(NULL, "");
        // we get the keyword, then connect that keyword to the function of that keyword
        
        if (cmd == NULL) {
            printf("That command (%s) does not exist!\n", command); // command is the keyword, cmd is the function 
        }
        else {
            cmd->func(rest);
        }
    }

    #ifdef _WIN32
    WSACleanup();
    #endif
    return 0;
}