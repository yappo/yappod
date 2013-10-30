/*
 *N-gramに関する処理
 */
#ifndef __NGRAM_H__ 
#define __NGRAM_H__ 

/* N-gramで切り出すバイト数 */
#define Ngram_N 4

/*
 *出現キーワードと、出現位置を収めるリスト
 */
typedef struct ngram_list{
  unsigned char *keyword;
  int index_count;
  int *index;
  struct ngram_list *next;
}NGRAM_LIST;

/*
 *出現キーワードと出現位置を格納する
 */
typedef struct{
  unsigned char *keyword;/* キーワード */
  int pos;/* 出現位置 */
}NGRAM_SEARCH_LIST;

NGRAM_SEARCH_LIST *YAP_Ngram_tokenize_search(char *body, int *keyword_num);
NGRAM_LIST *YAP_Ngram_tokenize(char *body, int *keyword_num);

char *YAP_Ngram_get_1byte(unsigned char *tokp);
char *YAP_Ngram_get_2byte(unsigned char *tokp);

#endif
