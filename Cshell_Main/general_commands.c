#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include "prototypes.h"
#ifdef _WIN32
#include <windows.h>

/* 
   
   General commands of CShell, all written here and registered in prototypes.h
   For more, you can add your own commands here. If they are more complex than just a simple chdir,
   its better to separate it from these commands. 
   
   8/20/2026 - Latest Update
   
*/
void enable_ansi(void) { // idk why this works exactly. im still new to this and this is weird
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#endif
void skip_ws(void) { while (*p == ' ') p++;}
double parse_expr(void);
double parse_factor(void) {
    skip_ws();
    if (*p == '(') {
        p++; // consume '('
        double val = parse_expr();
        skip_ws();
        if (*p == ')') p++; // consume ')'
        return val;
    }
    char *end;
    double val = strtod(p, &end);
    p = end;
    return val;
}

double parse_term(void) {
    double val = parse_factor();
    skip_ws();
    while (*p == '*' || *p == '/') {
        char op = *p++;
        double rhs = parse_factor();
        val = (op == '*') ? val * rhs : val / rhs;
        skip_ws();
    }
    return val;
}

double parse_expr(void) {
    double val = parse_term();
    skip_ws();
    while (*p == '+' || *p == '-') {
        char op = *p++;
        double rhs = parse_term();
        val = (op == '+') ? val + rhs : val - rhs;
        skip_ws();
    }
    return val;
}

void hello(const char *arg) {
    (void)arg;
    printf("Hello from C!\n");
}
void Goodbye(const char *arg) {
    (void)arg;
    printf("Bye!\n");
}
void echo(const char *arg) {
    if (arg != NULL) {
        printf("%s\n", arg);
    }
}
//main interpreter system tracker (mist) (idk i made it up on the spot)
void mist(const char *arg) {
    if (arg == NULL) {
        printf("'Mist help' can help\n");
        return;
    }
         // strcmp compares string
         // 0 when they match
    if (strcmp(arg, "windcmd") == 0) {
        if (windcmd) {
            printf("windcmd is now off\n");
            windcmd=false;
            return;
        }
        else {
            printf("windcmd is now on\n");
            windcmd=true;
            return;
        }
    }
    
    if (strcmp(arg, "help") == 0) {
        printf("Mist contains the following commands:\n");
        printf("Mist su - SuperUser mode\n");
        printf("Mist name / Mist username - Displays your current username\n");
        printf("Mist windcmd - switches to Windows Powershell, can run windows commands (run again to turn it off)\n");

    }
    else if (strcmp(arg, "username") == 0 || strcmp(arg, "name") == 0) {
        if (super_user) {
            printf("rootsu\n");
        }
        else {
            printf("user\n");
        }
    }
    else if (strcmp(arg, "su") == 0) {
    if (!super_user) {
        printf("Enter Super User code\n");
        printf("? >> ");
        char code[64];
        if (fgets(code, sizeof(code), stdin) == NULL) {
            printf("Didn't get that. Try again.\n");
            return;
        }
        code[strcspn(code, "\n")] = '\0'; //locate the position of \n and replace it with \0. Usefullll!!!!!!!!
         // note to self: strcmp is short for string compare
        if (strcmp(code, password) == 0) {
            printf("Super User = On\n");
            super_user = true;
            return;
        }
        else {
            printf("Wrong code. Not authorized.\n");
            return;
        }
    } else {
        printf("Switching super user mode off");
        super_user = false;
    }
    
    }
    else {
        printf("Mist does not contain that command.\n");
    }
}
void calc(const char *arg) {
    if (arg == NULL) { printf("calc: missing expression\n"); return; }
    p = arg;
    double result = parse_expr();
    printf("%g\n", result);
}
//
void print_cwd(const char *arg) {
    (void)arg;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } 
    else {
        printf("[!] PWD Failed\n");
    }
}
void clear_screen(const char *arg) {
    (void)arg;
    printf("\033[2J\033[H"); // \033[2J clears the screen and \033[H moves the cursor to the top and by cursor i mean it just takes the empty space out
    fflush(stdout);
}
void cd(const char *arg) {
    // no need to (void)arg because we need it
    if (arg == NULL || arg[0] == ' ' || arg[0] == '\0') {
        printf("[!] Invalid Argument\n");
        printf("Usage: cd <dir_name>\n");
        return; // stop it, because our conditions can't stop the cd
    }// chdir does CD, if it doesnt return 0, (meaning it didnt return 'True'), we'll raise an error
    if (chdir(arg) != 0) {
        perror("[!] Change Dir. experienced an error\n");
    }
}
void mkfile(const char *arg) {
    if (arg == NULL || arg[0] == ' ' || arg[0] == '\0') {
        printf("[!] Invalid Argument\n");
        printf("Usage: mkfile <file_name>\n");
        return;
    }           //named arg
    FILE *fp = fopen(arg, "a"); //append mode
    if (fp == NULL) {
        printf("[!] File creation failed\n");
        return;
    }
    fclose(fp);
}

// CAT HELPER: FORMAT CHECKER
int is_text_file(const unsigned char *buffer, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned char c = buffer[i];

        // allow common whitespace
        if (c == '\n' || c == '\r' || c == '\t') {
            continue;
        }

        // printable ASCII range
        if (c >= 0x20 && c <= 0x7E) {
            continue;
        }

        // anything else (especially null bytes, control chars) suggests binary
        return 0;
    }
    return 1;
}
int checkFileFormat(const char *filename) {
    unsigned char buffer[12]; // bumped up — some formats need more than 4 bytes to identify
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        perror("Error opening file");
        return 403; // 403: couldnt open file
    }
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if (bytesRead < 4) {
        printf("File is too small to determine format.\n");
        return 404; // 404: too small to check
    }

    // Images
    if (buffer[0] == 0x89 && buffer[1] == 'P' && buffer[2] == 'N' && buffer[3] == 'G') {
        printf("File format: PNG\n");
        return 1;
    }
    if (buffer[0] == 0xFF && buffer[1] == 0xD8) {
        printf("File format: JPEG\n");
        return 2;
    }
    if (buffer[0] == 'B' && buffer[1] == 'M') {
        printf("File format: BMP\n");
        return 3;
    }
    if (bytesRead >= 6 && buffer[0] == 'G' && buffer[1] == 'I' && buffer[2] == 'F' &&
        buffer[3] == '8' && (buffer[4] == '7' || buffer[4] == '9') && buffer[5] == 'a') {
        printf("File format: GIF\n");
        return 4;
    }
    if (bytesRead >= 12 && buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F' &&
        buffer[8] == 'W' && buffer[9] == 'E' && buffer[10] == 'B' && buffer[11] == 'P') {
        printf("File format: WEBP\n");
        return 5;
    }

    // Archives
    if (buffer[0] == 'P' && buffer[1] == 'K' && (buffer[2] == 0x03 || buffer[2] == 0x05 || buffer[2] == 0x07)) {
        printf("File format: ZIP (also used by .docx, .xlsx, .jar, etc.)\n");
        return 10;
    }
    if (buffer[0] == 0x1F && buffer[1] == 0x8B) {
        printf("File format: GZIP\n");
        return 11;
    }
    if (bytesRead >= 6 && buffer[0] == '7' && buffer[1] == 'z' && buffer[2] == 0xBC &&
        buffer[3] == 0xAF && buffer[4] == 0x27 && buffer[5] == 0x1C) {
        printf("File format: 7-Zip\n");
        return 12;
    }
    if (buffer[0] == 'R' && buffer[1] == 'a' && buffer[2] == 'r' && buffer[3] == '!') {
        printf("File format: RAR\n");
        return 13;
    }

    // Documents
    if (buffer[0] == '%' && buffer[1] == 'P' && buffer[2] == 'D' && buffer[3] == 'F') {
        printf("File format: PDF\n");
        return 20;
    }

    // Executables
    if (buffer[0] == 'M' && buffer[1] == 'Z') {
        printf("File format: Windows executable (EXE/DLL)\n");
        return 30;
    }
    if (buffer[0] == 0x7F && buffer[1] == 'E' && buffer[2] == 'L' && buffer[3] == 'F') {
        printf("File format: Linux executable (ELF)\n");
        return 31;
    }

    // Audio
    if (bytesRead >= 3 && buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3') {
        printf("File format: MP3 (ID3 tag)\n");
        return 40;
    }
    if (buffer[0] == 0xFF && (buffer[1] & 0xE0) == 0xE0) {
        printf("File format: MP3 (raw frame header)\n");
        return 41;
    }
    if (bytesRead >= 12 && buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F' &&
        buffer[8] == 'W' && buffer[9] == 'A' && buffer[10] == 'V' && buffer[11] == 'E') {
        printf("File format: WAV\n");
        return 42; // 42: WAV
    }
    if (is_text_file(buffer, bytesRead)) {
        printf("File format: TXT\n");
        return 43; // txt
    }

    printf("Unknown file format.\n");
    return 0; // 0: unknown file
}
void cat(const char *arg) {
    if (arg == NULL || arg[0] == ' ' || arg[0] == '\0') {
        printf("[!] Invalid Argument\n");
        printf("Usage: cat <file_name>\n");
        return;
    }
    if (strcmp(arg, "super_user.txt") == 0) {
        if (!super_user) {
            printf("You cannot read super_user.txt as a normu (Normal User)\n");
            return;
        }
    }
    if (strcmp(arg, "su_commands.c") == 0) {
        if (!super_user) {
            printf("You cannot read su_commands.c as a normu (Normal User)\n");
            return;
        }
    }
    // we gotta check the format
    if ((checkFileFormat(arg)) !=43) {
        printf("File format is not TXT, output might appear as gibberish\n");
    }
    // we'll also use magic numbers to check a file's format

    FILE *fp = fopen(arg, "r");
    if (fp == NULL) {
        printf("[!] File-fetch failed\n");
        return;
    }
    // if you reached here, youre good so far
    // file emptiness check:
    int empty_check;           //end of file
    if ((empty_check = fgetc(fp)) == EOF) {
        printf("File is empty.\n");
    }
    else {
        rewind(fp); // go back so that the first character isnt skipped with the emptiness check
        int c; // while we aren't at the end of the file
        printf("Contents of the file:\n");
        while ((c = fgetc(fp)) != EOF) {
            putchar(c);
        }
    }
    printf("\n"); // after the contents are pasted, a new line between the cursor of the terminal and the contents
    fclose(fp);
}
void make_directory(const char *arg) {
    // again, check for arg being invalid
    if (arg == NULL || arg[0] == ' ' || arg[0] == '\0') {
        printf("[!] Invalid Argument\n");
        printf("Usage: mkdir <directory_name>\n");
        return; // again... return and stop the whole thing
    }
    if (mkdir(arg) != 0) {
        perror("[!] Make Dir. experienced an error\n");
    }
}
void ldir(const char *arg) {    // if there's an entry arg
    const char *path = (arg != NULL && arg[0] != '\0' && arg[0] != ' ') ? arg : ".";
    // we check if there's entry. if so, we put it as the path, if not, path is "."
    DIR *d = opendir(path);
    // opendir doesn't return int so we cant do if (DIR *d = opendir(path)) { success };
    if (d == NULL) {
        printf("[!] Couldn't fetch that directory\n");
        return;
    }
    struct dirent *entry; // uhhhhh
    while ((entry = readdir(d)) != NULL) {
        printf("%s\n", entry->d_name); //uhm
    }
    closedir(d);
    // no need to return anything cuz the function's over anyway. and its void
}
void help(const char *arg) {
    (void)arg;
    printf("Here are the commands of CShell\n");

    for (size_t i = 0; i < command_count; i++) {
        printf("[%zu] %-8s %s - usage: %s\n",
               i + 1, commands[i].name, commands[i].desc, commands[i].usage);
    }
}

extern void get_cpu_vendor(char *out);

void cpuinfo(const char *arg) {
    (void)arg;

    char vendor[13];
    get_cpu_vendor(vendor);

    printf("CPU Vendor: %s\n", vendor);
}