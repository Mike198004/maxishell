#include "maxishell.h"

// список процессов
procs p = {
    .fg = {
        .pid = -1,
        .finished = 1
    },
    .bg = NULL,
    .cursor = 0,
    .capacity = 0
};

// установка текущего процесса
void set_fgproc(pid_t pid) {
    p.fg.pid = pid;
    p.fg.finished = 0;
}

// добавить фоновый процесс в список
int add_bgproc(pid_t pid, char *name) {
    bg_proc* bp;

    // если закончилось место под элементы массива фоновых процессов, увеличить
    if (p.cursor >= p.capacity) {
        p.capacity = p.capacity * 2 + 1;
        p.bg = (bg_proc*)realloc(p.bg, sizeof(bg_proc)*p.capacity);
        if (p.bg == NULL) {
            fprintf(stderr, "Couldn't reallocate buffer for background procs!\n");
            return -1;
        }
    }
    // вывести инфо в консоль, что процесс запущен
    fprintf(stderr, "[%zu] started.\n", p.cursor);
    // сохранить процесс во времянную переменную
    bp = &p.bg[p.cursor];

    // сохранить инфо о процессе
    bp->pid = pid;
    bp->finished = 0;

    time_t time_stamp = time(NULL);
    bp->time_stamp = ctime(&time_stamp);
    bp->cmd = strdup(name);
    p.cursor += 1;

    return 0;
}

// прибить текущий процесс
void kill_fgproc() {
    if (p.fg.pid != -1) {
        // убить процесс
        kill(p.fg.pid, SIGTERM);
        // установить флаг завершения
        p.fg.finished = 1;
        fprintf(stderr, "\n");
    }
}

// завершить процесс
int term_proc(char **args) {
    char *idx_arg;      // курсор в аргументе
    int proc_index = 0; // сконвертированный в int тип

    if (args[1] == NULL) {
        fprintf(stderr, "No proc index to stop!\n");
    } else {
        // установить курсор в индекс аргумента (args[0] сам процесс)
        idx_arg = args[1];
        // Convert string index arg to int
        while ((*idx_arg >= '0') && (*idx_arg <= '9')) {
            proc_index = (proc_index*10)+((*idx_arg)-'0');
            // передвинуть курсор правей
            idx_arg += 1; // may inc? ++
        };

        // убить процесс, если индекс нормальный и не завершен
        if ((*idx_arg != '\0') || (proc_index >= p.cursor)) {
            fprintf(stderr, "Incorrect background proc index!\n");
        } else if (!p.bg[proc_index].finished) {
            kill(p.bg[proc_index].pid, SIGTERM);
        }
    }

    return 1;
}

// является ли процесс фоновым
int is_bgproc(char **args) {
    // текущее положение в массиве
    int last_arg = 0;

    // поиск последнего аргумента в массиве
    while (args[last_arg+1] != NULL) {
        last_arg += 1;
    };

    // проверка, яляется ли последний процесс фоновым (&)
    if (strcmp(args[last_arg], "&") == 0) {
        // удалить амперсанд (&) в конце строки
        args[last_arg] = NULL;
        return 1;
    }

    // амперсанд не найден
    return 0;
}

// запуск нового процесса
int maxishell_run(char **args) 
{
    pid_t pid;		// Fork процесс id
    int bg_flag;	// флаг фонового процесса

    bg_flag = is_bgproc(args);
    // создание child процесса
    pid = fork();
    if (pid < 0)
        fprintf(stderr, "Couldn't create child process!\n");
    // child процесс
    else if (pid == 0) {
        // попытка запуска процесса
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "Couldn't execute unknown cmd!\n");
        }
        exit(1);
    }
    // родительский процесс
    else {
        if (bg_flag) {
            // пытаемся добавить фоновый процесс в массив
            if (add_bgproc(pid, args[0]) == -1) {
                // прибить все процессы и выйти
                maxishell_quit();
            }
        } else {
            // установить текущий процесс
            set_fgproc(pid);
            // 
            if (waitpid(pid, NULL, 0) == -1) {
                // выдать ошибку, если не возможно завершить процесс
                if (errno != EINTR) {
                    fprintf(stderr, "Couldn't track the completion of the process!\n");
                }
            }
        }
    }

    return 1;
}

// сменить текущую директорию
int maxishell_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "Expected an arg for \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            fprintf(stderr, "Error change directory to \"%s\"!\n", args[1]);
        }
    }

    return 1;
}

// очистка консоли
int maxishell_clear(char **args)
{
    fprintf(stderr, "\033[0d\033[2J");
    return 1;
}

int maxishell_help(char **args)
{
    fprintf(stderr, "\33[37;1m MaxiShell v. 1.0 Help \33[0m\n");
    fprintf(stderr, "\33[35;1m help - helping for this commands;\n");
    fprintf(stderr, " clear - clearing console;\n");
    fprintf(stderr, " ver - display the version of Linux;\n");
    fprintf(stderr, " bg - display the list of background processes;\n");
    fprintf(stderr, " exit - Quit from shell\n");
    fprintf(stderr, " term [index] - kill the index process;\33[0m\n");
    fprintf(stderr, "\33[37;1m Please, use the man command for help; \33[0m\n");

    return 1;
}

// вывод версии дистрибутива Linux
int maxishell_ver(char **args)
{
    FILE *fd;
    char linux_dist[128];

    memset(linux_dist, 0, sizeof(linux_dist));
    fd = fopen("/etc/issue", "r");
    if (!fd) {
        fprintf(stderr, "Error opening file /etc/issue!\n");
        return 0;
    }
    
    if (fgets(linux_dist, 128, fd)) {
        fprintf(stderr, "\33[37;1m %s \33[0m\n", linux_dist);
    }

    return 1;
}

// завершить процессы и оболочку
int maxishell_quit() {
    bg_proc *bp;

    // отключить лог для child процесса
    signal(SIGCHLD, SIG_IGN);
    // убить текущий процесс
    if (!p.fg.finished) {
        kill_fgproc();
    }

    // заувершить все фоновые процессы
    for (size_t i = 0;i < p.cursor;i++) {
        bp = &p.bg[i];
        // если процесс активен, то завершить его
        if (!bp->finished) {
            kill(bp->pid, SIGTERM);
        }
        // освободить память для cmd
        free(bp->cmd);
    }

    return 0;
}

// вывод списка фоновых задач
int maxishell_bglist(char **args) {
    bg_proc *bp;

    for (size_t i = 0;i < p.cursor;i++) {
        bp = &p.bg[i];
        // вывести инфо о процессе
        fprintf(stderr,
            "[%zu]%s cmd: %s%s;%s pid: %s%d; %s"
            "state: %s%s;%s timestamp: %s%s", i,
            SECONDARY_COLOR, RESET_COLOR, bp->cmd,
            SECONDARY_COLOR, RESET_COLOR, bp->pid,
            SECONDARY_COLOR, RESET_COLOR, bp->finished ? "finished" : "active",
            SECONDARY_COLOR, RESET_COLOR, bp->time_stamp);
    }

    return 1;
}

// запуск команд
int maxishell_exec(char **args)
{
    fprintf(stderr, "cmd = %s\n", args[0]);
    
    // проверка на встроенные команды
    if (strcmp(args[0], "help") == 0)
	return (maxishell_help(args));
    if (strcmp(args[0], "ver") == 0)
	return (maxishell_ver(args));
    if (strcmp(args[0], "term") == 0)
	return (term_proc(args));
    if (strcmp(args[0], "clear") == 0)
	return (maxishell_clear(args));
    if (strcmp(args[0], "bg") == 0)
	return (maxishell_bglist(args));
    if (strcmp(args[0], "cd") == 0)
	return (maxishell_cd(args));
    if (strcmp(args[0], "exit") == 0)
	return (maxishell_quit(args));
    // выполнение внешних команд
    return (maxishell_run(args));
}

// поддерживает '\' для продолжения ввода аргументов на следующей строке
char* maxishell_readline() {
    char *in_line;
    ssize_t buf_size = CMD_BUFSIZE;
    int pos = 0;
    int ch;
    
    in_line = (char *)malloc(CMD_BUFSIZE);
    if (!in_line) {
	fprintf(stderr, "Error allocating mem for cmd!");
	return NULL;
    }

    // читаем stdin
    while(1) {
	ch = getchar();
	
	// проверка на спецсимволы
	if (ch == '\\') {	// если встречается продолжение ввода, то переход на новую строку
	    fprintf(stderr, "next arg please\n");
	    in_line[pos] = 32;	// добавить в буфер новую пробел
	    pos++;
	} else {
	    if (ch != '\n') {	// если не новая строка запомнить символ
		in_line[pos] = ch;
		pos++;
	    } else if ((in_line[pos-1] != 32) && (ch == '\n') && (strlen(in_line) > 0)) {
		fprintf(stderr, "enter!\n");
		in_line[pos] = '\0';
		break;
	    } else if (strlen(in_line) == 0) {
		//fprintf(stderr, "cmd is null!\n");
		free(in_line);
		return NULL;
	    }
	}
//	pos++;
    };
    
    if (in_line[pos - 1] == '\n') {
        in_line[pos - 1] = '\0';
    }

    return (in_line);
}

// разбор командной строки
char **maxishell_splitcmd(char *line)
{
    size_t buf_size = MAXISHELL_TOK_BUFSIZE;
    size_t pos = 0;
    char **split_tokens = NULL;
    char* token;

    //fprintf(stderr, "cmd line: %s", line);
    // выделить память под массив аргументов
    split_tokens = (char **)malloc(sizeof(char*)*buf_size);
    if (split_tokens == NULL) {
        fprintf(stderr, "Error allocating mem for tokens!");
        return NULL;
    }

    // цикл разбора строки аргументов
    token = strtok(line, MAXISHELL_TOK_DELIM);
    while (token != NULL) {
        split_tokens[pos++] = token;

        // если закончилось свободное место под массив, то выделить заново
        if (pos >= buf_size) {
            buf_size *= 2;
            split_tokens = (char **)realloc(split_tokens, buf_size*sizeof(char *));
            if (split_tokens == NULL) {
                fprintf(stderr, "Couldn't reallocate mem for arguments!\n");
                return NULL;
            }
        }
        // следующий разделитель
        token = strtok(NULL, MAXISHELL_TOK_DELIM);
    }
    split_tokens[pos] = NULL;

    return (split_tokens);
}

// срабатывает по завершению child процесса
void mark_ended_proc() {
    bg_proc *bp;

    // получить id процесса завершенного
    pid_t pid = waitpid(-1, NULL, 0);

    // обработка текущего процесса
    if (pid == p.fg.pid) {
        p.fg.finished = 1;	// установить флаг завершения
    }
    // обработка фонового процесса
    else {
        // поиск фонового процесса в массиве
        for(size_t i = 0;i < p.cursor;i++) {
            bp = &p.bg[i];
            if (bp->pid == pid) {
                printf("[%zu] finished.\n", i);
                // установить новое состояние процесса
                bp->finished = 1;
                break;
            }
        }
    }
}

// основной цикл
void maxishell_loop()
{
    uid_t uid;
    gid_t gid;
    
    char *cwd;	// текущая рабочая директория
    char *cmd;	// командная строка
    char **cmd_args = NULL;
    int status = 0;
    register struct passwd *pw;
    
    // выделение памяти
    cwd = (char *)malloc(PATH_MAX);
    if (!cwd) {
	fprintf(stderr, "Error allocating mem for CWD!\n");
        exit(EXIT_FAILURE);
    }

    // установить обработчик Ctrl-C
    signal(SIGINT, kill_fgproc);
    // установить обработчик окончания фонового процесса
    signal(SIGCHLD, mark_ended_proc);

    uid = geteuid();
    pw = getpwuid(uid);
    if (!pw)
        perror("getpwuid() can't find user!");

    gid = pw->pw_gid;
    do {
        if (getcwd(cwd, PATH_MAX) != NULL) {
            fprintf(stderr, "[%s:\33[37;1m %s \33[0m $] ", cwd, pw->pw_name);
            cmd = maxishell_readline();
            // пустая строка
            if (!cmd) {
        	fprintf(stderr, "maxshell_loop(): Command line is NULL!\n");
        	//free(cwd);
        	//maxishell_quit();
        	status = 1;	// продолжить читать командную строку
        	//break;
    	    } else {
    		// разобрать строку
        	cmd_args = maxishell_splitcmd(cmd);
        	if (!cmd_args) {
            	    fprintf(stderr, "maxishell_splitcmd() error!\n");
            	    free(cmd);
            	    free(cwd);
            	    maxishell_quit();
            	    status = 0;
            	    break;
        	}
        	status = maxishell_exec(cmd_args);
		free(cmd);
		free(cmd_args);
    	    }
        } else {
            free(cwd);
            if (cmd)
                free(cmd);

            perror("getcwd() error");
        }
    //    free(cmd);
    //    free(cmd_args);
    } while (status);
}

// оновной блок программы
int main(int argc, char **argv, char **env)
{
    maxishell_loop();

    return 0;
}
