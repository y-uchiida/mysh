#ifndef ASTREE_H
#define ASTREE_H

typedef enum {
    NODE_PIPE 			= (1 << 0),
    NODE_BCKGRND 		= (1 << 1),
    NODE_SEQ 			= (1 << 2),
    NODE_REDIRECT_IN 	= (1 << 3),
    NODE_REDIRECT_OUT 	= (1 << 4),
    NODE_CMDPATH		= (1 << 5),
    NODE_ARGUMENT		= (1 << 6),

    NODE_DATA 			= (1 << 7),
} NodeType;

/*
** AST: abstract syntax tree(抽象構文木)
** 単純な二分木構造を定義している
*/
typedef struct ASTreeNode
{
    int type; /* たぶん、NodeTypeを入れるんだと思う */
    char* szData; /* サイズデータ？分からん…実際のテキストかな… */
    struct ASTreeNode* left; /* 左の枝へのポインタ */
    struct ASTreeNode* right; /* 右の枝へのポインタ */
} ASTreeNode;

#define NODETYPE(a) (a & (~NODE_DATA))	// get the type of the nodes

void ASTreeAttachBinaryBranch (ASTreeNode * root , ASTreeNode * leftNode , ASTreeNode * rightNode);
void ASTreeNodeSetType (ASTreeNode * node , NodeType nodetype );
void ASTreeNodeSetData (ASTreeNode * node , char * data );
void ASTreeNodeDelete (ASTreeNode * node );

#endif
