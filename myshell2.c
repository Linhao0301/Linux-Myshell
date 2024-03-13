/* 头文件 */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>             // readdir函数
#include <readline/readline.h>   //readline库封装键盘操作
#include <readline/history.h>

/* 宏定义 */
#define IN 1
#define OUT 0
#define MAX_CMD 10
#define BUFFSIZE 100
#define MAX_CMD_LEN 100


/* 全局变量 */
int argc;                                       // 有效参数个数
char* argv[MAX_CMD];                            // 参数指针数组
char command[MAX_CMD][MAX_CMD_LEN];             // 参数数组
// 例如ls -a -l 那么argc为3，
// command[0]是ls，command[1]是-a，
// command[0][0]是l，command[0][1]是s
// argv[0]是指向command[0]的指针，argv【0】就是ls，argv【1】就是-a
//argc就是3
char buf[BUFFSIZE];                             // 接受输入的完整字符串数组 
char backupBuf[BUFFSIZE];                       // 完整字符串的备份
char curPath[BUFFSIZE];                         // 当前shell工作路径
int i, j;                                       // 循环变量
int commandNum;                                 // 已经输入指令数目
char prevPath[BUFFSIZE] = "";                   //存储上一个工作目录


int get_input(char buf[]);                      // 输入指令并存入buf数组
void parse(char* buf);                          // 解析字符串
void do_cmd(int argc, char* argv[]);

// 重定向指令
int commandWithOutputRedi(char buf[BUFFSIZE]);          // 执行输出重定向
int commandWithInputRedi(char buf[BUFFSIZE]);           // 执行输入重定向命令
int commandWithReOutputRedi(char buf[BUFFSIZE]);        // 执行输出重定向追加写
int commandWithPipe(char buf[BUFFSIZE]);                // 执行管道命令
int commandInBackground(char buf[BUFFSIZE]);



//《《《内置指令相关 
typedef int (*cmd_handler)(int argc, char* argv[]);     // 内置命令处理函数类型          
int handle_cd(int argc, char* argv[]);                  // 执行cd指令
int handle_history(int argc, char* argv[]);             // 打印历史指令
int handle_exit(int argc, char* argv[]);
int handle_top(int argc, char* argv[]);
                                                        
typedef struct {                                    
    char* cmd;
    cmd_handler handler;
} cmd_map;                                              // 命令和函数的映射结构
cmd_map commands[] = {                                  // 命令映射表 注意区分command[][]和commands[],
    {"cd", handle_cd},
    {"history", handle_history},
    {"exit",handle_exit},
    {"top",handle_top},             //状态信息
                              
    // ... 其他命令
    {NULL, NULL} // 结尾哨兵
};  
// 》》》内置指令相关


//《《《外部指令相关
int execute_external_command(char* argv[]); 
#define MY_COMMANDS_DIR "./mycommands/"
// 》》》外部指令相关



// 《处理输入和指令判定，调用指令区
int main() {
    // while循环
    while(1) {
        
        // 输入字符存入buf数组, 如果输入字符数为0, 则跳过此次循环
        if (get_input(buf) == 0)
            continue;
        // strcpy(history[commandNum++], buf);
        strcpy(backupBuf, buf);
        parse(buf);
        do_cmd(argc, argv);
        argc = 0;
    }
}

int get_input(char *buf) {
    char *input = readline("[myshell]$ ");
    if (!input) {
        return 0;  // EOF or error
    }

    // 添加到历史记录中
    add_history(input);

    // 将输入复制到 buf 中
    strncpy(buf, input, BUFFSIZE);
    buf[BUFFSIZE - 1] = '\0';  // 确保 buf 结尾的 '\0'

    int len = strlen(buf);
    free(input);  // 释放 readline 分配的内存
    return len;
}

// 处理输入的语法，分别填写argv argc
void parse(char* buf) {
    // 初始化argv数组和argc
    for (i = 0; i < MAX_CMD; i++) {
        argv[i] = NULL;
        for (j = 0; j < MAX_CMD_LEN; j++)
            command[i][j] = '\0';
    }
    argc = 0;
    // 下列操作改变了buf数组, 为buf数组做个备份
    strcpy(backupBuf, buf);
    /** 构建command数组
     *  即若输入为"ls -a"
     *  strcmp(command[0], "ls") == 0 成立且
     *  strcmp(command[1], "-a") == 0 成立
     */  
    int len = strlen(buf);
    for (i = 0, j = 0; i < len; ++i) {
        if (buf[i] != ' ') {
            command[argc][j++] = buf[i];
        } else {
            if (j != 0) {
                command[argc][j] = '\0';
                ++argc;
                j = 0;
            }
        }
    }
    if (j != 0) {
        command[argc][j] = '\0';
    }

    /** 构建argv数组
     *  即若输入buf为"ls -a"
     *  strcmp(argv[0], "ls") == 0 成立且
     *  strcmp(argv[1], "-a") == 0 成立*/
    argc = 0;
    int flg = OUT;
    for (i = 0; buf[i] != '\0'; i++) {
        if (flg == OUT && !isspace(buf[i])) {
            flg = IN;
            argv[argc++] = buf + i;
        } else if (flg == IN && isspace(buf[i])) {
            flg = OUT;
            buf[i] = '\0';
        }
    }
    argv[argc] = NULL;
}

void do_cmd(int argc, char* argv[]) {
   
    /* 识别program命令 */
    // 识别重定向输出命令
    for (j = 0; j < MAX_CMD; j++) {
        if (strcmp(command[j], ">") == 0) {
            strcpy(buf, backupBuf);
            int sample = commandWithOutputRedi(buf);
            return;
        }
    }
    // 识别输入重定向
    for (j = 0; j < MAX_CMD; j++) {
        if (strcmp(command[i], "<") == 0) {
            strcpy(buf, backupBuf);
            int sample = commandWithInputRedi(buf);
            return;
        }
    }
    // 识别追加写重定向
    for (j = 0; j < MAX_CMD; j++) {
        if (strcmp(command[j], ">>") == 0) {
            strcpy(buf, backupBuf);
            int sample = commandWithReOutputRedi(buf);
            return;
        }
    }

    // 识别管道命令
    for (j = 0; j < MAX_CMD; j++) {
        if (strcmp(command[j], "|") == 0) {
            strcpy(buf, backupBuf);
            int sample = commandWithPipe(buf);
            return;
        }
    }

    // 识别后台运行命令
    for (j = 0; j < MAX_CMD; j++) {
        if (strcmp(command[j], "&") == 0) {
            strcpy(buf, backupBuf);
            int sample = commandInBackground(buf);
            return;
        }
    }

    //处理内部指令
    for (int i = 0; commands[i].cmd != NULL; i++) {
        if (strcmp(argv[0], commands[i].cmd) == 0) {
            commands[i].handler(argc, argv);
            return; // 只是返回，不带任何返回值
        }
    }
    // 处理外部命令
    int result = execute_external_command(argv);
    if (result != 0) {
        // 输出错误信息
        printf("命令 '%s' 执行失败，错误码: %d\n", argv[0], result);
    }

}

int execute_external_command(char* argv[]) {
    char command_path[1024];

    // 构造在自定义目录下的完整命令路径
    snprintf(command_path, sizeof(command_path), "%s%s", MY_COMMANDS_DIR, argv[0]);

    // 创建子进程
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // 子进程：替换成完整路径的命令
        argv[0] = command_path; // 将完整路径设置为第一个参数
        execv(command_path, argv);
        perror("execv");
        exit(EXIT_FAILURE);  // 如果execv失败
    } else {
        // 父进程：等待子进程结束
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            return exit_status;
        }
    }
    return 0;
}
// 》处理输入和指令判定，调用指令区



//《内置指令函数区
int handle_cd(int argc, char* argv[]) {
    static char prevPath[BUFFSIZE] = "";  // 用于存储上一个工作目录的路径
    char curPath[BUFFSIZE];  // 用于存储当前工作目录的路径

    // 获取当前工作目录
    if (getcwd(curPath, sizeof(curPath)) == NULL) {
        perror("getcwd");
        return 0; // 获取当前工作目录失败
    }

    if (argc == 2 && strcmp(argv[1], "-") == 0) {
        // 如果输入是 'cd -'
        if (strlen(prevPath) == 0) {
            printf("没有记录的上一个工作目录\n");
            return 0; // 如果没有记录的上一个目录
        }

        // 切换到上一个工作目录
        if (chdir(prevPath) != 0) {
            perror("chdir");
            return 0; // 执行失败
        }

        // 打印新的工作目录
        printf("移动到目录: %s\n", prevPath);

        // 更新上一个工作目录信息
        strcpy(prevPath, curPath);
        return 1; // 执行成功
    } else if (argc == 2) {
        // 处理正常的 'cd' 命令
        if (chdir(argv[1]) != 0) {
            perror("chdir");
            return 0; // 执行失败
        }

        // 更新上一个工作目录信息
        strcpy(prevPath, curPath);

        // 获取并打印新的工作目录
        if (getcwd(curPath, sizeof(curPath)) == NULL) {
            perror("getcwd");
            return 0; // 获取新工作目录失败
        }
        printf("移动到目录: %s\n", curPath);
        return 1; // 执行成功
    } else {
        printf("用法错误: cd <目录>\n");
        return 0; // 参数错误
    }
}

int handle_history(int argc, char* argv[]) {
    int history_count = 0;
    HIST_ENTRY **the_list = history_list();

    if (the_list) {
        while (the_list[history_count]) {
            ++history_count;
        }
    }

    int n = history_count; // 默认显示所有历史记录

    // 检查是否提供了参数，并且是有效数字
    if (argc > 1) {
        int requested = atoi(argv[1]);
        if (requested > 0 && requested < n) {
            n = requested;
        }
    }

    // 显示历史记录
    for (int i = history_count - n; i < history_count; i++) {
        printf("%d: %s\n", i + history_base, the_list[i]->line);
    }

    return 0;
}

int handle_exit(int argc, char* argv[]) {
    // 您可以根据需要处理退出前的清理工作
    // 例如，释放资源、保存状态、打印退出消息等

    // 如果需要，可以根据提供的参数设置退出状态
    int exit_status = 0;
    if (argc > 1) {
        exit_status = atoi(argv[1]);
    }

    // 打印退出消息（如果需要）
    printf("Exiting shell with status %d\n", exit_status);

    // 退出程序
    exit(exit_status);
}

int handle_top(int argc, char* argv[]){
    FILE *fp = NULL;                    // 文件指针
    char buff[255];

    /* 获取内容一: 
       总体内存大小，
       空闲内存大小，
       缓存大小。 */
    fp = fopen("/proc/meminfo", "r");   // 以只读方式打开meminfo文件
    fgets(buff, 255, (FILE*)fp);        // 读取meminfo文件内容进buff
    fclose(fp);

    // 获取 pagesize
    int i = 0, pagesize = 0;
    while (buff[i] != ' ') {
        pagesize = 10 * pagesize + buff[i] - 48;
        i++;
    }

    // 获取 页总数 total
    i++;
    int total = 0;
    while (buff[i] != ' ') {
        total = 10 * total + buff[i] - 48;
        i++;
    }

    // 获取空闲页数 free
    i++;
    int free = 0;
    while (buff[i] != ' ') {
        free = 10 * free + buff[i] - 48;
        i++;
    }

    // 获取最大页数量largest
    i++;
    int largest = 0;
    while (buff[i] != ' ') {
        largest = 10 * largest + buff[i] - 48;
        i++;
    }

    // 获取缓存页数量 cached
    i++;
    int cached = 0;
    while (buff[i] >= '0' && buff[i] <= '9') {
        cached = 10 * cached + buff[i] - 48;
        i++;
    }

    // 总体内存大小 = (pagesize * total) / 1024 单位 KB
    int totalMemory  = pagesize / 1024 * total;
    // 空闲内存大小 = pagesize * free) / 1024 单位 KB
    int freeMemory   = pagesize / 1024 * free;
    // 缓存大小    = (pagesize * cached) / 1024 单位 KB
    int cachedMemory = pagesize / 1024 * cached;

    printf("totalMemory  is %d KB\n", totalMemory);
    printf("freeMemory   is %d KB\n", freeMemory);
    printf("cachedMemory is %d KB\n", cachedMemory);

    /* 2. 获取内容2
        进程和任务数量
     */
    fp = fopen("/proc/kinfo", "r");     // 以只读方式打开meminfo文件
    memset(buff, 0x00, 255);            // 格式化buff字符串
    fgets(buff, 255, (FILE*)fp);        // 读取meminfo文件内容进buff
    fclose(fp);

    // 获取进程数量
    int processNumber = 0;
    i = 0;
    while (buff[i] != ' ') {
        processNumber = 10 * processNumber + buff[i] - 48;
        i++;
    }
    printf("processNumber = %d\n", processNumber);

    // 获取任务数量
    i++;
    int tasksNumber = 0;
    while (buff[i] >= '0' && buff[i] <= '9') {
        tasksNumber = 10 * tasksNumber + buff[i] - 48;
        i++;
    }
    printf("tasksNumber = %d\n", tasksNumber);


    // /* 3. 获取psinfo中的内容 */
    DIR *d;
    struct dirent *dir;
    d = opendir("/proc");
    int totalTicks = 0, freeTicks = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {                   // 遍历proc文件夹
            if (strcmp(dir->d_name, ".") != 0 && 
                strcmp(dir->d_name, "..") != 0) {
                    char path[255];
                    memset(path, 0x00, 255);
                    strcpy(path, "/proc/");
                    strcat(path, dir->d_name);          // 连接成为完成路径名
                    struct stat s;
                    if (stat (path, &s) == 0) {
                        if (S_ISDIR(s.st_mode)) {       // 判断为目录
                            strcat(path, "/psinfo");

                            FILE* fp = fopen(path, "r");
                            char buf[255];
                            memset(buf, 0x00, 255);
                            fgets(buf, 255, (FILE*)fp);
                            fclose(fp);

                            // 获取ticks和进程状态
                            int j = 0;
                            for (i = 0; i < 4;) {
                                for (j = 0; j < 255; j++) {
                                    if (i >= 4) break;
                                    if (buf[j] == ' ') i++;
                                }
                            }
                            // 循环结束, buf[j]为进程的状态, 共有S, W, R三种状态.
                            int k = j + 1;
                            for (i = 0; i < 3;) {               // 循环结束后k指向ticks位置
                                for (k = j + 1; k < 255; k++) {
                                    if (i >= 3) break;
                                    if (buf[k] == ' ') i++;
                                }
                            }
                            int processTick = 0;
                            while (buf[k] != ' ') {
                                processTick = 10 * processTick + buff[k] - 48;
                                k++;
                            }
                            totalTicks += processTick;
                            if (buf[j] != 'R') {
                                freeTicks += processTick;
                            }
                        }else continue;
                    }else continue;
                }
        }
    }
    printf("CPU states: %.2lf%% used,\t%.2lf%% idle",
           (double)((totalTicks - freeTicks) * 100) / (double)totalTicks,
           (double)(freeTicks * 100) / (double)totalTicks);
    return 0;
}

int handle_help(int argc, char* argv[]){
    // 不输入指定命令，则输出所有命令
    if (argc == 1){
        printf("This is myshell\n");
        printf("Here are some built in cmd:\n");
        for (int i = 0; i < sizeof(commands) / (sizeof(commands[0].cmd) + sizeof(commands[0].handler)) - 1; i++){
            printf("%s\n", commands[i].cmd);
        }

        printf("Try 'help <cmd>' for more information.\n");
    }
    // 查看指定命令的帮助 
    else if (argc == 2){
        // 查看cd命令的帮助手册
        if (strcmp("cd", argv[1]) == 0){
            printf("cd: Change the shell working directory.\n");
            printf("   Options:\n");
            printf("      'cd':     change the current directory to home\n");
            printf("    'cd ~':     same as 'cd'\n");
            printf("'cd [dir]':     change the current direcrtory to dir, the dir can be absolute path relative path\n");
            printf("   'cd ..':     change to the parent directory\n");
            printf("    'cd -':     change to the last directory you visited\n");
        }
        // 查看history命令的帮助手册
        else if (strcmp("history", argv[1]) == 0) {
            printf("history: Review the commands used earlier.\n");
            printf("      Options:\n");
            printf("    'history':     review the all commands\n");
            printf("'history [n]':     review the last n commands\n");
        }
        // 查看exit命令的帮助手册
        else if (strcmp("exit", argv[1]) == 0) {
            printf("exit: Exit the shell.\n");
            printf("  Options:\n");
            printf("   'exit':     exit the shell with status of the last command executed\n");
            printf("'exit [n]:     exit the shell with a status of n\n");
        }
        // 查看top命令的帮助手册
        else if (strcmp("top", argv[1]) == 0) {
            printf("top: Display Linux processes.\n");
            printf("   Options:\n");
            printf("  'top':     Display Linux processes and all related information\n");
        }
        // 查看help命令的帮助手册
        else if (strcmp("help", argv[1]) == 0) {
            printf("help: Display information about builtin commands.\n");
            printf("    Options:\n");
            printf("     'help':     display all builtin commands\n");
            printf("'help <cmd>:     display information of the given command\n'");
        }
        // 查看pwd命令的帮助手册
        else if (strcmp("pwd", argv[1]) == 0) {
            printf("pwd: Print the name of the current working directory.\n");
            printf("    Options:\n");
            printf("   'pwd -L':     print the value of $PWD if it names the current working directory\n");
            printf("   'pwd -P':     print the physical directory, without any symbolic links\n");
        }
    }
    
    return 0;
}

int handle_pwd(int argc, char* argv[]){
    if (getcwd(curPath, sizeof(curPath)) != NULL){
        printf("%s\n", curPath);
    } else {
        perror("getcwd() error");
    }
}

//》内置指令函数区


//输出重定向 
int commandWithOutputRedi(char buf[BUFFSIZE]) {
    strcpy(buf, backupBuf);
    char outFile[BUFFSIZE];
    memset(outFile, 0x00, BUFFSIZE);
    int RediNum = 0;
    for ( i = 0; i + 1 < strlen(buf); i++) {
        if (buf[i] == '>' && buf[i + 1] == ' ') {
            RediNum++;
            break;
        }
    }
    if (RediNum != 1) {
        printf("输出重定向指令输入有误!");
        return 0;
    }

    for (i = 0; i < argc; i++) {
        if (strcmp(command[i], ">") == 0) {
            if (i + 1 < argc) {
                strcpy(outFile, command[i + 1]);
            } else {
                printf("缺少输出文件!");
                return 0;
            }
        }
    }

    // 指令分割, outFile为输出文件, buf为重定向符号前的命令
    for (j = 0; j < strlen(buf); j++) {
        if (buf[j] == '>') {
            break;
        }
    }
    buf[j - 1] = '\0';
    buf[j] = '\0';
    // 分割指令后重新调用parse来解析指令
    parse(buf);
    pid_t pid;
    switch(pid = fork()) {
        case -1: {
            printf("创建子进程未成功");
            return 0;
        }
        // 处理子进程:
        case 0: {
            // 完成输出重定向
            int fd;
            fd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, 0777);//文件权限
            // 文件打开失败
            if (fd < 0) {
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);    //dup2 是一个系统调用，用于复制文件描述符。                                
            execvp(argv[0], argv);      //STDOUT_FILENO 是一个常量，代表标准输出（通常是屏幕）的文件描述符。
            if (fd != STDOUT_FILENO) {
                close(fd);
            }
            // 代码健壮性: 如果子进程未被成功执行, 则报错
            printf("%s: 命令输入错误\n", argv[0]);
            // exit函数终止当前进程, 括号内参数为1时, 会像操作系统报告该进程因异常而终止
            exit(1);
        }
        default: {
            int status;
            waitpid(pid, &status, 0);       // 等待子进程返回
            int err = WEXITSTATUS(status);  // 读取子进程的返回码
            if (err) { 
                printf("Error: %s\n", strerror(err));
            } 
        }                        
    }

}

//输入重定向 
int commandWithInputRedi(char buf[BUFFSIZE]) {
    strcpy(buf, backupBuf);
    char inFile[BUFFSIZE];
    memset(inFile, 0x00, BUFFSIZE);
    int RediNum = 0;
    for ( i = 0; i + 1< strlen(buf); i++) {
        if (buf[i] == '<' && buf[i + 1] == ' ') {
            RediNum++;
            break;
        }
    }
    if (RediNum != 1) {
        printf("输入重定向指令输入有误!");
        return 0;
    }

    for (i = 0; i < argc; i++) {
        if (strcmp(command[i], "<") == 0) {
            if (i + 1 < argc) {
                strcpy(inFile, command[i + 1]);
            } else {
                printf("缺少输入指令!");
                return 0;
            }
        }
    }

    // 指令分割, InFile为输出文件, buf为重定向符号前的命令
    for (j = 0; j < strlen(buf); j++) {
        if (buf[j] == '<') {
            break;
        }
    }
    buf[j - 1] = '\0';
    buf[j] = '\0';
    parse(buf);
    pid_t pid;
    switch(pid = fork()) {
        case -1: {
            printf("创建子进程未成功");
            return 0;
        }
        // 处理子进程:
        case 0: {
            // 完成输入重定向
            int fd;
            fd = open(inFile, O_RDONLY); //只是读取文件 无需设置权限位
            // 文件打开失败
            if (fd < 0) {
                exit(1);
            }
            dup2(fd, STDIN_FILENO);  
            execvp(argv[0], argv);
            if (fd != STDIN_FILENO) {
                close(fd);
            }
            // 代码健壮性: 如果子进程未被成功执行, 则报错
            printf("%s: 命令输入错误\n", argv[0]);
            // exit函数终止当前进程, 括号内参数为1时, 会像操作系统报告该进程因异常而终止
            exit(1);
        }
        default: {
            int status;
            waitpid(pid, &status, 0);       // 等待子进程返回
            int err = WEXITSTATUS(status);  // 读取子进程的返回码
            if (err) { 
                printf("Error: %s\n", strerror(err));
            } 
        }                        
    }

}

//重定向输出将追加到指定文件的末尾。
int commandWithReOutputRedi(char buf[BUFFSIZE]) {
    strcpy(buf, backupBuf);
    char reOutFile[BUFFSIZE];
    memset(reOutFile, 0x00, BUFFSIZE);
    int RediNum = 0;
    for ( i = 0; i + 2 < strlen(buf); i++) {
        if (buf[i] == '>' && buf[i + 1] == '>' && buf[i + 2] == ' ') {
            RediNum++;
            break;
        }
    }
    if (RediNum != 1) {
        printf("追加输出重定向指令输入有误!");
        return 0;
    }

    for (i = 0; i < argc; i++) {
        if (strcmp(command[i], ">>") == 0) {
            if (i + 1 < argc) {
                strcpy(reOutFile, command[i + 1]);
            } else {
                printf("缺少输出文件!");
                return 0;
            }
        }
    }

    // 指令分割, outFile为输出文件, buf为重定向符号前的命令
    for (j = 0; j + 2 < strlen(buf); j++) {
        if (buf[j] == '>' && buf[j + 1] == '>' 
            && buf[j + 2] == ' ') {
            break;
        }
    }
    buf[j - 1] = '\0';
    buf[j] = '\0';
    // 解析指令
    parse(buf);
    pid_t pid;
    switch(pid = fork()) {
        case -1: {
            printf("创建子进程未成功");
            return 0;
        }
        // 处理子进程:
        case 0: {
            // 完成输出重定向
            int fd;
            fd = open(reOutFile, O_WRONLY|O_APPEND|O_CREAT|O_APPEND, 0777);
            // 文件打开失败
            if (fd < 0) {
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);  
            execvp(argv[0], argv);
            if (fd != STDOUT_FILENO) {
                close(fd);
            }
            // 代码健壮性: 如果子进程未被成功执行, 则报错
            printf("%s: 命令输入错误\n", argv[0]);
            // exit函数终止当前进程, 括号内参数为1时, 会像操作系统报告该进程因异常而终止
            exit(1);
        }
        default: {
            int status;
            waitpid(pid, &status, 0);       // 等待子进程返回
            int err = WEXITSTATUS(status);  // 读取子进程的返回码
            if (err) { 
                printf("Error: %s\n", strerror(err));
            } 
        }                        
    }   
}


int commandWithPipe(char buf[BUFFSIZE]) {
    // 获取管道符号的位置索引
    for(j = 0; buf[j] != '\0'; j++) {
        if (buf[j] == '|')
            break;
    }

    // 分离指令, 将管道符号前后的指令存放在两个数组中
    // outputBuf存放管道前的命令, inputBuf存放管道后的命令
    char outputBuf[j];
    memset(outputBuf, 0x00, j);
    char inputBuf[strlen(buf) - j];
    memset(inputBuf, 0x00, strlen(buf) - j);
    for (i = 0; i < j - 1; i++) {
        outputBuf[i] = buf[i];
    }
    for (i = 0; i < strlen(buf) - j - 1; i++) {
        inputBuf[i] = buf[j + 2 + i];
    }


    int pd[2]; //创建一个有两个文件描述符的数组：pd[0] 用于读取，pd[1] 用于写入。
    pid_t pid;
    if (pipe(pd) < 0) {
        perror("pipe()");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork()");
        exit(1);
    }


    if (pid == 0) {                     // 子进程写管道
        close(pd[0]);                   // 关闭子进程的读端
        dup2(pd[1], STDOUT_FILENO);     // 将子进程的写端作为标准输出
        parse(outputBuf);
        execvp(argv[0], argv);
        if (pd[1] != STDOUT_FILENO) {
            close(pd[1]);
        }
    }else {                              // 父进程读管道
        /** 关键代码
         *  子进程写管道完毕后再执行父进程读管道, 所以需要用wait函数等待子进程返回后再操作
         */
        int status;
        waitpid(pid, &status, 0);       // 等待子进程返回
        int err = WEXITSTATUS(status);  // 读取子进程的返回码
        if (err) { 
            printf("Error: %s\n", strerror(err));
        }

        close(pd[1]);                    // 关闭父进程管道的写端
        dup2(pd[0], STDIN_FILENO);       // 管道读端读到的重定向为标准输入
        parse(inputBuf);
        execvp(argv[0], argv);
        if (pd[0] != STDIN_FILENO) {
            close(pd[0]);
        }       
    }

    return 1;
}


int commandInBackground(char buf[BUFFSIZE]) {
    char backgroundBuf[strlen(buf)];
    memset(backgroundBuf, 0x00, strlen(buf));
    for (i = 0; i < strlen(buf); i++) {
        backgroundBuf[i] = buf[i];
        if (buf[i] == '&') {
            backgroundBuf[i] = '\0';
            backgroundBuf[i - 1] = '\0';
            break;
        }
    }

    pid_t pid;
    pid = fork();
    if (pid < 0) {
        perror("fork()");
        exit(1);
    }

    if (pid == 0) {
        // 将stdin、stdout、stderr重定向到/dev/null
        freopen( "/dev/null", "w", stdout );
        freopen( "/dev/null", "r", stdin ); 
        signal(SIGCHLD,SIG_IGN);
        parse(backgroundBuf);
        execvp(argv[0], argv);
        printf("%s: 命令输入错误\n", argv[0]);
        // exit函数终止当前进程, 括号内参数为1时, 会像操作系统报告该进程因异常而终止
        exit(1);
    }else {
        exit(0);
    }
}





