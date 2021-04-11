#ifndef COMMAND_H
#define COMMAND_H

#include <unistd.h>
#include <stdbool.h>
#include "astree.h"

/*
** CommandInternal:
** execute_simple_command() などで利用される
** ひとつのコマンドを実行するための周辺情報をまとめて保持する
*/
struct CommandInternal
{
	int argc; /* 引数の数 */
	char **argv; /* コマンドライン引数の文字列を保持するダブルポインタ */
	bool stdin_pipe; /* 入力の指定ディスクリプタがあるか */
	bool stdout_pipe; /* 出力の指定ディスクリプタがあるか */
	int pipe_read; /* 入力ディスクリプタ番号 */
	int pipe_write; /* 出力ディスクリプタ番号 */
	char* redirect_in; /* 入力になるファイル名 */
	char* redirect_out; /* 出力先のファイル名 */
	bool asynchrnous; /* 同期的実行か、非同期的実行かの真偽値 */
};

typedef struct CommandInternal CommandInternal;

void set_prompt(char* str);
char* getprompt();
void ignore_signal_for_shell();
void execute_cd(CommandInternal* cmdinternal);
void execute_prompt(CommandInternal* cmdinternal);
void execute_pwd(CommandInternal* cmdinternal);
void execute_command_internal(CommandInternal* cmdinternal);
int init_command_internal(ASTreeNode* simplecmdNode, 
						  CommandInternal* cmdinternal, 
						  bool async,
						  bool stdin_pipe,
						  bool stdout_pipe,
						  int pipe_read,
						  int pipe_write,
						  char* redirect_in,
						  char* redirect_out
);
void destroy_command_internal(CommandInternal* cmdinternal);

#endif
