#ifndef ASTREE_H
#define ASTREE_H

typedef enum {
    NODE_PIPE 			= (1 << 0), /* パイプ ( '|' ) */
    NODE_BCKGRND 		= (1 << 1), /* バックグラウンド実行する ( '&' ) */ 
    NODE_SEQ 			= (1 << 2), /* 実行完了後に残りの処理に入る ( ';' ) */
    NODE_REDIRECT_IN 	= (1 << 3), /* 入力受け取りリダイレクト ( '<' ) */
    NODE_REDIRECT_OUT 	= (1 << 4), /* 出力先指定リダイレクト ( '>' ) */
    NODE_CMDPATH		= (1 << 5), /* 実行ファイルのパス(コマンド名) */
    NODE_ARGUMENT		= (1 << 6), /* 単独の引数 */

    NODE_DATA 			= (1 << 7), /* 制御文字ではなく、文字列データ(token)を持つことを示す */
} NodeType;

/*
** AST: abstract syntax tree(抽象構文木)
** 単純な二分木構造を定義している
*/
typedef struct ASTreeNode
{
    int type; /* enum NodeType */
    char* szData; /* パースした結果のトークン文字列...szDataがどんな意味かは不明 */
    struct ASTreeNode* left; /* 左の枝へのポインタ */
    struct ASTreeNode* right; /* 右の枝へのポインタ */
} ASTreeNode;

/*
** NODETYPE(a):
** 抽象構文木のノードを受け取り、そのメンバtypeの値からNODE_TYPEを取得する
*/
#define NODETYPE(a) (a & (~NODE_DATA))	// get the type of the nodes

void ASTreeAttachBinaryBranch (ASTreeNode * root , ASTreeNode * leftNode , ASTreeNode * rightNode);
void ASTreeNodeSetType (ASTreeNode * node , NodeType nodetype );
void ASTreeNodeSetData (ASTreeNode * node , char * data );
void ASTreeNodeDelete (ASTreeNode * node );

#endif
