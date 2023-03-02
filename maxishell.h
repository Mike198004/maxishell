#ifndef __MAXISHELL_H__
#define __MAXISHELL_H__

#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>
#include <time.h>
#include <errno.h>

#define LOGIN_MAX_SIZE	64
#define CMD_BUFSIZE	4096 // one page

#define MAXISHELL_TOK_BUFSIZE 32
#define MAXISHELL_TOK_DELIM " \t\n\\"	// spaces, '\', tabes, newlines

#define PRIMARY_COLOR   "\033[92m"
#define SECONDARY_COLOR "\033[90m"
#define RESET_COLOR     "\033[0m"

// сруктура фонового процесса
typedef struct bg_proc_t {
    pid_t pid;		// id процесса
    char finished;	// состояние процесса
    char* time_stamp;	// время процесса
    char* cmd;		// команда с аргументами
} bg_proc;

// структура текущего процесса
typedef struct fg_proc_t {
    pid_t pid;
    char finished;
} fg_proc;

// структура со списком всех процессов
typedef struct procs_t {
    fg_proc fg;		// текущий id процесса
    bg_proc *bg;	// массив фоновых задач
    size_t cursor;	// позиция в массиве
    size_t capacity;	// размер массива
} procs;

void set_fgproc(pid_t);
int add_bgproc(pid_t, char *);
void kill_fgproc();
int term_proc(char **);
int is_bgproc(char **);
void mark_ended_proc();

int maxishell_run(char **);
int maxishell_cd(char **);
int maxishell_clear(char **);
int maxishell_help(char **);
int maxishell_ver(char **);
int maxishell_bglist(char **);
int maxishell_exec(char **);
char* maxishell_readline();
char** maxishell_splitcmd(char *);
void maxishell_loop();
int maxishell_quit();

#endif // __MAXISHELL_H__
