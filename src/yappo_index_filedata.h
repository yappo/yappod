/*
 *URLメタデータを取扱う
 */
#ifndef __INDEX_FILEDATA_H__
#define __INDEX_FILEDATA_H__

#include "yappo_db.h"
#include "yappo_index.h"

/*
 *filedateで利用される
 *
 */
typedef struct{
  char *url;
  char *title;
  char *comment;
  int size;
  int keyword_num;/* 文書に含まれるキーワード数 */
  time_t lastmod;
  int domainid;
  unsigned char *other;
  int other_len;
}FILEDATA;


#define FILEDATA_NAME "filedata"
#define FILEDATA_SIZE_NAME "filedata_size"
#define FILEDATA_INDEX_NAME "filedata_index"


int YAP_Index_Filedata_get(YAPPO_DB_FILES *ydfp, int fileindex, FILEDATA *filedata);
int YAP_Index_Filedata_put(YAPPO_DB_FILES *ydfp, int fileindex, FILEDATA *filedata);
int YAP_Index_Filedata_del(YAPPO_DB_FILES *ydfp, int fileindex);
int YAP_Index_Filedata_free (FILEDATA *p);
int YAP_Index_Filedata_gc(YAPPO_DB_FILES *ydfp, char *filedata, char *filedata_size, char *filedata_index);

#endif
