#ifndef LEXER_H
#define LEXER_H

enum TokenType /* 入力されたコマンドのtokenを種類分けしている…PIPEなど特殊な動作をする文字を独立させている */
{
	CHAR_GENERAL = -1,
	CHAR_PIPE = '|',
	CHAR_AMPERSAND = '&',
	CHAR_QOUTE = '\'',
	CHAR_DQUOTE = '\"',
	CHAR_SEMICOLON = ';',
	CHAR_WHITESPACE = ' ',
	CHAR_ESCAPESEQUENCE = '\\',
	CHAR_TAB = '\t',
	CHAR_NEWLINE = '\n',
	CHAR_GREATER = '>',
	CHAR_LESSER = '<',
	CHAR_NULL = 0,
	
	TOKEN	= -1,
};

enum
{ /* 入力状況の状態管理。クオートの入力待ちとか、エスケープ処理中とか… */
	STATE_IN_DQUOTE, /* ダブルクオート文字列の中にいる状態 */
	STATE_IN_QUOTE, /* シングルクオート文字列の中にいる状態 */
	
	STATE_IN_ESCAPESEQ, /* エスケープ文字が入力された状態 */
	STATE_GENERAL, /* 通常の状態 */
};

typedef struct tok tok_t;
typedef struct lexer lexer_t;

struct tok
{ /* 入力されたコマンドを解析した結果、tokenの一覧として連結リストにしている */
	char* data;
	int type;
	tok_t* next;
};

struct lexer
{ /* tokenの連結リストと、、、ntoksってなんだろう… number of tokens (tokenの数)と予想 */
	tok_t* llisttok;
	int ntoks;
};

int lexer_build(char* input, int size, lexer_t* lexerbuf);
void lexer_destroy(lexer_t* lexerbuf);
#endif
