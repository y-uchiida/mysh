#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "execute.h"
#include "command.h"

void show_lexerlist(tok_t *tokens)
{
	tok_t *tmp;
	int i;

	tmp = tokens;
	i = 0;
	while (tmp != NULL)
	{
		printf("\t - token.data: %s\n", tmp->data);
		printf("\t - strlen(token.data): %d\n", strlen(tmp->data));
		printf("\t - token.type: %d\n", tmp->type);
		tmp = tmp->next;
		i++;
	}
	return ;
}

int main()
{
	// ignore Ctrl-\ Ctrl-C Ctrl-Z signals
	ignore_signal_for_shell(); /* シグナルの取得をする */

	// set the prompt
	set_prompt("swoorup % ");

	while (1)
	{
		char *linebuffer; /* 読み込んだコマンド行を保持する */
		size_t len; /* linebufferの領域の大きさ。getlineで自動設定する */

		lexer_t lexerbuf; /* 解析したトークンを保持するもので、連結リストになっている */
		ASTreeNode *exectree; /* 抽象構文木のルートを定義している */

		/* 割り込みが発生した場合に備えて、getline関数の実行をループにしておく */
		// keep getline in a loop in case interruption occurs
		int again = 1; /* getline関数(標準入力からのコマンド取得)をループするかどうかの真偽値 */
		while (again) {
			again = 0; /* ループしないようにしとく */
			printf("%s", getprompt()); /* プロンプトを出力 */
			linebuffer = NULL; /* 標準入力から受け取るための文字列ポインタの領域を空に */
			len = 0; /* linebufferの大きさ */

			/* ssize_t = typedef long */ /* nread: number of read charracters */
			/*
			** nread: 成功した場合、読み込んだ文字数。エラーの場合もしくはファイルの終端に達した場合には-1を返す。エラーの場合はerrnoに値を設定する
			** linebuffer: 標準入力に与えられた文字列を保持する
			** len: バッファの領域の大きさを指定できる。事前にmallocなどで領域を確保してある場合、その大きさを指定する。
			**      0だったら、getline()が新たに領域を確保してくれる。
			**      受け取る文字列がバッファの領域サイズを超えている場合、reallocで拡張してくれる
			**      領域を拡張した場合、その大きさに合わせてlenの値も更新される
			** stdin: 行の読込をするストリームを指定する。FILE構造体のポインタ
			** 
			*/
			ssize_t nread = getline(&linebuffer, &len, stdin);
			
			/* システムコールの割り込み発生した場合の対応。
			** 文字の読込ができておらず、かつerrnoにEINTRが設定されている時、getlineを再実行
			** EINTR: Interrupted system call
			*/
			if (nread <= 0 && errno == EINTR) {
				again = 1;        	// signal interruption, read again
				clearerr(stdin);	// clear the error
			}
		}

		// user pressed ctrl-D
		if (feof(stdin)) {
			exit(0);
			return 0;
		}
		
		// lexically analyze and build a list of tokens
		lexer_build(linebuffer, len, &lexerbuf); /* 字句解析を行い、トークン一覧を作成する */
		free(linebuffer);

		// printf("\n----- end lexer_buid -----\n");
		// show_lexerlist(lexerbuf.llisttok);

		/* 一つ以上のトークンがある場合、parserに処理を渡す */
		// parse the tokens into an abstract syntax tree
		if (!lexerbuf.ntoks || parse(&lexerbuf, &exectree) != 0) { /* tokenの連結リストを、構文解析にかける */
			continue; /* 入力文字の受け取りまで戻る */
		}

		/* 生成された抽象構文木に沿ってコマンドを実行 */
		execute_syntax_tree(exectree);

		/* 実行完了後…コマンドの入力データをfreeする */
		// free the structures
		ASTreeNodeDelete(exectree);
		lexer_destroy(&lexerbuf);
	}

	return 0;
}

