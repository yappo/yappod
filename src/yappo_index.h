/*
 *インデクシング回り
 */
#ifndef __INDEX_H__
#define __INDEX_H__

#include "yappo_db.h"


#define FILEINDEX_NAME "fileindex.db"
#define DOMAININDEX_NAME "domainindex.db"
#define FILENUM_NAME "filenum"
#define DOMAINNUM_NAME "domainnum"
#define KEYWORDNUM_NAME "keywordnum"
#define DOMAINID_NAME "domainid"
#define SCORE_NAME "score"
#define SIZE_NAME "size"
#define FILEKEYWORDNUM_NAME "filekeywordnum"
#define URLLEN_NAME "urllen"
#define KEYWORD_TOTALNUM_NAME "keyword_totalnum"
#define KEYWORD_DOCSNUM_NAME "keyword_docsnum"

#define KEYWORD_2BYTE_NAME "keyword_2byte"
#define KEYWORD_1BYTE_NAME "keyword_1byte.db"


/*
 *辞書ファイルの構造体
 *
 */
/*
typedef struct{
  unsigned long keyword_id;//一意なID
  int total_num;//キーワードの出現回数
  int docs_num;//キーワードの出現文書数
}KEYWORD;
*/

int YAP_Index_get_keyword(YAPPO_DB_FILES *ydfp, unsigned char *keyword, unsigned long *keyword_id);
int YAP_Index_put_keyword(YAPPO_DB_FILES *ydfp, unsigned char *keyword, unsigned long *keyword_id);


unsigned char *YAP_Index_8bit_encode(int *list, int list_len, int *ret_len);
int *YAP_Index_8bit_decode(unsigned char *encode, int *list_len, int len);
#endif
