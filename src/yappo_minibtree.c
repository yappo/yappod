/*
 *
 *小規模なbtree
 *追加時のバランスなどは行なわれない
 *
 */
#include <stdio.h>

#include "yappo_minibtree.h"
#include "yappo_alloc.h"


/*
 *btreeの初期化
 */
MINIBTREE *YAP_Minibtree_init()
{
  MINIBTREE *p = (MINIBTREE *) YAP_malloc(sizeof(MINIBTREE));
  p->key   = NULL;
  p->data  = NULL;
  p->left  = NULL;
  p->right = NULL;
  p->top   = NULL;
  p->id    = 0;

  return p;
}


/*
 *rootをkeyで検索して一致するノードを返す
 */
MINIBTREE *YAP_Minibtree_search( MINIBTREE *root, unsigned char *key)
{
  MINIBTREE *this;

  /*子ノードが存在しない*/
  if (root->left == NULL && root->right == NULL) {
    return NULL;
  }

  /*探索開始*/
  this = root->left;
  while (1) {
    int cmp = 0;

    cmp = strcmp( this->key, key);

    if (cmp == 0) {
      /*一致*/
      return this;
    } else if(cmp > 0){
      /*左辺をチェック*/
      if (this->left == NULL) {
	return NULL;
      } else {
	/*左辺に移動*/
	this = this->left;
      }
    } else if(cmp < 0){
      /*右辺をチェック*/
      if (this->right == NULL) {
	return NULL;
      } else {
	/*右辺に移動*/
	this = this->right;
      }
    }
  }

  return NULL;
}

/*
 *rootノードにaddノードを追加する
 */
int YAP_Minibtree_add( MINIBTREE *root, MINIBTREE *add)
{
  MINIBTREE *this;
  
  /*子ノードが存在しないので左ノードに追加する*/
  if (root->left == NULL && root->right == NULL) {
    root->left = add;
    add->top = root;
    return 0;
  }

  /*追加可能なノードまでさがっていく*/
  this = root->left;
  while (1) {
    int cmp = 0;

    cmp = strcmp( this->key, add->key);

    if (cmp == 0) {
      /*一致してしまったら登録できない*/
      return 1;
    } else if(cmp > 0){
      /*左辺をチェック*/
      if (this->left == NULL) {
	/*左辺に登録*/
	this->left = add;
	this->left->top = this->left;
	return 0;
      } else {
	/*左辺に移動*/
	this = this->left;
      }
    } else if(cmp < 0){
      /*右辺をチェック*/
      if (this->right == NULL) {
	/*右辺に登録*/
	this->right = add;
	this->right->top = this->right;
	return 0;
      } else {
	/*右辺に移動*/
	this = this->right;
      }
    }
  }

  return 0;
}

