/*
 *
 *DB回りの処理を行なう
 *
 */
#ifndef __YAPPO_DB_H__
#define __YAPPO_DB_H__

typedef unsigned long u_long;

#include <sys/types.h>
#include <db.h>
#include <pthread.h>

/* 1つの出現位置ファイルで扱うURL数 */
#define MAX_POS_URL 10000


/* DBを開くときのオプション */
#define YAPPO_DB_WRITE 0
#define YAPPO_DB_READ 1

/* 
 *  検索時に使用される各種キャッシュ
*/
typedef struct{ 
  /* 総URL数 */
  unsigned int total_filenum;

  /* 総DOMAIN数 */
  unsigned int total_domainnum;

  /* 総キーワード数 */
  unsigned int total_keywordnum;

  /* スコアファイルキャッシュ */
  double *score;
  unsigned int score_num;
  pthread_mutex_t score_mutex;

  /* ファイルサイズキャッシュ */
  int *size;
  unsigned int size_num;
  pthread_mutex_t size_mutex;

  /* URL文字数ファイルキャッシュ */
  int *urllen;
  unsigned int urllen_num;
  pthread_mutex_t urllen_mutex;

  /* 各URLのキーワード数ファイルキャッシュ */
  int filekeywordnum_num;
  unsigned int *filekeywordnum;
  pthread_mutex_t filekeywordnum_mutex;

  /* domain idファイルキャッシュ */
  int *domainid;
  unsigned int domainid_num;
  pthread_mutex_t domainid_mutex;

  /* 削除URLファイルキャッシュ */
  unsigned char *deletefile;
  unsigned int deletefile_num;
  pthread_mutex_t deletefile_mutex;
}YAPPO_CACHE;


/*
 *DBのファイルハンドル
 */
typedef struct{
  int  mode;/* dbを開くときのオプション */

  char *base_dir;/* dbの基本ディレクトリ */

  char *keyword_2byte_name, *postings_2byte_name;
  char *keyword_1byte_name, *postings_1byte_name;
  char *filedata_name;

  /* URL/文書ID対応表 */
  char *fileindex;
  char *fileindex_tmp;
  DB *fileindex_db;

  /* domain/domain ID対応表 */
  char *domainindex;
  char *domainindex_tmp;
  DB *domainindex_db;

  /* 辞書ファイル(2byte) */
  char *key2byte;
  char *key2byte_tmp;
  FILE *key2byte_file;

  /* 辞書ファイル(1byte) */
  char *key1byte;
  char *key1byte_tmp;
  DB *key1byte_db;

  /* 総URL数 */
  unsigned int total_filenum;
  char *filenum;
  FILE *filenum_file;

  /* 総DOMAIN数 */
  unsigned int total_domainnum;
  char *domainnum;
  FILE *domainnum_file;

  /* 総キーワード数 */
  unsigned int total_keywordnum;
  char *keywordnum;
  FILE *keywordnum_file;

  /* 各URLのDOMAIN ID */
  char *domainid;
  FILE *domainid_file;

  /* 各URLのサイズ */
  char *size;
  FILE *size_file;

  /* 各URLののベーススコア */
  char *score;
  FILE *score_file;

  /* 各URLのキーワード数 */
  char *filekeywordnum;
  FILE *filekeywordnum_file;

  /* URLの長さ */
  char *urllen;
  FILE *urllen_file;

  /* キーワードの総出現数 */
  char *keyword_totalnum;
  FILE *keyword_totalnum_file;

  /* キーワードの総出現URL数 */
  char *keyword_docsnum;
  FILE *keyword_docsnum_file;

  /*
   *linklist関係
   */
  /* linklistで取扱っているURL数 */
  int linklist_num;
  char *linklist;
  FILE *linklist_file;
  char *linklist_size;
  FILE *linklist_size_file;
  char *linklist_index;
  FILE *linklist_index_file;

  /*
   *出現位置ファイル関係
   */
  unsigned long pos_num;
  int pos_fileindex_start, pos_fileindex_end;
  /* インデックス作成等で使用する 現在処理中のfileindex */
  int pos_fileindex_start_w, pos_fileindex_end_w;
  char *pos;
  char *pos_tmp;
  FILE *pos_file;
  char *pos_size;
  char *pos_size_tmp;
  FILE *pos_size_file;
  char *pos_index;
  char *pos_index_tmp;
  FILE *pos_index_file;

  /*
   *URLメタデータ関係
   */
  char *filedata;
  char *filedata_tmp;
  FILE *filedata_file;
  char *filedata_size;
  char *filedata_size_tmp;
  FILE *filedata_size_file;
  char *filedata_index;
  char *filedata_index_tmp;
  FILE *filedata_index_file;

  /*
   *削除在URLファイル
   */
  char *deletefile;
  char *deletefile_tmp;
  FILE *deletefile_file;


  /* 検索時に使用される各種キャッシュ */
  YAPPO_CACHE *cache;
}YAPPO_DB_FILES;

void YAP_Db_filename_set (YAPPO_DB_FILES *p);

void YAP_Db_base_open (YAPPO_DB_FILES *p);
void YAP_Db_base_close (YAPPO_DB_FILES *p);

void YAP_Db_linklist_open (YAPPO_DB_FILES *p);
void YAP_Db_linklist_close (YAPPO_DB_FILES *p);

int YAP_Db_pos_open (YAPPO_DB_FILES *p, int id);
void YAP_Db_pos_close (YAPPO_DB_FILES *p);

#endif
