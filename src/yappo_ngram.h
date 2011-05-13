/*
 *N-gram�˴ؤ������
 */
#ifndef __NGRAM_H__ 
#define __NGRAM_H__ 

/* N-gram���ڤ�Ф��Х��ȿ� */
#define Ngram_N 4

/*
 *�и�������ɤȡ��и����֤�����ꥹ��
 */
typedef struct ngram_list{
  unsigned char *keyword;
  int index_count;
  int *index;
  struct ngram_list *next;
}NGRAM_LIST;

/*
 *�и�������ɤȽи����֤��Ǽ����
 */
typedef struct{
  unsigned char *keyword;/* ������� */
  int pos;/* �и����� */
}NGRAM_SEARCH_LIST;

NGRAM_SEARCH_LIST *YAP_Ngram_tokenize_search(char *body, int *keyword_num);
NGRAM_LIST *YAP_Ngram_tokenize(char *body, int *keyword_num);

char *YAP_Ngram_get_1byte(unsigned char *tokp);
char *YAP_Ngram_get_2byte(unsigned char *tokp);

#endif
