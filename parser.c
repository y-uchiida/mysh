#include "parser.h"
#include "lexer.h"
#include "astree.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*** Shell Grammer as given in the assignment question 1 ***/
/* 直訳：割り当てられた質問1 で与えられたシェルの文法 */

/* BNF記法によるコマンドラインの構文 */
/**
 *
 * // <command line>(ユーザーがプロンプトで入力したコマンドライン文字列)は
 * // <job> と <command line> の塊に分割することができる
 * // また、<job> と <command line>の間には '&' もしくは ';' で区切られる
 * // '&' または ';' がない場合は、ひとかたまりの<job>として認識される
	<command line>	::=  	<job>
						|	<job> '&'
						| 	<job> '&' <command line>
						|	<job> ';'
						|	<job> ';' <command line>

 * // <job>(一連の処理としてまとめられたコマンド実行)は、
 * // <job> と <command> の塊に分割することができる
 * // <job> と<command>は、 '|' で区切られる
 * // '|' が無ければ、その<job> は一塊のものとして認識される
	<job>			::=		<command>
						|	<job> '|' <command>

 * // <command>(リダイレクト演算子による入出力先の指定を伴ったコマンド実行)は、
 * // <simple command> と <filename> に分割することができる
 * // <simple command> と <filename> は、　リダイレクト演算子('<', または '>') で区切られる
	<command>		::=		<simple command>
						|	<simple command> '<' <filename>
						|	<simple command> '>' <filename>

 * // simple command の定義... man bash より 引用
 * // 単純なコマンド (simple command) とは、 変数の代入を並べたもの (これは省略可能です) の後に、
 * // ブランク区切りの単語とリダイレクションを記述し、 最後に制御演算子を置いたものです。
 * // 最初の単語は実行するコマンドを指定します。 これは 0 番目の引き数となります。
 * // 残りの単語は起動されるコマンドに引き数として渡されます。
 * // --- 引用ここまで
 * //
 * // <simple command>(ひとつのコマンドプログラムと、実行の際に渡すコマンドライン引数)は、
 * // <pathname>(コマンドプログラムの位置を示すパス)と<token>(コマンドライン引数のひとつ)に分割できる
 * // つまり、最終的には <pathname> <token> <token> <token> ... の形になる
	<simple command>::=		<pathname>
						|	<simple command>  <token>
 *
 *
 *
**/

/*** Shell Grammer for recursive descent parser ***/
/* 直訳: 再帰下降パーサーのためのシェル構文 */

/*** Removed left recursion and left factoring ***/
/* 直訳: 左再帰と左ファクタリングを除去 */

/*
** 左再帰の定義...wikipediaより引用
** 左再帰（英: Left recursion）とは、言語（普通、形式言語について言うが、自然言語に対しても考えられ得る）の
** 文法（構文規則）にあらわれる再帰的な規則（定義）の特殊な場合で、ある非終端記号を展開した結果、その先頭（最も左）に
** その非終端記号自身があらわれるような再帰のことである。
** --- 引用ここまで
** つまり、上記の修正前BNFでは
** 	<job>			::=		<command>
					|	    <job> '|' <command> // <-　<job>を展開したら、先頭にまた<job>がでてくる
**
** もしくは、
** 	<simple command>::=		<pathname>
					|   	<simple command>  <token> // <- <simple command>の展開結果の先頭が<simple command>
**
** などが該当するということ…らしい
*/

/*
** 正確な理解まで至っていないが…再帰下降構文解析のロジックで解析関数を実装すると、
** 左再帰の部分が無限再帰に陥ってしまうので、再帰下降構文解析をする場合には避けなければならない
** なお、バックトラックを行うようにすれば、左再帰を回避することは可能らしい
** ただし、一般的にバックトラックは処理の効率が悪く、複雑な実装となることが多い
** 左再帰のためにバックトラックを用いるよりも、左再帰を発生させない構文解析ができるほうがよい
** 参考: LL(1)構文解析 - いらずんば
** https://bowmoq.hatenadiary.org/entry/20090221/1235192369
*/

/**
 *
 * // 修正前と変更なし(順番の入れ替えあり)
	<command line>	::= 	<job> ';' <command line>
						|	<job> ';'
						| 	<job> '&' <command line>
						|	<job> '&'
							<job>

 * // <job> ::= <job> '|' <command> だったのを、 <command> '|' <job>に変更している
	<job>			::=		<command> '|' <job>
						|	<command>

 * // 修正前と変更なし(順番の入れ替えあり)
	<command>		::=		<simple command> '<' <filename> // this grammer is a bit incorrect, see grammer.llf // 直訳: この文法は少し正確ではありません。grammer.llfを確認してください。
						|	<simple command> '>' <filename>
						|	<simple command>

 * // <simple command> は、修正前は以下の2パターンだった
 * //
 * // <simple command>::=	<pathname>
 * // 					|	<simple command>  <token>
 * //
 * // 連続する <token> を右側に持つのではなく、左側に一つの <pathname> と
 * // それに続く <token list> を持つように変更されている
    <simple command>::=		<pathname> <token list>

 * // <token list> の定義を追加
 * // こちらも左再帰が起こらないように、左にひとつの<token> と
 * // それに続く <token list> があるという形で定義している
 * // そして末尾(最後のtoken) が EMPTY(== NULL)になったら解析完了
	<token list>	::=		<token> <token list>
						|	(EMPTY)

 *
 *
 *
**/

ASTreeNode* CMDLINE();		//	test all command line production orderwise
ASTreeNode* CMDLINE1();		//	<job> ';' <command line>
ASTreeNode* CMDLINE2();		//	<job> ';'
ASTreeNode* CMDLINE3();		//	<job> '&' <command line>
ASTreeNode* CMDLINE4();		//	<job> '&'
ASTreeNode* CMDLINE5();		//	<job>

ASTreeNode* JOB();			// test all job production in order
ASTreeNode* JOB1();			// <command> '|' <job>
ASTreeNode* JOB2();			// <command>

ASTreeNode* CMD();			// test all command production orderwise
ASTreeNode* CMD1();			//	<simple command> '<' <filename>
ASTreeNode* CMD2();			//	<simple command> '>' <filename>
ASTreeNode* CMD3();			//	<simple command>

ASTreeNode* SIMPLECMD();	// test simple cmd production
ASTreeNode* SIMPLECMD1();	// <pathname> <token list>

ASTreeNode* TOKENLIST();	// test tokenlist production
ASTreeNode* TOKENLIST1();	//	<token> <token list>
ASTreeNode* TOKENLIST2();	//	EMPTY

/*
** グローバル変数として現在処理中のトークンのポインタを宣言
*/
// curtok token pointer
tok_t* curtok = NULL;

/*
** term():
** curtok(現在解析中のtoken)のtokentypeを判定する
** curtoe->typeと引数で与えられたtokentypeと一致すればtrueを返し、
** そうでなければfalseを返す。
** trueの場合にbufferptrが与えられていれば、bufferptrにcurtokの値を複製する
** falseの場合は、終了時にcurtokをひとつ先に進める
*/
bool term(int toketype, char** bufferptr)
{
	if (curtok == NULL)
		return false;
	
    if (curtok->type == toketype)
    {
		if (bufferptr != NULL) {
			*bufferptr = malloc(strlen(curtok->data) + 1);
			strcpy(*bufferptr, curtok->data);
		}
		curtok = curtok->next;
        return true;
    }

    curtok = curtok->next;
    return false;
}

/*
** CMDLINE():
** すべてのコマンドライン生成物を順番に検証する
*/
ASTreeNode *CMDLINE()
{
    /*
    ** 処理中のtokenのポインタをいったん保存する
    ** 解析のコマンドでcurtokを先に進めてしまうので、
    ** 元の位置に戻れるようにするためだと思われる
    */
    tok_t *save = curtok; 

    ASTreeNode *node; /* 抽象構文木の要素となるポインタを宣言 */

    if ((curtok = save, node = CMDLINE1()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE2()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE3()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE4()) != NULL)
        return node;

    if ((curtok = save, node = CMDLINE5()) != NULL)
        return node;

    /* どれにも当てはまらなかったら、NULLを返す(終端ってことかな…) */
    return NULL;
}

/*
** CMDLINE1():
** 以下のパターンに合致するかを検証する
** <job> ';' <command line>
*/
ASTreeNode* CMDLINE1()
{
    ASTreeNode* jobNode;
    ASTreeNode* cmdlineNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

    if (!term(CHAR_SEMICOLON, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    if ((cmdlineNode = CMDLINE()) == NULL) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_SEQ);
    ASTreeAttachBinaryBranch(result, jobNode, cmdlineNode);

    return result;
}

/*
** CMDLINE2():
** 以下のパターンに合致するかを検証する
** <job> ';'
*/
ASTreeNode* CMDLINE2()
{
    ASTreeNode* jobNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

	if (!term(CHAR_SEMICOLON, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_SEQ);
    ASTreeAttachBinaryBranch(result, jobNode, NULL);

    return result;
}

/*
** CMDLINE3():
** 以下のパターンに合致するかを検証する
** <job> '&' <command line>
*/
ASTreeNode* CMDLINE3()
{
    ASTreeNode* jobNode;
    ASTreeNode* cmdlineNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

    if (!term(CHAR_AMPERSAND, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    if ((cmdlineNode = CMDLINE()) == NULL) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_BCKGRND);
    ASTreeAttachBinaryBranch(result, jobNode, cmdlineNode);

    return result;
}

/*
** CMDLINE4():
** 以下のパターンに合致するかを検証する
** <job> '&'
*/
ASTreeNode* CMDLINE4()
{
    ASTreeNode* jobNode;
    ASTreeNode* result;

    if ((jobNode = JOB()) == NULL)
        return NULL;

	if (!term(CHAR_AMPERSAND, NULL)) {
        ASTreeNodeDelete(jobNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_BCKGRND);
    ASTreeAttachBinaryBranch(result, jobNode, NULL);

    return result;
}

/*
** CMDLINE5():
** 以下のパターンに合致するかを検証する
** <job>
*/
ASTreeNode* CMDLINE5()
{
    return JOB();
}

/*
** JOB():
** すべてジョブの生成物を順番に検証する
** <job>
*/
ASTreeNode* JOB()
{
    tok_t* save = curtok;

    ASTreeNode* node;

    if ((curtok = save, node = JOB1()) != NULL)
        return node;

    if ((curtok = save, node = JOB2()) != NULL)
        return node;

    return NULL;
}

/*
** JOB1():
** 以下のパターンに合致するかを検証する
** <command> '|' <job>
*/
ASTreeNode* JOB1()
{
    ASTreeNode* cmdNode;
    ASTreeNode* jobNode;
    ASTreeNode* result;

    if ((cmdNode = CMD()) == NULL)
        return NULL;

    if (!term(CHAR_PIPE, NULL)) {
        ASTreeNodeDelete(cmdNode);
        return NULL;
    }

    if ((jobNode = JOB()) == NULL) {
        ASTreeNodeDelete(cmdNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_PIPE);
    ASTreeAttachBinaryBranch(result, cmdNode, jobNode);

    return result;
}

/*
** JOB2():
** 以下のパターンに合致するかを検証する
** <command>
*/
ASTreeNode* JOB2()
{
    return CMD();
}

/*
** CMD():
** すべてコマンド生成物を順番に検証する
*/
ASTreeNode* CMD()
{
    tok_t* save = curtok;

    ASTreeNode* node;

    if ((curtok = save, node = CMD1()) != NULL)
        return node;

    if ((curtok = save, node = CMD2()) != NULL)
        return node;

    if ((curtok = save, node = CMD3()) != NULL)
        return node;

    return NULL;
}

/*
** CMD1():
** 以下のパターンに合致するかを検証する
** <simple command> '<' <filename>
*/
ASTreeNode* CMD1()
{
    ASTreeNode* simplecmdNode;
    ASTreeNode* result;

    if ((simplecmdNode = SIMPLECMD()) == NULL)
        return NULL;

    if (!term(CHAR_LESSER, NULL)) {
		ASTreeNodeDelete(simplecmdNode);
		return NULL;
	}
	
	char* filename;
	if (!term(TOKEN, &filename)) {
		free(filename);
        ASTreeNodeDelete(simplecmdNode);
        return NULL;
    }

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_REDIRECT_IN);
    ASTreeNodeSetData(result, filename);
    ASTreeAttachBinaryBranch(result, NULL, simplecmdNode);

    return result;
}

/*
** CMD2():
** 以下のパターンに合致するかを検証する
** <simple command> '>' <filename>
*/
ASTreeNode* CMD2()
{
    ASTreeNode* simplecmdNode;
    ASTreeNode* result;

    if ((simplecmdNode = SIMPLECMD()) == NULL)
        return NULL;

	if (!term(CHAR_GREATER, NULL)) {
		ASTreeNodeDelete(simplecmdNode);
		return NULL;
	}
	
	char* filename;
	if (!term(TOKEN, &filename)) {
		free(filename);
		ASTreeNodeDelete(simplecmdNode);
		return NULL;
	}

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_REDIRECT_OUT);
    ASTreeNodeSetData(result, filename);
	ASTreeAttachBinaryBranch(result, NULL, simplecmdNode);

    return result;
}

/*
** CMD3():
** 以下のパターンに合致するかを検証する
** <simple command>
*/
ASTreeNode* CMD3()
{
    return SIMPLECMD();
}

/*
** SIMPLECMD():
** 単純なコマンド形式の生成物を順番に検証する
*/
ASTreeNode* SIMPLECMD()
{
    tok_t* save = curtok;
    return SIMPLECMD1();
}

/*
** SIMPLECMD1():
** 以下のパターンに合致するかを検証する
** <pathname> <token list>
*/
ASTreeNode* SIMPLECMD1()
{
    ASTreeNode* tokenListNode;
    ASTreeNode* result;

    char* pathname;
    if (!term(TOKEN, &pathname))
        return NULL;

    tokenListNode = TOKENLIST();
    // we don't check whether tokenlistNode is NULL since its a valid grammer

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_CMDPATH);
    ASTreeNodeSetData(result, pathname);
	ASTreeAttachBinaryBranch(result, NULL, tokenListNode);

    return result;
}

/*
** TOKENLIST():
** トークンリスト生成物の検証を順番に行う
*/
ASTreeNode* TOKENLIST()
{
    tok_t* save = curtok;

    ASTreeNode* node;

    if ((curtok = save, node = TOKENLIST1()) != NULL)
        return node;

    if ((curtok = save, node = TOKENLIST2()) != NULL)
        return node;

    return NULL;
}

/*
** TOKENLIST1():
** 以下のパターンに合致するかを検証する
** <token> <token list>
*/
ASTreeNode* TOKENLIST1()
{
    ASTreeNode* tokenListNode;
    ASTreeNode* result;

    char* arg;
    if (!term(TOKEN, &arg))
        return NULL;

    tokenListNode = TOKENLIST();
    // we don't check whether tokenlistNode is NULL since its a valid grammer
    /* tokenListNodeがNULLである場合も有効な文法であるため、エラーとしてチェックしない */

    result = malloc(sizeof(*result));
    ASTreeNodeSetType(result, NODE_ARGUMENT);
    ASTreeNodeSetData(result, arg);
	ASTreeAttachBinaryBranch(result, NULL, tokenListNode);

    return result;
}

/*
** TOKENLIST2():
** 以下のパターンに合致するかを検証する
** EMPTY
*/
ASTreeNode* TOKENLIST2()
{
    /* 順番に解析されていき、最終的にEMPTYになる…のだと思われ */
    return NULL;
}

/*
** perser():
** tokensから抽象構文木を生成する
*/
int parse(lexer_t* lexbuf, ASTreeNode** syntax_tree)
{
	if (lexbuf->ntoks == 0) /* tokenがひとつもない場合、終了する */
		return -1;
	
    /* curtok: current token pointer
    ** とりあえずlexbufが保持しているtokenリストの先頭のポインタを取っている…
    */
	curtok = lexbuf->llisttok;

    /*
    ** tokenリストを解析した結果の抽象構文木を返してくる関数CMDLINEを実行
    ** CMDLINE内部で、<command line> -> <job> -> <command> -> <simple command> -> <token list> -> <token> の順に分割しながら解析を行ってくれる
    */
    *syntax_tree = CMDLINE();
	
    /* 解析すべきtokenが残っているのに、CMDLINE()から処理が戻っている = エラー */
    if (curtok != NULL && curtok->type != 0)
    {
        printf("Syntax Error near: %s\n", curtok->data);
        return -1;
    }
	
	return 0;
}
