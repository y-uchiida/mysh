#include <stdio.h>
#include <glob.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"


/*
** show_globbuf:
** glob()の動作検証のために、処理内容が入っているglobbufの内容を一覧表示する関数
*/
void show_globbuf(glob_t globbuf)
{
	glob_t tmp;
	int i;

	printf("----- show_globbuf start. -----\n\n");

	i = 0;
	tmp = globbuf;
	while (tmp.gl_pathv != NULL && *(tmp.gl_pathv) != NULL)
	{
		printf("\t - [%d] %s\n", i, *(tmp.gl_pathv));
		*(tmp.gl_pathv)++;
		i++;
	}
	printf("\n----- show_globbuf end. -----\n\n");

	return ;
}

/*
** getchartype:
** 文字を受け取り、コマンドラインの文法上のその文字のタイプ(文字が持つ意味)を返す
*/
int getchartype(char c)
{
	switch(c)
	{
		case '\'':
			return CHAR_QOUTE;
			break;
		case '\"':
			return CHAR_DQUOTE;
			break;
		case '|':
			return CHAR_PIPE;
			break;
		case '&':
			return CHAR_AMPERSAND;
			break;
		case ' ':
			return CHAR_WHITESPACE;
			break;
		case ';':
			return CHAR_SEMICOLON;
			break;
		case '\\':
			return CHAR_ESCAPESEQUENCE;
			break;
		case '\t':
			return CHAR_TAB;
			break;
		case '\n':
			return CHAR_NEWLINE;
			break;
		case '>':
			return CHAR_GREATER;
			break;
		case '<':
			return CHAR_LESSER;
			break;
		case 0:
			return CHAR_NULL;
			break;
		default:
			return CHAR_GENERAL;
	};
	
	return CHAR_GENERAL;
}

/* クオートを取り除く処理 */
void strip_quotes(char* src, char* dest)
{
	int n = strlen(src);
	if (n <= 1) { /* コピー元の文字列の長さが0,または1のとき、複製だけして終了 */
		strcpy(dest, src);
		return;
	}
	
	int i; /* srcのインデックス */
	char lastquote = 0; /* 直前に合ったのが、ダブルクオートかシングルクオート化 */
	int j = 0; /* destのインデックス */
	
	for (i=0; i < n; i++) /* srcの最後(n文字目)まで1文字ずつ処理 */
	{
		char c = src[i];
		if ((c == '\'' || c == '\"') && lastquote == 0) /* 最初にクオートを見つけたら、lastquoteに記憶して読み飛ばし */
			lastquote = c;
		else if (c == lastquote) /* 直前に現れたクオートと同じなら、lastquoteをリセットして読み飛ばし */
			lastquote = 0;
		else
			dest[j++] = c; /* クオート以外の文字、またはlastquoteに記憶したクオートと違う種類のものだったら、destにコピー */
	}
	
	dest[j] = 0; /* 末尾に終端文字をつける */
}

/* tokenの内容を初期化する */
void tok_init(tok_t* tok, int datasize)
{
	/* 入力された文字列がひとつのtokenだった場合に備えて、最大サイズ + 1で領域を確保する */
	tok->data = malloc(datasize + 1); // 1 for null terminator
	tok->data[0] = 0;
	
	/* いったん、内容をNULLにしておく */
	tok->type = CHAR_NULL;
	tok->next = NULL;
}

void tok_destroy(tok_t* tok) {
	if (tok != NULL) {
		free(tok->data);
		tok_destroy(tok->next);
		free(tok);
	}
}

/*
** 標準入力から受け取った文字列input から、tokenの一覧を作成する
** input: stdinからgetlineした文字列
** size: inputが格納される文字列領域(linebuffer)のサイズ
** lexerbuf: tokenを保持するための構造体
*/
int lexer_build(char* input, int size, lexer_t* lexerbuf)
{

	if (lexerbuf == NULL) /* lexerbufがNULLはあり得ない…ので、エラーとして終了 */
		return -1;
	
	if (size == 0) { /* 1文字も入力されてない場合 */
		lexerbuf->ntoks = 0; /* tokenの数を0に設定 */
		return 0;
	}
	
	lexerbuf->llisttok = malloc(sizeof(tok_t)); /* 最初のtokenを入れるポインタを作成 */
	
	/* リストの先頭になるtokenポインタを確保する */
	// allocate the first token
	tok_t* token = lexerbuf->llisttok; /* トークンの連結リストのポインタを代入 */
	tok_init(token, size);
	
	int i = 0;
	int j = 0, ntemptok = 0;
	
	char c; /* 1文字ずつ、input文字列の中身を確認していく */
	int state = STATE_GENERAL;
	
	do
	{
		/* iはinputの文字カウンタ、jはtokenの文字カウンタ */
		c = input[i]; /* i文字目を取得 */
		int chtype = getchartype(c); /* その文字のタイプを取得。特別な意味を持たない文字の場合、-1が返っている */
		
		/* 検査中の文字列の状態を確認する */
		if (state == STATE_GENERAL) /* 特別な状態ではない場合 */
		{
			switch (chtype)
			{
				case CHAR_QOUTE: /* i文字目がシングルクオートだった場合…シングルクオートで囲まれた文字列という認識を開始 */
					state = STATE_IN_QUOTE; /* 状態認識を変更 */
					token->data[j++] = CHAR_QOUTE; /*  */
					token->type = TOKEN;
					break;
					
				case CHAR_DQUOTE: /* i文字目がダブルクオートだった場合…ダブルクオート文字列が開始したと認識 */
					state = STATE_IN_DQUOTE; /* 状態認識を変更 */
					token->data[j++] = CHAR_DQUOTE; /*  */
					token->type = TOKEN;
					break;
					
				case CHAR_ESCAPESEQUENCE: /* i文字目がエスケープ(\\)だった場合…1文字のバックスラッシュとして処理 */
					token->data[j++] = input[++i];
					token->type = TOKEN;
					break;
					
				case CHAR_GENERAL: /* 通常の文字のとき */
					token->data[j++] = c; /* 現在のトークンの末尾に1文字追加 */
					token->type = TOKEN; /*  */
					break;
					
				case CHAR_WHITESPACE:
					if (j > 0) {
						token->data[j] = 0;
						token->next = malloc(sizeof(tok_t));
						token = token->next;
						tok_init(token, size - i);
						j = 0;
					}
					break;
					
				case CHAR_SEMICOLON: /* セミコロンの場合 */
				case CHAR_GREATER: /* 大なり記号の場合 */
				case CHAR_LESSER: /* 小なり記号の場合 */
				case CHAR_AMPERSAND: /* アンパサンドの場合 */
				case CHAR_PIPE: /* パイプ記号の場合 */
					
					/* 読み取っていたトークンがあれば終了させておく */
					// end the token that was being read before
					if (j > 0) {
						token->data[j] = 0;
						token->next = malloc(sizeof(tok_t));
						token = token->next;
						tok_init(token, size - i);
						j = 0;
					}
					
					/* 単独の文字でトークンを生成する */
					// next token
					token->data[0] = chtype;
					token->data[1] = 0;
					token->type = chtype; /* token_typeはTOKEN(-1)ではなくそれぞれのchartypeを設定しておく */
					
					/* そして次のトークンを生成 */
					token->next = malloc(sizeof(tok_t));
					token = token->next;
					tok_init(token, size - i);
					break;
			}
		}
		else if (state == STATE_IN_DQUOTE) { /* ダブルクオート文字列内のとき */
			token->data[j++] = c; /* 末尾に1文字追加 */
			if (chtype == CHAR_DQUOTE)
				state = STATE_GENERAL; /* i文字目がダブルクオートだった場合、ダブルクオート文字列の状態を終了 */
			
		}
		else if (state == STATE_IN_QUOTE) { /* シングルクオート文字列内のとき */
			token->data[j++] = c; /* 末尾に1文字追加 */
			if (chtype == CHAR_QOUTE)
				state = STATE_GENERAL; /* i文字目がシングルクオートだったら、シングルクオート文字列の状態を終了 */
		}
		
		if (chtype == CHAR_NULL) { /* i文字目がヌル文字の場合 */
			if (j > 0) { /* 1文字以上入力があったとしたら、 */
				token->data[j] = 0; /* 文字列に終端を付ける */
				ntemptok++; /* tokenの数を増やす */
				j = 0; /* tokenの文字数のカウンタを戻す */
			}
		}
		
		i++;
	} while (c != '\0'); /* i文字目が終端でなければ繰り返し */
	
	/* glob()関数を用いて、ワイルドカード記号を展開する処理を行う */
	token = lexerbuf->llisttok; /* 先頭のトークンのポインタに戻す */
	int k = 0; 
	while (token != NULL) /* 末尾のトークンまで順に実施 */
	{
		if (token->type == TOKEN)
		{ /* 通常のトークンの場合(type==token) */
			glob_t globbuf;
			glob(token->data, GLOB_TILDE, NULL, &globbuf);
			
			// show_globbuf(globbuf);

			if (globbuf.gl_pathc > 0)
			{
				k += globbuf.gl_pathc; /* pathcはマッチしたパスの数。これをいままでの処理結果に加算 */
				/* 後で連結できるように、次のトークンのポインタを保持しておく */
				// save the next token so we can attach it later
				tok_t* saved = token->next;

				/* 現在のトークンを、最初のトークンと置き換える */				
				// replace the current token with the first one
				free(token->data); 
				token->data = malloc(strlen(globbuf.gl_pathv[0]) + 1);
				strcpy(token->data, globbuf.gl_pathv[0]);
								
				int i; /* 2つ目以降の文字列をループでトークンのリストに追加する */
				for (i = 1; i < globbuf.gl_pathc; i++)
				{
					token->next = malloc(sizeof(tok_t));
					tok_init(token->next, strlen(globbuf.gl_pathv[i]));
					token = token->next;
					token->type = TOKEN;
					strcpy(token->data, globbuf.gl_pathv[i]);
				}
				
				token->next = saved; /* 最後に、保持しておいたもともとのトークンのリストをつなげる */
			}
			/* globでパスのマッチがない場合( == 置換が必要なワイルドカードを含んでいなかった場合) */
			else {
				/* ユーザーからのトークンは、特殊文字をエスケープするために引用符で囲まれている場合があるので、それを取り除く */
				// token from the user might be inside quotation to escape special characters
				// hence strip the quotation symbol
				char* stripped = malloc(strlen(token->data) + 1);
				strip_quotes(token->data, stripped);
				free(token->data);
				token->data = stripped;
				k++;
			}
		}
		
		token = token->next; /* 処理を次のtokenへ進める */
	}
	
	lexerbuf->ntoks = k; /* ワイルドカードをすべて展開した後のtoken数に更新 */
	return k;
}

void lexer_destroy(lexer_t* lexerbuf)
{
	if (lexerbuf == NULL)
		return;
	
	tok_destroy(lexerbuf->llisttok);
}
