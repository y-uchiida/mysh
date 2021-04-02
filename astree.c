#include "astree.h"
#include <assert.h>
#include <stdlib.h>

/*
** ASTreeAttachBinaryBranch():
** 二分木のAST(抽象構文木)に、ノードを追加する
** rootは二分木のルートノードだけではなく、AST上のいずれかのノードを指定できる
*/
void ASTreeAttachBinaryBranch(ASTreeNode* root, ASTreeNode* leftNode, ASTreeNode* rightNode)
{
    assert(root != NULL); /* root == NULL のとき、プログラムを停止 */
    root->left = leftNode; /* rootに対して、左の子ノードを追加 */
    root->right = rightNode; /* rootに対して、右の子ノードを追加 */
}

/*
** ASTreeNodeSetType():
** 二分木のAST(抽象構文木)のノードに、NodeTypeを設定する
** NodeType は、astree.hで定義されている
** typedef enum {
**  NODE_PIPE 			= (1 << 0),
**  NODE_BCKGRND 		= (1 << 1),
**  NODE_SEQ 			= (1 << 2),
**  NODE_REDIRECT_IN 	= (1 << 3),
**  NODE_REDIRECT_OUT 	= (1 << 4),
**  NODE_CMDPATH		= (1 << 5),
**  NODE_ARGUMENT		= (1 << 6),

**  NODE_DATA 			= (1 << 7),
** } NodeType;
*/
void ASTreeNodeSetType(ASTreeNode* node, NodeType nodetype)
{
    assert(node != NULL);
    node->type = nodetype;
}

/*
** ASTreeNodeSetData():
** 指定の文字列を、nodeのszDataに保存する
** パイプやリダイレクトなどの制御用文字ではなく、
** データとして保持すべきトークン文字列をノードに関連付けて保存する処理である
** トークン文字列を持つノードは、メンバnodeTypeに "NODE_DATA"が追加で付与される
** 追加での付与なので、そのノードが持つ役割を示すタイプと並列して設定される
** つまり、コマンドライン引数のトークンを解析して抽象構文木に登録する場合、
** 引数であることを示す NODE_AGUMENT とともに
** 与えられた引数の文字列を持っていることを示す NODE_DATA も追加的に設定される
*/
void ASTreeNodeSetData(ASTreeNode* node, char* data)
{
    assert(node != NULL);
    if(data != NULL) { /* 引数に、保持すべきトークン文字列が与えられていたら */
        node->szData = data;
        node->type |= NODE_DATA; /* NODE_DATAのビットを1にする...szDataにデータが入っていることを示す？ */
    }
}

/*
** ASTreeNodeDelete():
** 指定したノードを削除する
** typeにNODE_DATAが設定されている場合、文字列データも保存されているので
** それも併せて解放する
** また、そのノードにつながっている子ノードも再帰的に解放する
*/
void ASTreeNodeDelete(ASTreeNode* node)
{
    if (node == NULL)
        return;

    if (node->type & NODE_DATA) /* NODE_DATAのフラグが入っているかを検査する...szDataに値があるかどうか？ */
        free(node->szData); /* トークン文字列が保存されている場合、それを解放 */

    ASTreeNodeDelete(node->left); /* 左のノードを再帰的に解放する */
    ASTreeNodeDelete(node->right); /* 右のノードを再帰的に解放する */
    free(node);
}
