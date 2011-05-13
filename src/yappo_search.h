/*
 *
 *キーワードを保有するドキュメント番号の取得等
 *
 */
#ifndef __SEARCH_H__ 
#define __SEARCH_H__ 

#include "yappo_db.h"
#include "yappo_index.h"
#include "yappo_ngram.h"

/*
 *該当文書データ
 */
typedef struct{ 
  int fileindex;/* URL ID */
  int pos_len;/* 出現位置の数 */
  double score;/* tf.idf */
  int *pos;/* 出現位置 */
}SEARCH_DOCUMENT;

/*
 *検索結果
 */
typedef struct{
  unsigned long keyword_id;
  int keyword_total_num;
  int keyword_docs_num;
  SEARCH_DOCUMENT *docs_list;/* 該当文書データ */
}SEARCH_RESULT;

void YAP_Search_result_free (SEARCH_RESULT *p);
SEARCH_RESULT *YAP_Search_result_delete (YAPPO_DB_FILES *ydfp, SEARCH_RESULT *p);

SEARCH_RESULT *YAP_Search_op_and (SEARCH_RESULT *left, SEARCH_RESULT *right, int order);
SEARCH_RESULT *YAP_Search_op_or (SEARCH_RESULT *left, SEARCH_RESULT *right);
SEARCH_RESULT *YAP_Search_phrase (YAPPO_DB_FILES *ydfp, NGRAM_SEARCH_LIST *ngram_list, int ngram_list_len);
SEARCH_RESULT *YAP_Search_gram (YAPPO_DB_FILES *ydfp, unsigned char *key);
SEARCH_RESULT *YAP_Search_word (YAPPO_DB_FILES *ydfp, char *keyword);

SEARCH_RESULT *YAP_Search (YAPPO_DB_FILES *ydfp, char **keyword_list, int keyword_list_num, int max_size, int op);
#endif 
