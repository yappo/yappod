/*
 *
 *�����Ϥ�btree
 *�ɲû��ΥХ�󥹤ʤɤϹԤʤ��ʤ�
 *
 */
#include <stdio.h>

#include "yappo_minibtree.h"
#include "yappo_alloc.h"


/*
 *btree�ν����
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
 *root��key�Ǹ������ư��פ���Ρ��ɤ��֤�
 */
MINIBTREE *YAP_Minibtree_search( MINIBTREE *root, unsigned char *key)
{
  MINIBTREE *this;

  /*�ҥΡ��ɤ�¸�ߤ��ʤ�*/
  if (root->left == NULL && root->right == NULL) {
    return NULL;
  }

  /*õ������*/
  this = root->left;
  while (1) {
    int cmp = 0;

    cmp = strcmp( this->key, key);

    if (cmp == 0) {
      /*����*/
      return this;
    } else if(cmp > 0){
      /*���դ�����å�*/
      if (this->left == NULL) {
	return NULL;
      } else {
	/*���դ˰�ư*/
	this = this->left;
      }
    } else if(cmp < 0){
      /*���դ�����å�*/
      if (this->right == NULL) {
	return NULL;
      } else {
	/*���դ˰�ư*/
	this = this->right;
      }
    }
  }

  return NULL;
}

/*
 *root�Ρ��ɤ�add�Ρ��ɤ��ɲä���
 */
int YAP_Minibtree_add( MINIBTREE *root, MINIBTREE *add)
{
  MINIBTREE *this;
  
  /*�ҥΡ��ɤ�¸�ߤ��ʤ��ΤǺ��Ρ��ɤ��ɲä���*/
  if (root->left == NULL && root->right == NULL) {
    root->left = add;
    add->top = root;
    return 0;
  }

  /*�ɲò�ǽ�ʥΡ��ɤޤǤ����äƤ���*/
  this = root->left;
  while (1) {
    int cmp = 0;

    cmp = strcmp( this->key, add->key);

    if (cmp == 0) {
      /*���פ��Ƥ��ޤä�����Ͽ�Ǥ��ʤ�*/
      return 1;
    } else if(cmp > 0){
      /*���դ�����å�*/
      if (this->left == NULL) {
	/*���դ���Ͽ*/
	this->left = add;
	this->left->top = this->left;
	return 0;
      } else {
	/*���դ˰�ư*/
	this = this->left;
      }
    } else if(cmp < 0){
      /*���դ�����å�*/
      if (this->right == NULL) {
	/*���դ���Ͽ*/
	this->right = add;
	this->right->top = this->right;
	return 0;
      } else {
	/*���դ˰�ư*/
	this = this->right;
      }
    }
  }

  return 0;
}

