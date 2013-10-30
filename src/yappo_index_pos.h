/*
 *キーワード出現位置データを取扱う
 */
#ifndef __INDEX_POS_H__
#define __INDEX_POS_H__

#include "yappo_db.h"

#define POSTINGS_NAME "pos/%d"
#define POSTINGS_SIZE_NAME "pos/%d_size"
#define POSTINGS_INDEX_NAME "pos/%d_index"

int YAP_Index_Pos_get(YAPPO_DB_FILES *ydfp, unsigned long keyword_id, unsigned char **postings_buf, int *postings_buf_len);
int YAP_Index_Pos_put(YAPPO_DB_FILES *ydfp, unsigned long keyword_id, unsigned char *postings_buf, int postings_buf_len);
int YAP_Index_Pos_del(YAPPO_DB_FILES *ydfp, unsigned long keyword_id);
int YAP_Index_Pos_gc(YAPPO_DB_FILES *ydfp, char *pos, char *pos_size, char *pos_index);
#endif
