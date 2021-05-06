#include "command.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

char* prompt = NULL; /* 入力待ち受け時に表示する文字列の領域のポインタ */
bool signalset = false;
void   (*SIGINT_handler)(int);

/* 受け取った文字列をpromptに代入して、画面上に表示する準備をする */
void set_prompt(char* str)
{
    if (prompt != NULL)
        free(prompt);

    prompt = malloc(strlen(str) + 1);
    strcpy(prompt, str);
}

char* getprompt()
{
	return prompt;
}

/* ignore_signal_for_shell()で、myshell プロセスに対して送信されたシグナルのハンドラを設定している
** mysh プロセスに対するシグナルハンドラの設定が済んでいることを示すため、
** グローバル変数 signalsetを利用している
*/
void ignore_signal_for_shell()
{
	signalset = true;
	
	// ignore "Ctrl-C"
    SIGINT_handler = signal(SIGINT, SIG_IGN);

	// ignore "Ctrl-Z"
    signal(SIGTSTP, SIG_IGN);

	// ignore "Ctrl-\"
    signal(SIGQUIT, SIG_IGN);
}

/*
** ignore_sigint_in_childは、子プロセス(つまり、外部コマンドを実行するためにforkされたプロセス)へ
** SIGINTシグナルが送られたときのハンドラを設定している
** 分岐元のmysh プロセスは、ignore_sitnal_for_shell() 関数の実行により
** SIGINT シグナルを無視するように設定されているため、ここから派生したコマンド実行プロセスも
** そのままではSIGINTを無視してしまう
** そこで、signalsetフラグがtrue (== SIGINTを無視するように設定済み)の場合は、
** SIGINTのシグナルハンドラー関数を再設定している
*/
void restore_sigint_in_child()
{
	if (signalset)
		signal(SIGINT, SIGINT_handler);
}

// built-in command cd
void execute_cd(CommandInternal* cmdinternal)
{
    if (cmdinternal->argc == 1) {
		struct passwd *pw = getpwuid(getuid());
		const char *homedir = pw->pw_dir;
		chdir(homedir);
	}
    else if (cmdinternal->argc > 2)
        printf("cd: Too many arguments\n");
    else {
        if (chdir(cmdinternal->argv[1]) != 0)
            perror(cmdinternal->argv[1]);
    }
}

// built-in command prompt /* 組み込みコマンド prompt */
void execute_prompt(CommandInternal* cmdinternal)
{
    if (cmdinternal->argc == 1)
        printf("prompt: Please specify the prompt string\n");
    else if (cmdinternal->argc > 2)
        printf("prompt: Too many arguments\n");
    else {
        set_prompt(cmdinternal->argv[1]);
    }
}

// built-in command pwd /* 組み込みコマンド pwd */
void execute_pwd(CommandInternal* cmdinternal)
{
    pid_t pid;
    if((pid = fork()) == 0 ) {
        if (cmdinternal->redirect_out) {
            int fd = open(cmdinternal->redirect_out, O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror(cmdinternal->redirect_out);
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
        }

        if (cmdinternal->stdout_pipe)
			dup2(cmdinternal->pipe_write, STDOUT_FILENO);

        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            fprintf(stdout, "%s\n", cwd);
        else
            perror("getcwd() error");

        exit(0);
    }
    else if (pid < 0) {
        perror("fork");
        return;
    }
    else
		while (waitpid(pid, NULL, 0) <= 0);

    return;
}

void zombie_process_handler(int signum)
{
    int more = 1;        // more zombies to claim
    pid_t pid;           // pid of the zombie
    int status;          // termination status of the zombie

    while (more) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0)
            printf("\n%d terminated\n", pid);
        if (pid<=0)
            more = 0;
    }
}

void execute_command_internal(CommandInternal* cmdinternal)
{

    if (cmdinternal->argc < 0)
        return;

    // check for built-in commands /* 組み込みコマンドの実行 */
    if (strcmp(cmdinternal->argv[0], "cd") == 0) {
        execute_cd(cmdinternal);
        return;
    }
    else if (strcmp(cmdinternal->argv[0], "prompt") == 0) {
		execute_prompt(cmdinternal);
        return;
	}
    else if (strcmp(cmdinternal->argv[0], "pwd") == 0)
        return execute_pwd(cmdinternal);
    else if(strcmp(cmdinternal->argv[0], "exit") == 0) {
		exit(0);
		return;
	}

    pid_t pid;
    if((pid = fork()) == 0 ) {
		// restore the signals in the child process
        /* -> 子プロセスのシグナルを復元する */
		restore_sigint_in_child();
		
		// store the stdout file desc
        /* 出力先のファイルディスクリプタを格納 */
        int stdoutfd = dup(STDOUT_FILENO);

		// for bckgrnd jobs redirect stdin from /dev/null
        /* -> バックグラウンド処理のジョブの場合、標準入力をdev/null からリダイレクトする */
        if (cmdinternal->asynchrnous) {
            int fd = open("/dev/null", O_RDWR);
            if (fd == -1) {
                perror("/dev/null");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
        }

        // redirect stdin from file if specified
        /* -> ファイルディスクリプタが指定されていた場合、標準入力をそのファイルからリダイレクトする */
        if (cmdinternal->redirect_in) {
            int fd = open(cmdinternal->redirect_in, O_RDONLY);
            if (fd == -1) {
                perror(cmdinternal->redirect_in);
                exit(1);
            }

            dup2(fd, STDIN_FILENO);
        }

        // redirect stdout to file if specified
        /* -> ファイルの指定がある場合、標準出力をファイルにリダイレクト */
        else if (cmdinternal->redirect_out) {
            int fd = open(cmdinternal->redirect_out, O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror(cmdinternal->redirect_out);
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
        }

        // read stdin from pipe if present
        /* -> 標準入力があれば、パイプから読み込む */
        if (cmdinternal->stdin_pipe)
            dup2(cmdinternal->pipe_read, STDIN_FILENO);

		// write stdout to pipe if present
        /* -> 標準出力があれば、パイプに書き込む */
        if (cmdinternal->stdout_pipe)
            dup2(cmdinternal->pipe_write, STDOUT_FILENO);

        if (execvp(cmdinternal->argv[0], cmdinternal->argv) == -1) {
			// restore the stdout for displaying error message
            /* -> エラーメッセージを表示するための、標準出力の復元 */
            dup2(stdoutfd, STDOUT_FILENO);
			
            printf("Command not found: \'%s\'\n", cmdinternal->argv[0]);
			exit(1);
        }
        
    }
    else if (pid < 0) {
        perror("fork");
        return;
    }

    if (!cmdinternal->asynchrnous)
    {
        // wait till the process has not finished
        /* -> プロセスが終了していない状態で待つ */
        while (waitpid(pid, NULL, 0) <= 0);
    }
    else
    {
		// set the sigchild handler for the spawned process
        /* -> 起動したプロセスのシグナルIDのハンドラを設定する */
        printf("%d started\n", pid);
        struct sigaction act;
        act.sa_flags = 0;
        act.sa_handler = zombie_process_handler;
        sigfillset( & (act.sa_mask) ); // to block all

        if (sigaction(SIGCHLD, &act, NULL) != 0)
            perror("sigaction");
    }

    return;
}

/*
** init_command_internal():
** コマンド情報を取りまとめて設定する構造体 CommandInternal を、
** 引数の内容で設定する
** 実行コマンドの情報がすべて決定する execute_simple_command()で呼び出される
*/
int init_command_internal(ASTreeNode* simplecmdNode,
                          CommandInternal* cmdinternal,
                          bool async,
                          bool stdin_pipe,
                          bool stdout_pipe,
                          int pipe_read,
                          int pipe_write,
                          char* redirect_in,
                          char* redirect_out)
{
    /* simplecmdNode の値がNULLもしくはtypeがNODE_CMDPATHではない場合、エラー */
    if (simplecmdNode == NULL || !(NODETYPE(simplecmdNode->type) == NODE_CMDPATH))
    {
        cmdinternal->argc = 0;
        return -1;
    }

    /* 引数を取り出していくため、argNodeを設定 */
    ASTreeNode* argNode = simplecmdNode;

    int i = 0;
    /* 引数の数をカウントするため、右の枝にNODE_ARGUMENTかNODE_CMDPATHがつづく数を数える */
    while (argNode != NULL && (NODETYPE(argNode->type) == NODE_ARGUMENT || NODETYPE(argNode->type) == NODE_CMDPATH)) {
        argNode = argNode->right;
        i++;
    }

    /* カウントした分だけ、文字列ポインタを確保する。最後にNULLポインタをつけるので、数えた数よりひとつ多く確保しておく */
    cmdinternal->argv = (char**)malloc(sizeof(char*) * (i + 1));
    argNode = simplecmdNode; /* argNodeを巻き戻し */
    i = 0; /* 引数文字列の数をカウント。最後にargcになる */
    while (argNode != NULL && (NODETYPE(argNode->type) == NODE_ARGUMENT || NODETYPE(argNode->type) == NODE_CMDPATH)) {
        cmdinternal->argv[i] = (char*)malloc(strlen(argNode->szData) + 1);
        strcpy(cmdinternal->argv[i], argNode->szData); /* 各ノードの文字列データを複製する */

        argNode = argNode->right; /* 次のノードへ進む */
        i++;
    }

    cmdinternal->argv[i] = NULL; /* 引数文字列の末尾ポインタをNULLに設定 */
    cmdinternal->argc = i;

    /* 引数として渡された値をそのままcmdinternalに保存する */
    cmdinternal->asynchrnous = async;
    cmdinternal->stdin_pipe = stdin_pipe;
    cmdinternal->stdout_pipe = stdout_pipe;
    cmdinternal->pipe_read = pipe_read;
    cmdinternal->pipe_write = pipe_write;
    cmdinternal->redirect_in = redirect_in;
    cmdinternal->redirect_out = redirect_out;

    return 0;
}

/* 入力コマンド情報を破棄・解放(free)する */
void destroy_command_internal(CommandInternal* cmdinternal)
{
    int i;
    for (i = 0; i < cmdinternal->argc; i++)
        free(cmdinternal->argv[i]);

    free(cmdinternal->argv);
    cmdinternal->argc = 0;
}
