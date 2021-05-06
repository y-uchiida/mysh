#include "command.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

/*
** execute_simple_command():
** コマンド実行の最小単位
** execute_commandから呼び出される
** execute_commandで、リダイレクトの記述は解釈されており、
** 引数にredirect_in や redirect_outの値として渡される
*/
void execute_simple_command(ASTreeNode* simple_cmd_node,
                             bool async,
                             bool stdin_pipe,
                             bool stdout_pipe,
                             int pipe_read,
                             int pipe_write,
                             char* redirect_in,
                             char* redirect_out
                            )
{

    // printf("\t - execute_simple_commnad here.\n");
    // printf("\t - NODE_TYPE(simple_cmd_node->type): %d\n", NODETYPE(simple_cmd_node->type));
    // printf("\t - simple_cmd_node->szData: %s\n", simple_cmd_node->szData);

    // if (simple_cmd_node->right != NULL)
    // {
    //     printf("\t - NODE_TYPE(simple_cmd_node->right->type): %d\n", NODETYPE(simple_cmd_node->right->type));
    //     printf("\t - simple_cmd_node->right->szData: %s\n", simple_cmd_node->right->szData);

    // }
    // printf("\n");

    CommandInternal cmdinternal;
    init_command_internal(simple_cmd_node, &cmdinternal, async, stdin_pipe, stdout_pipe,
                          pipe_read, pipe_write, redirect_in, redirect_out
                         );
	execute_command_internal(&cmdinternal);
	destroy_command_internal(&cmdinternal);
}

/*
** execute_command():
** 単一のコマンドの実行
** execute_jobから呼び出される
** パイプ文字を含んだ、入出力連携はexecute_jobにより解釈されて、
** commandには入出力先のディスクリプタを指定する形で情報が引き渡される
*/
void execute_command(ASTreeNode* cmdNode,
                      bool async, /* 実行の同期・非同期(';' と '&' に関係) */
                      bool stdin_pipe, /* 入力の指定ディスクリプタがあるか */
                      bool stdout_pipe, /* 出力の指定ディスクリプタがあるか */
                      int pipe_read,  /* 入力ディスクリプタ番号 */
                      int pipe_write) /* 出力ディスクリプタ番号 */
{
    if (cmdNode == NULL)
        return;

    // printf("\t - execute_command here.\n");
    // printf("\t - NODE_TYPE(cmdNode->type): %d\n", NODETYPE(cmdNode->type));
    // printf("\t - cmdNode->szData: %s\n", cmdNode->szData);
    // printf("\n");

    switch (NODETYPE(cmdNode->type)) /* NODETYPEにより分岐 */
    {
    case NODE_REDIRECT_IN:		// right side contains simple cmd node /* 右の枝に、入力となるファイルが指定されている場合 ( '<' ) */
        execute_simple_command(cmdNode->right,
                               async,
                               stdin_pipe,
                               stdout_pipe,
                               pipe_read,
                               pipe_write,
                               cmdNode->szData, NULL
                              );
        break;
    case NODE_REDIRECT_OUT:		// right side contains simple cmd node /* 右の枝に、出力先のファイルが指定されている場合 ( '>' ) */
        execute_simple_command(cmdNode->right,
                               async,
                               stdin_pipe,
                               stdout_pipe,
                               pipe_read,
                               pipe_write,
                               NULL, cmdNode->szData
                              );
        break;
    case NODE_CMDPATH: /* リダイレクト( '<' / '>' )が設定されていない場合 */
        execute_simple_command(cmdNode,
                               async,
                               stdin_pipe,
                               stdout_pipe,
                               pipe_read,
                               pipe_write,
                               NULL, NULL
                              );
        break;
    }
}

/*
** execute_pipline():
** パイプライン付きコマンドの実行
** execute_jobから呼び出される
** 
*/
void execute_pipeline(ASTreeNode* t, bool async)
{
    int file_desc[2];

    // printf("\t - execute_pipeline here.\n");
    // printf("\t - NODETYPE(t->type): %d\n", NODETYPE(t->type));
    // printf("\t - t->szData: %s\n", t->szData);

    pipe(file_desc);
    int pipewrite = file_desc[1];
    int piperead = file_desc[0];

	// read input from stdin for the first job
    /* 最初のjob(左のノード)のために、stdinから入力を読み込みます */
    execute_command(t->left, async, false, true, 0, pipewrite);
    ASTreeNode* jobNode = t->right;

    // printf("\t - NODETYPE(jobNode->type): %d\n", NODETYPE(jobNode->type));
    // printf("\t - jobNode->szData: %s\n", jobNode->szData);
    // printf("\n");

    /*  多重パイプの処理 ... typeにNODE_PIPEが設定されている間は繰り返し */
    while (jobNode != NULL && NODETYPE(jobNode->type) == NODE_PIPE)
    {
        close(pipewrite); // close the write end　/* パイプの書き込み側を閉じる */
        pipe(file_desc); /* 新しくpipeをひらく */
        pipewrite = file_desc[1]; /* 新しい方のディスクリプタをpipewriteに設定 */

        /* 先に実行するコマンド(左の枝) */
        execute_command(jobNode->left, async, true, true, piperead, pipewrite);
        close(piperead); /* 読み込み側のパイプをとじる */
        piperead = file_desc[0]; /* 新しく作ったほうのディスクリプタを設定 */

        jobNode = jobNode->right; /* 結果の引き渡し先になるコマンド(右の枝)に進む */
    }

    piperead = file_desc[0]; /* 読み込み側のパイプを指定 */
    close(pipewrite); /* 書き込み側のディスクリプタは使い終わっているので、閉じておく */

	// write output to stdout for the last job
    /* 最後のジョブ(右の枝)の実行結果を、stdoutに出力する */
    execute_command(jobNode, async, true, false, piperead, 0);	// only wait for the last command if necessary /* 必要であれば、直前のコマンドを待つだけ */
    close(piperead);
}

/*
** execute_job():
** execute_cmdline から呼び出される
** 第2引数のasyncの値は、同期実行にするか非同期実行にするかを切り替えるもの
** 
*/
void execute_job(ASTreeNode* jobNode, bool async)
{
    if (jobNode == NULL)
        return;

    // printf("\t - execute_job here.\n");
    // printf("\t - NODETYPE(jobNode->type): %d\n", NODETYPE(jobNode->type));
    // printf("\t - jobNode->szData: %s\n", jobNode->szData);
    // printf("\n");

    switch (NODETYPE(jobNode->type))
    {
    case NODE_PIPE: /* '|' の場合 */
        execute_pipeline(jobNode, async); /**/
        break;
    case NODE_CMDPATH: /* '|' がなく、単独のコマンドの場合 */
    default:
        execute_command(jobNode, async, false, false, 0, 0);
        break;
    }
}

/*
** execute_cmdline():
** execute_syntax_tree から呼び出される
** 引数で渡された抽象構文木のnodeのtypeを見て処理を振り分ける
*/
void execute_cmdline(ASTreeNode* cmdline)
{
    /* node がNULLのとき、実行すべきものがないのでreturn */
    if (cmdline == NULL)
        return;

    // printf("\t - execute_cmdline here.\n");
    // printf("\t - NODETYPE(cmdline->type): %d\n", NODETYPE(cmdline->type));
    // printf("\t - cmdline->szData: %s\n", cmdline->szData);
    // printf("\n");

    switch(NODETYPE(cmdline->type)) /* NODETYPEによって処理を振り分ける */
    {
    case NODE_SEQ: /* ';' の場合  */
        execute_job(cmdline->left, false); /* async(非同期実行) の引数をfalseにして、同期実行にする */
        execute_cmdline(cmdline->right); /* 右の枝は再度解析するので、execute_cmdlineへ渡す */
        break;

    case NODE_BCKGRND: /* '&' の 場合 */
        execute_job(cmdline->left, true); // job to be background /* asyncの引数をtrueにして、非同期実行にする */
        execute_cmdline(cmdline->right); /* 右の枝は再度解析するので、execute_cmdlineへ渡す */
        break;
    default:
        execute_job(cmdline, false); /* ';' や '&' がない単独のコマンドの場合 */
    }
}

/*
** execute_syntax_tree():
** shell.cから直接呼び出される関数
** astのルートからコマンドの実行を開始する
*/
void execute_syntax_tree(ASTreeNode* tree)
{
	// interpret the syntax tree
    execute_cmdline(tree);
}
