#ifndef COMMANDS_H
#define COMMANDS_H
#include <stdbool.h>
// platform stuff
#ifdef _WIN32
    #include <direct.h> // i know these weird things dont need indentation. its just so weird without indentation to me (the python-poisoned mind LOL)
    #define getcwd _getcwd
    #define chdir _chdir
    #define mkdir(path) _mkdir(path)
    #include <windows.h>
    void enable_ansi(void);
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #define mkdir(path) mkdir(path, 0755)
#endif

// shared types
typedef void (*command_func)(const char *args);
typedef struct {
    const char *name;
    command_func func;
    const char *usage;
    const char *desc;
} Command;
// shared globals (defined in main.c, used in commands.c)
extern bool super_user;
extern bool windcmd;
extern char password[64];
extern const char *p;
extern Command commands[];
extern const size_t command_count;
// command function prototypes
void hello(const char *arg);
void Goodbye(const char *arg);
void calc(const char *arg);
void print_cwd(const char *arg);
void clear_screen(const char *arg);
void cd(const char *arg);
void mkfile(const char *arg);
void cat(const char *arg);
void make_directory(const char *arg);
void ldir(const char *arg);
void help(const char *arg);
void echo(const char *arg);
void mist(const char *arg);
void netfo(const char *arg);
void ping(const char *arg);
void download(const char *arg);
void games(const char *arg);
void get_exe_dir(char *out, size_t out_size);
void handle_su_cmds(const char *arg);
#endif