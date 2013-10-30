/*
 *
 *DB回りの処理を行なう
 *
 */
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "yappo_db.h"
#include "yappo_index.h"
#include "yappo_index_pos.h"
#include "yappo_index_filedata.h"
#include "yappo_index_deletefile.h"
#include "yappo_linklist.h"
#include "yappo_alloc.h"
#include "yappo_ngram.h"

/*
 *YAPPO_DB_FILESに必要なファイル名をセットする
 */
void YAP_Db_filename_set (YAPPO_DB_FILES *p)
{
  char *tmp;
  char *base = p->base_dir;
  int base_len = strlen(base) + 2;


  /* URLとIDの対比表 */
  tmp = (char *) YAP_malloc(base_len + strlen(FILEINDEX_NAME) + 1);
  sprintf( tmp, "%s/%s", base, FILEINDEX_NAME);
  p->fileindex = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(FILEINDEX_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, FILEINDEX_NAME);
  p->fileindex_tmp = tmp;

  /* DOMAINとIDの対比表 */
  tmp = (char *) YAP_malloc(base_len + strlen(DOMAININDEX_NAME) + 1);
  sprintf( tmp, "%s/%s", base, DOMAININDEX_NAME);
  p->domainindex = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(DOMAININDEX_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, DOMAININDEX_NAME);
  p->domainindex_tmp = tmp;

  /* 削除URL */
  tmp = (char *) YAP_malloc(base_len + strlen(DELETEFILE_NAME) + 1);
  sprintf( tmp, "%s/%s", base, DELETEFILE_NAME);
  p->deletefile = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(DELETEFILE_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, DELETEFILE_NAME);
  p->deletefile_tmp = tmp;

  /* URLメタデータ */
  tmp = (char *) YAP_malloc(base_len + strlen(FILEDATA_NAME) + 1);
  sprintf( tmp, "%s/%s", base, FILEDATA_NAME);
  p->filedata = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(FILEDATA_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, FILEDATA_NAME);
  p->filedata_tmp = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(FILEDATA_SIZE_NAME) + 1);
  sprintf( tmp, "%s/%s", base, FILEDATA_SIZE_NAME);
  p->filedata_size = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(FILEDATA_SIZE_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, FILEDATA_SIZE_NAME);
  p->filedata_size_tmp = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(FILEDATA_INDEX_NAME) + 1);
  sprintf( tmp, "%s/%s", base, FILEDATA_INDEX_NAME);
  p->filedata_index = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(FILEDATA_INDEX_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, FILEDATA_INDEX_NAME);
  p->filedata_index_tmp = tmp;

  /* 辞書ファイル */
  tmp = (char *) YAP_malloc(base_len + strlen(KEYWORD_2BYTE_NAME) + 1);
  sprintf( tmp, "%s/%s", base, KEYWORD_2BYTE_NAME);
  p->key2byte = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(KEYWORD_2BYTE_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, KEYWORD_2BYTE_NAME);
  p->key2byte_tmp = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(KEYWORD_1BYTE_NAME) + 1);
  sprintf( tmp, "%s/%s", base, KEYWORD_1BYTE_NAME);
  p->key1byte = tmp;
  tmp = (char *) YAP_malloc(base_len + strlen(KEYWORD_1BYTE_NAME) + 5);
  sprintf( tmp, "%s/%s_tmp", base, KEYWORD_1BYTE_NAME);
  p->key1byte_tmp = tmp;

  /* 登録URL数 */
  tmp = (char *) YAP_malloc(base_len + strlen(FILENUM_NAME) + 1);
  sprintf( tmp, "%s/%s", base, FILENUM_NAME);
  p->filenum = tmp;

  /* 登録DOMAIN数 */
  tmp = (char *) YAP_malloc(base_len + strlen(DOMAINNUM_NAME) + 1);
  sprintf( tmp, "%s/%s", base, DOMAINNUM_NAME);
  p->domainnum = tmp;

  /* 登録キーワード数 */
  tmp = (char *) YAP_malloc(base_len + strlen(KEYWORDNUM_NAME) + 1);
  sprintf( tmp, "%s/%s", base, KEYWORDNUM_NAME);
  p->keywordnum = tmp;

  /* 各URLのサイズ */
  tmp = (char *) YAP_malloc(base_len + strlen(SIZE_NAME) + 1);
  sprintf( tmp, "%s/%s", base, SIZE_NAME);
  p->size = tmp;

  /* 各URLのDOMAIN ID */
  tmp = (char *) YAP_malloc(base_len + strlen(DOMAINID_NAME) + 1);
  sprintf( tmp, "%s/%s", base, DOMAINID_NAME);
  p->domainid = tmp;

  /* 各URLのスコア */
  tmp = (char *) YAP_malloc(base_len + strlen(SCORE_NAME) + 1);
  sprintf( tmp, "%s/%s", base, SCORE_NAME);
  p->score = tmp;

  /* 各URLのキーワード数 */
  tmp = (char *) YAP_malloc(base_len + strlen(FILEKEYWORDNUM_NAME) + 1);
  sprintf( tmp, "%s/%s", base, FILEKEYWORDNUM_NAME);
  p->filekeywordnum = tmp;

  /* URLの長さ */
  tmp = (char *) YAP_malloc(base_len + strlen(URLLEN_NAME) + 1);
  sprintf( tmp, "%s/%s", base, URLLEN_NAME);
  p->urllen = tmp;

  /* キーワードの総出現数 */
  tmp = (char *) YAP_malloc(base_len + strlen(KEYWORD_TOTALNUM_NAME) + 1);
  sprintf( tmp, "%s/%s", base, KEYWORD_TOTALNUM_NAME);
  p->keyword_totalnum = tmp;

  /* キーワードの総出現URL数 */
  tmp = (char *) YAP_malloc(base_len + strlen(KEYWORD_DOCSNUM_NAME) + 1);
  sprintf( tmp, "%s/%s", base, KEYWORD_DOCSNUM_NAME);
  p->keyword_docsnum = tmp;
}

/*
 *baseファイルが存在していたら、destファイルにコピーする
 *destが存在していたらなにもしない
 */
void _tmp_copy (char *base, char *dest)
{
  struct stat f_stats;
 
  printf("%s/%s\n", base, dest);
  memset(&f_stats, 0, sizeof(struct stat));
  stat(dest, &f_stats);
  if (S_ISREG(f_stats.st_mode)) {
    return;
  }

  memset(&f_stats, 0, sizeof(struct stat));
  stat(base, &f_stats);
  if (S_ISREG(f_stats.st_mode)) {
    if (fork()) {
      int s;
      wait(&s);
    } else {
      execl("/bin/cp", "/bin/cp", base, dest, (char *) 0);
      exit(0);
    }
  }
}

/*
 *常時開いておくDBを開く
 */
void YAP_Db_base_open (YAPPO_DB_FILES *p)
{
  struct stat f_stats;
  u_int32_t mode = DB_RDONLY;
  char *fileindex, *domainindex, *deletefile, *key2byte, *key1byte;
  char *filedata, *filedata_size, *filedata_index;

  if (p->mode == YAPPO_DB_WRITE) {
    /* 書きこみモード */
    mode = DB_CREATE;

    /* すでにファイルが存在している場合は、作業用ファイルをコピーする */
    _tmp_copy(p->fileindex, p->fileindex_tmp);
    _tmp_copy(p->domainindex, p->domainindex_tmp);
    _tmp_copy(p->deletefile, p->deletefile_tmp);
    _tmp_copy(p->filedata, p->filedata_tmp);
    _tmp_copy(p->filedata_size, p->filedata_size_tmp);
    _tmp_copy(p->filedata_index, p->filedata_index_tmp);
    _tmp_copy(p->key2byte, p->key2byte_tmp);
    _tmp_copy(p->key1byte, p->key1byte_tmp);

    /* ファイル名の設定 */
    fileindex = p->fileindex_tmp;
    domainindex = p->domainindex_tmp;
    deletefile = p->deletefile_tmp;
    filedata = p->filedata_tmp;
    filedata_size = p->filedata_size_tmp;
    filedata_index = p->filedata_index_tmp;
    key2byte = p->key2byte_tmp;
    key1byte = p->key1byte_tmp;
  } else {
    /* ファイル名の設定 */
    fileindex = p->fileindex;
    domainindex = p->domainindex;
    deletefile = p->deletefile;
    filedata = p->filedata;
    filedata_size = p->filedata_size;
    filedata_index = p->filedata_index;
    key2byte = p->key2byte;
    key1byte = p->key1byte;
  }

  /* URLとIDの対比表 */
  db_create(&(p->fileindex_db), NULL, 0);
  p->fileindex_db->open(p->fileindex_db, NULL, fileindex, NULL,
                    DB_BTREE, mode, 0);

  /* DOMAINとIDの対比表 */
  db_create(&(p->domainindex_db), NULL, 0);
  p->domainindex_db->open(p->domainindex_db, NULL, domainindex, NULL,
                    DB_BTREE, mode, 0);

  /* 削除URL */
  if (p->mode == YAPPO_DB_WRITE) {
    /* 書きこみ時 */
    memset(&f_stats, 0, sizeof(struct stat));
    stat(deletefile, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      p->deletefile_file = fopen(deletefile, "w");
      fclose(p->deletefile_file);
    }
    p->deletefile_file = fopen(deletefile, "r+");
  } else {
    /* 読み込み時 */
    p->deletefile_file = fopen(deletefile, "r");
  }


  /* URLメタデータ */
  if (p->mode == YAPPO_DB_WRITE) {
    /* 書きこみ時 */
    memset(&f_stats, 0, sizeof(struct stat));
    stat(filedata, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      p->filedata_file = fopen(filedata, "w");
      fclose(p->filedata_file);
    }
    memset(&f_stats, 0, sizeof(struct stat));
    stat(filedata_size, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      p->filedata_size_file = fopen(filedata_size, "w");
      fclose(p->filedata_size_file);
    }
    memset(&f_stats, 0, sizeof(struct stat));
    stat(filedata_index, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      p->filedata_index_file = fopen(filedata_index, "w");
      fclose(p->filedata_index_file);
    }

    p->filedata_file = fopen(filedata, "r+");
    p->filedata_size_file = fopen(filedata_size, "r+");
    p->filedata_index_file = fopen(filedata_index, "r+");
  } else {
    /* 読み込み時 */
    p->filedata_file = fopen(filedata, "r");
    p->filedata_size_file = fopen(filedata_size, "r");
    p->filedata_index_file = fopen(filedata_index, "r");
  }

  /* 辞書ファイル 2byte */
  if (p->mode == YAPPO_DB_WRITE) {
    /* 書きこみ時 */
    memset(&f_stats, 0, sizeof(struct stat));
    stat(key2byte, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      char c = 0;
      p->key2byte_file = fopen(key2byte, "w");
      /*
	fseek(p->key2byte_file, sizeof(long) * (1 << (7 * Ngram_N)), SEEK_SET);
	fwrite(&c, 1, 1, p->key2byte_file);
      */
      fclose(p->key2byte_file);
    }
    p->key2byte_file = fopen(key2byte, "r+");
  } else {
    /* 読み込み時 */
    p->key2byte_file = fopen(key2byte, "r");
  }

  /* 辞書ファイル 1byte */
  db_create(&(p->key1byte_db), NULL, 0);
  p->key1byte_db->open(p->key1byte_db, NULL, key1byte, NULL,
                    DB_BTREE, mode, 0);



  /* 登録URL数 */
  memset(&f_stats, 0, sizeof(struct stat));
  stat(p->filenum, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->total_filenum = 0;
  } else {
    /* 読み込み */
    p->filenum_file = fopen(p->filenum, "r");
    fread(&(p->total_filenum), sizeof(int), 1, p->filenum_file);
    fclose(p->filenum_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));

  /* 登録DOMAIN数 */
  memset(&f_stats, 0, sizeof(struct stat));
  stat(p->domainnum, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->total_domainnum = 0;
  } else {
    /* 読み込み */
    p->domainnum_file = fopen(p->domainnum, "r");
    fread(&(p->total_domainnum), sizeof(int), 1, p->domainnum_file);
    fclose(p->domainnum_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));

  /* 登録キーワード数 */
  stat(p->keywordnum, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->total_keywordnum = 0;
  } else {
    /* 読み込み */
    p->keywordnum_file = fopen(p->keywordnum, "r");
    fread(&(p->total_keywordnum), sizeof(int), 1, p->keywordnum_file);
    fclose(p->keywordnum_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));

  printf("url=%d:key=%d\n", p->total_filenum, p->total_keywordnum);


  /* 各URLのサイズ */
  stat(p->size, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->size_file = fopen(p->size, "w");
    fclose(p->size_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));
  if (p->mode == YAPPO_DB_WRITE) {
    p->size_file = fopen(p->size, "r+");
    fseek(p->size_file, sizeof(int) * p->total_filenum, SEEK_SET);
  } else {
    p->size_file = fopen(p->size, "r");
  }

  /* 各URLのDOMAIN ID */
  stat(p->domainid, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->domainid_file = fopen(p->domainid, "w");
    fclose(p->domainid_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));
  if (p->mode == YAPPO_DB_WRITE) {
    p->domainid_file = fopen(p->domainid, "r+");
    fseek(p->domainid_file, sizeof(int) * p->total_domainnum, SEEK_SET);
  } else {
    p->domainid_file = fopen(p->domainid, "r");
  }

  /* 各URLのスコア */
  stat(p->score, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->score_file = fopen(p->score, "w");
    fclose(p->score_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));
  if (p->mode == YAPPO_DB_WRITE) {
    p->score_file = fopen(p->score, "r+");
    fseek(p->score_file, sizeof(double) * p->total_filenum, SEEK_SET);
  } else {
    p->score_file = fopen(p->score, "r");
  }


  /* 各URLのキーワード数 */
  stat(p->filekeywordnum, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->filekeywordnum_file = fopen(p->filekeywordnum, "w");
    fclose(p->filekeywordnum_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));
  if (p->mode == YAPPO_DB_WRITE) {
    p->filekeywordnum_file = fopen(p->filekeywordnum, "r+");
    fseek(p->filekeywordnum_file, sizeof(int) * p->total_filenum, SEEK_SET);
  } else {
    p->filekeywordnum_file = fopen(p->filekeywordnum, "r");
  }

  /* URLの長さ */
  stat(p->urllen, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->urllen_file = fopen(p->urllen, "w");
    fclose(p->urllen_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));
  if (p->mode == YAPPO_DB_WRITE) {
    p->urllen_file = fopen(p->urllen, "r+");
    fseek(p->urllen_file, sizeof(int) * p->total_filenum, SEEK_SET);
  } else {
    p->urllen_file = fopen(p->urllen, "r");
  }

  /* キーワードの総出現数 */
  stat(p->keyword_totalnum, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->keyword_totalnum_file = fopen(p->keyword_totalnum, "w");
    fclose(p->keyword_totalnum_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));
  if (p->mode == YAPPO_DB_WRITE) {
    p->keyword_totalnum_file = fopen(p->keyword_totalnum, "r+");
  } else {
    p->keyword_totalnum_file = fopen(p->keyword_totalnum, "r");
  }

  /* キーワードの総出現URL数 */
  stat(p->keyword_docsnum, &f_stats);
  if (! S_ISREG(f_stats.st_mode)) {
    /* 新規作成 */
    p->keyword_docsnum_file = fopen(p->keyword_docsnum, "w");
    fclose(p->keyword_docsnum_file);
  }
  memset(&f_stats, 0, sizeof(struct stat));
  if (p->mode == YAPPO_DB_WRITE) {
    p->keyword_docsnum_file = fopen(p->keyword_docsnum, "r+");
  } else {
    p->keyword_docsnum_file = fopen(p->keyword_docsnum, "r");
  }
}


/*
 *常時開いておくDBを閉じる
 */
void YAP_Db_base_close (YAPPO_DB_FILES *p)
{

  /* URLとIDの対比表 */
  p->fileindex_db->close(p->fileindex_db, 0);
  free(p->fileindex);
  p->fileindex = NULL;
  free(p->fileindex_tmp);
  p->fileindex_tmp = NULL;

  /* DOMAINとIDの対比表 */
  p->domainindex_db->close(p->domainindex_db, 0);
  free(p->domainindex);
  p->domainindex = NULL;
  free(p->domainindex_tmp);
  p->domainindex_tmp = NULL;

  /* 削除URL */
  fclose(p->deletefile_file);
  free(p->deletefile);
  p->deletefile = NULL;
  free(p->deletefile_tmp);
  p->deletefile_tmp = NULL;

  /* URLメタデータ */
  fclose(p->filedata_file);
  fclose(p->filedata_size_file);
  fclose(p->filedata_index_file);

  if (p->mode == YAPPO_DB_WRITE) {
    /* データファイルの掃除 */
    YAP_Index_Filedata_gc(p, p->filedata_tmp, p->filedata_size_tmp, p->filedata_index_tmp);
  }

  free(p->filedata);
  p->filedata = NULL;
  free(p->filedata_tmp);
  p->filedata_tmp = NULL;
  free(p->filedata_size);
  p->filedata_size = NULL;
  free(p->filedata_size_tmp);
  p->filedata_size_tmp = NULL;
  free(p->filedata_index);
  p->filedata_index = NULL;
  free(p->filedata_index_tmp);
  p->filedata_index_tmp = NULL;

  /* 辞書ファイル */
  fclose(p->key2byte_file);
  free(p->key2byte);
  p->key2byte = NULL;
  free(p->key2byte_tmp);
  p->key2byte_tmp = NULL;

  p->key1byte_db->close(p->key1byte_db, 0);
  free(p->key1byte);
  p->key1byte = NULL;
  free(p->key1byte_tmp);
  p->key1byte_tmp = NULL;

  if (p->mode == YAPPO_DB_WRITE) {
    DIR *pos_dir;
    struct dirent *direntp;
    char *dir_path;

    /* tmpファイルを元にもどす */
    dir_path = (char *) YAP_malloc(strlen(p->base_dir) + 1);
    sprintf(dir_path, "%s", p->base_dir);
    pos_dir = opendir(dir_path);
    while ((direntp = readdir(pos_dir)) != NULL) {
      char *name_tmp = direntp->d_name;
      int len = strlen(name_tmp);
      char *name = YAP_malloc(strlen(dir_path) + len + 2);
      sprintf(name, "%s/%s", dir_path, name_tmp);
      len = strlen(name);

      printf("name: %s\n", name);
      if (name[len-4] == '_' && name[len-3] == 't' && name[len-2] == 'm' && name[len-1] == 'p') {
	char *new_name = (char *) YAP_malloc(len + 1);
	strcpy(new_name, name);
	new_name[len-4] = 0;
	printf("/bin/mv %s %s\n", name, new_name);
	if (fork()) {
	  int s;
	  wait(&s);
	} else {
	  execl("/bin/mv", "/bin/mv", name, new_name, (char *) 0);
	  exit(0);
	}
	free(new_name);
      }
    }
    closedir(pos_dir);
    free(dir_path);

    /* 位置情報ファイルも元にもどす */
    dir_path = (char *) YAP_malloc(strlen(p->base_dir) + 5);
    sprintf(dir_path, "%s/pos", p->base_dir);
    pos_dir = opendir(dir_path);
    while ((direntp = readdir(pos_dir)) != NULL) {
      char *name_tmp = direntp->d_name;
      int len = strlen(name_tmp);
      char *name = YAP_malloc(strlen(dir_path) + len + 2);
      sprintf(name, "%s/%s", dir_path, name_tmp);
      len = strlen(name);

      printf("name: %s\n", name);
      if (name[len-4] == '_' && name[len-3] == 't' && name[len-2] == 'm' && name[len-1] == 'p') {
	char *new_name = (char *) YAP_malloc(len + 1);
	strcpy(new_name, name);
	new_name[len-4] = 0;
	new_name = (char *) YAP_realloc(new_name, strlen(new_name) + 1);
	printf("/bin/mv %s %s\n", name, new_name);
	if (fork()) {
	  int s;
	  wait(&s);
	} else {
	  execl("/bin/mv", "/bin/mv", name, new_name, (char *) 0);
	  exit(0);
	}
	free(new_name);
      }
    }
    closedir(pos_dir);
    free(dir_path);
  }

  /* 登録URL数 */
  if (p->mode == YAPPO_DB_WRITE) {
    /* 書きこみモード */
    p->filenum_file = fopen(p->filenum, "w"); 
    fwrite(&(p->total_filenum), sizeof(int), 1, p->filenum_file);
    fclose(p->filenum_file);
  }
  free(p->filenum);
  p->filenum = NULL;

  /* 登録DOMAIN数 */
  if (p->mode == YAPPO_DB_WRITE) {
    /* 書きこみモード */
    p->domainnum_file = fopen(p->domainnum, "w"); 
    fwrite(&(p->total_domainnum), sizeof(int), 1, p->domainnum_file);
    fclose(p->domainnum_file);
  }
  free(p->domainnum);
  p->domainnum = NULL;

  /* 登録キーワード数 */
  if (p->mode == YAPPO_DB_WRITE) {
    /* 書きこみモード */
    p->keywordnum_file = fopen(p->keywordnum, "w");
    fwrite(&(p->total_keywordnum), sizeof(int), 1, p->keywordnum_file);
    fclose(p->keywordnum_file);
  }
  free(p->keywordnum);
  p->keywordnum = NULL;

  /* 各URLのサイズ */
  fclose(p->size_file);
  free(p->size);
  p->size = NULL;

  /* 各URLのDOMAIN ID */
  fclose(p->domainid_file);
  free(p->domainid);
  p->domainid = NULL;

  /* 各URLのスコア */
  fclose(p->score_file);
  free(p->score);
  p->score = NULL;

  /* 各URLのキーワード数 */
  fclose(p->filekeywordnum_file);
  free(p->filekeywordnum);
  p->filekeywordnum = NULL;

  /* URLの長さ */
  fclose(p->urllen_file);
  free(p->urllen);
  p->urllen = NULL;

  /* キーワードの総出現数 */
  fclose(p->keyword_totalnum_file);
  free(p->keyword_totalnum);
  p->keyword_totalnum = NULL;

  /* キーワードの総出現URL数 */
  fclose(p->keyword_docsnum_file);
  free(p->keyword_docsnum);
  p->keyword_docsnum = NULL;
}


/*
 *linklistファイルを開く
 */
void YAP_Db_linklist_open (YAPPO_DB_FILES *p)
{
  char *base = p->base_dir;
  int base_len = strlen(base);
  int ret;
  struct stat f_stats; 

  p->linklist = (char *) YAP_malloc(base_len + strlen(LINKLIST_NAME) + 2);
  sprintf(p->linklist, "%s/%s", base, LINKLIST_NAME);
  memset(&f_stats, 0, sizeof(struct stat));
  stat(p->linklist, &f_stats);
  if ( ! S_ISREG(f_stats.st_mode)) {
    free(p->linklist);
    p->linklist = NULL;
    return;
  }
  p->linklist_file = fopen(p->linklist, "r");

  p->linklist_size = (char *) YAP_malloc(base_len + strlen(LINKLIST_SIZE_NAME) + 2);
  sprintf(p->linklist_size, "%s/%s", base, LINKLIST_SIZE_NAME);
  memset(&f_stats, 0, sizeof(struct stat));
  stat(p->linklist_size, &f_stats);
  if ( ! S_ISREG(f_stats.st_mode)) {
    free(p->linklist);
    p->linklist = NULL;
    free(p->linklist_size);
    p->linklist_size = NULL;

    fclose(p->linklist_file);
    return;
  }
  p->linklist_size_file = fopen(p->linklist_size, "r");

  p->linklist_index = (char *) YAP_malloc(base_len + strlen(LINKLIST_INDEX_NAME) + 2);
  sprintf(p->linklist_index, "%s/%s", base, LINKLIST_INDEX_NAME);
  memset(&f_stats, 0, sizeof(struct stat));
  stat(p->linklist_index, &f_stats);
  if ( ! S_ISREG(f_stats.st_mode)) {
    free(p->linklist);
    p->linklist = NULL;
    free(p->linklist_size);
    p->linklist_size = NULL;
    free(p->linklist_index);
    p->linklist_index = NULL;

    fclose(p->linklist_file);
    fclose(p->linklist_size_file);
    return;
  }
  p->linklist_index_file = fopen(p->linklist_index, "r");

  ret = fread(&(p->linklist_num), sizeof(int), 1, p->linklist_file);
  if (ret == 0) {
    p->linklist_num = 0;
  }
}

/*
 *linklistファイルを閉じる
 */
void YAP_Db_linklist_close (YAPPO_DB_FILES *p)
{
  if (p->linklist != NULL) {
    fclose(p->linklist_file);
    free(p->linklist);
    p->linklist = NULL;
    
    fclose(p->linklist_size_file);
    free(p->linklist_size);
    p->linklist_size = NULL;
    
    fclose(p->linklist_index_file);
    free(p->linklist_index);
    p->linklist_index = NULL;
    
    p->linklist_num = 0;
  }
}
  
/*
 *位置情報DBを開く
 */
int YAP_Db_pos_open (YAPPO_DB_FILES *p, int pos_id)
{
  char *tmp;
  char *base = p->base_dir;
  int base_len = strlen(base) + 2;
  struct stat f_stats; 
  char *pos, *pos_size, *pos_index;
  int ret;

  /* ファイル名作成 */
  tmp = (char *) YAP_malloc(base_len + strlen(POSTINGS_NAME) + 1);
  sprintf( tmp, "%s/"POSTINGS_NAME, base, pos_id);

  if (p->mode == YAPPO_DB_READ) {
    /*
     *読み込みモードなら
     *開く前にファイルが存在しているか調べる
     */
    memset(&f_stats, 0, sizeof(struct stat));
    stat(tmp, &f_stats);

    if (! S_ISREG(f_stats.st_mode)) {
      /* 存在しない */
      free(tmp);
      return 0;
    }
  }

  p->pos = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(POSTINGS_NAME) + 5);
  sprintf( tmp, "%s/"POSTINGS_NAME"_tmp", base, pos_id);
  p->pos_tmp = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(POSTINGS_SIZE_NAME) + 1);
  sprintf( tmp, "%s/"POSTINGS_SIZE_NAME, base, pos_id);
  p->pos_size = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(POSTINGS_SIZE_NAME) + 5);
  sprintf( tmp, "%s/"POSTINGS_SIZE_NAME"_tmp", base, pos_id);
  p->pos_size_tmp = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(POSTINGS_INDEX_NAME) + 1);
  sprintf( tmp, "%s/"POSTINGS_INDEX_NAME, base, pos_id);
  p->pos_index = tmp;

  tmp = (char *) YAP_malloc(base_len + strlen(POSTINGS_INDEX_NAME) + 5);
  sprintf( tmp, "%s/"POSTINGS_INDEX_NAME"_tmp", base, pos_id);
  p->pos_index_tmp = tmp;


  /*
   *DBを開く
   */
  if (p->mode == YAPPO_DB_WRITE) {
    /*
     *書きこみモード
     *すでにファイルが存在している場合は、作業用ファイルをコピーする
     */
    _tmp_copy(p->pos, p->pos_tmp);
    _tmp_copy(p->pos_size, p->pos_size_tmp);
    _tmp_copy(p->pos_index, p->pos_index_tmp);

    /* ファイル名の設定 */
    pos = p->pos_tmp;
    pos_size = p->pos_size_tmp;
    pos_index = p->pos_index_tmp;
  } else {
    /* ファイル名の設定 */
    pos = p->pos;
    pos_size = p->pos_size;
    pos_index = p->pos_index;
  }


  if (p->mode == YAPPO_DB_WRITE) {
    /* 書き込み時 */
    memset(&f_stats, 0, sizeof(struct stat));
    stat(pos, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      int i = 0;
      long l = 0;
      p->pos_file = fopen(pos, "w");
      fwrite(&l, sizeof(long), 1, p->pos_file);
      fwrite(&i, sizeof(int), 1, p->pos_file);
      fwrite(&i, sizeof(int), 1, p->pos_file);
      fclose(p->pos_file);
    }
    memset(&f_stats, 0, sizeof(struct stat));
    stat(pos_size, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      p->pos_size_file = fopen(pos_size, "w");
      fclose(p->pos_size_file);
    }
    memset(&f_stats, 0, sizeof(struct stat));
    stat(pos_index, &f_stats);
    if ( ! S_ISREG(f_stats.st_mode)) {
      p->pos_index_file = fopen(pos_index, "w");
      fclose(p->pos_index_file);
    }
    
    p->pos_file = fopen(pos, "r+");
    p->pos_size_file = fopen(pos_size, "r+");
    p->pos_index_file = fopen(pos_index, "r+");
  } else {
    /* 読み込み時 */
    p->pos_file = fopen(pos, "r");
    p->pos_size_file = fopen(pos_size, "r");
    p->pos_index_file = fopen(pos_index, "r");
  }

  /* 各種情報を読みこむ */
  ret = fread(&(p->pos_num), sizeof(long), 1, p->pos_file);
  if (ret == 0) {
    p->pos_num = 0;
  }
  ret = fread(&(p->pos_fileindex_start), sizeof(int), 1, p->pos_file);
  if (ret == 0) {
    p->pos_fileindex_start = 0;
  }
  ret = fread(&(p->pos_fileindex_end), sizeof(int), 1, p->pos_file);
  if (ret == 0) {
    p->pos_fileindex_end = 0;
  }

  return 1;
}

/*
 *位置情報DBを閉じる
 */
void YAP_Db_pos_close (YAPPO_DB_FILES *p)
{


  /* 各種情報を書きこむ */
  if (p->mode == YAPPO_DB_WRITE) {

    if (p->pos_fileindex_start == 0) {
      p->pos_fileindex_start = p->pos_fileindex_start_w;
    }
    if (p->pos_fileindex_end < p->pos_fileindex_end_w) {
      p->pos_fileindex_end = p->pos_fileindex_end_w;
    }

    fseek(p->pos_file, 0L, SEEK_SET);
    fwrite(&(p->pos_num), sizeof(long), 1, p->pos_file);
    fwrite(&(p->pos_fileindex_start), sizeof(int), 1, p->pos_file);
    fwrite(&(p->pos_fileindex_end), sizeof(int), 1, p->pos_file);
  }

  /* ファイルを閉じる */
  fclose(p->pos_file);
  fclose(p->pos_size_file);
  fclose(p->pos_index_file);

  /* データの整理を行なう */
  if (p->mode == YAPPO_DB_WRITE) {
    YAP_Index_Pos_gc(p, p->pos_tmp, p->pos_size_tmp, p->pos_index_tmp);
  }

  /*
   *メモリクリア
   */
  free(p->pos);
  p->pos = NULL;
  free(p->pos_tmp);
  p->pos_tmp = NULL;
  free(p->pos_size);
  p->pos_size = NULL;
  free(p->pos_size_tmp);
  p->pos_size_tmp = NULL;
  free(p->pos_index);
  p->pos_index = NULL;
  free(p->pos_index_tmp);
  p->pos_index_tmp = NULL;
}

/*
 *キャッシュの初期化
 */
void YAP_Db_cache_init (YAPPO_CACHE *p) 
{
  memset(p, 0, sizeof(YAPPO_CACHE));

  pthread_mutex_init(&(p->score_mutex), NULL);
  pthread_mutex_init(&(p->size_mutex), NULL);
  pthread_mutex_init(&(p->urllen_mutex), NULL);
  pthread_mutex_init(&(p->filekeywordnum_mutex), NULL);
  pthread_mutex_init(&(p->domainid_mutex), NULL);
  pthread_mutex_init(&(p->deletefile_mutex), NULL);

  p->score = NULL;
  p->size = NULL;
  p->urllen = NULL;
  p->filekeywordnum = NULL;
  p->domainid = NULL;
  p->deletefile = NULL;

  p->score_num = 0;
  p->size_num = 0;
  p->urllen_num = 0;
  p->filekeywordnum_num = 0;
  p->domainid_num = 0;
  p->deletefile_num = 0;

  p->total_filenum = 0;
  p->total_domainnum = 0;
  p->total_keywordnum = 0;
}

/*
 *キャッシュの破棄
 */
void YAP_Db_cache_destroy (YAPPO_CACHE *p) 
{
  pthread_mutex_destroy(&(p->score_mutex));
  pthread_mutex_destroy(&(p->size_mutex));
  pthread_mutex_destroy(&(p->urllen_mutex));
  pthread_mutex_destroy(&(p->filekeywordnum_mutex));
  pthread_mutex_destroy(&(p->domainid_mutex));
  pthread_mutex_destroy(&(p->deletefile_mutex));

  if (p->score != NULL) {
    free(p->score);
    p->score = NULL;
  }
  if (p->size != NULL) {
    free(p->size);
    p->size = NULL;
  }
  if (p->urllen != NULL) {
    free(p->urllen);
    p->urllen = NULL;
  }
  if (p->filekeywordnum != NULL) {
    free(p->filekeywordnum);
    p->filekeywordnum = NULL;
  }
  if (p->domainid != NULL) {
    free(p->domainid);
    p->domainid = NULL;
  }
  if (p->deletefile != NULL) {
    free(p->deletefile);
    p->deletefile = NULL;
  }

  p->score_num = 0;
  p->size_num = 0;
  p->urllen_num = 0;
  p->filekeywordnum_num = 0;
  p->domainid_num = 0;
  p->deletefile_num = 0;

  p->total_filenum = 0;
  p->total_domainnum = 0;
  p->total_keywordnum = 0;
}

/*
 *必要ならば各ファイルをメモリ上にキャッシュする
 */
void YAP_Db_cache_load (YAPPO_DB_FILES *ydfp, YAPPO_CACHE *p) 
{

  if (ydfp->total_filenum != p->total_filenum ||
      ydfp->total_domainnum != p->total_domainnum ||
      ydfp->total_keywordnum != p->total_keywordnum) {
    /*
     *キャッシュ上の数値と実際の数値が食い違っている
     *ので読みこみ開始する
     */

    printf("CACHE LOAD\n");

    /* スコアファイルキャッシュ */
    pthread_mutex_lock(&(p->score_mutex));
    p->score = (double *) YAP_realloc(p->score, sizeof(double) * ydfp->total_filenum);
    fseek(ydfp->score_file, 0L, SEEK_SET);
    p->score_num = fread(p->score, sizeof(double), ydfp->total_filenum, ydfp->score_file);
    pthread_mutex_unlock(&(p->score_mutex));

    /* ファイルサイズキャッシュ */
    pthread_mutex_lock(&(p->size_mutex));
    p->size = (int *) YAP_realloc(p->size, sizeof(int) * ydfp->total_filenum);
    fseek(ydfp->size_file, 0L, SEEK_SET);
    p->size_num = fread(p->size, sizeof(int), ydfp->total_filenum, ydfp->size_file);
    pthread_mutex_unlock(&(p->size_mutex));

    /* URL文字数ファイルキャッシュ */
    pthread_mutex_lock(&(p->urllen_mutex));
    p->urllen = (int *) YAP_realloc(p->urllen, sizeof(int) * ydfp->total_filenum);
    fseek(ydfp->urllen_file, 0L, SEEK_SET);
    p->urllen_num = fread(p->urllen, sizeof(int), ydfp->total_filenum, ydfp->urllen_file);
    pthread_mutex_unlock(&(p->urllen_mutex));


    /* 各URLのキーワード数ファイルキャッシュ */
    pthread_mutex_lock(&(p->filekeywordnum_mutex));
    p->filekeywordnum = (int *) YAP_realloc(p->filekeywordnum, sizeof(int) * ydfp->total_filenum);
    fseek(ydfp->filekeywordnum_file, 0L, SEEK_SET);
    p->filekeywordnum_num = fread(p->filekeywordnum, sizeof(int), ydfp->total_filenum, ydfp->filekeywordnum_file);
    pthread_mutex_unlock(&(p->filekeywordnum_mutex));
    

    /* domain idファイルキャッシュ */
    pthread_mutex_lock(&(p->domainid_mutex));
    p->domainid = (int *) YAP_realloc(p->domainid, sizeof(int) * ydfp->total_filenum);
    fseek(ydfp->domainid_file, 0L, SEEK_SET);
    p->domainid_num = fread(p->domainid, sizeof(int), ydfp->total_filenum, ydfp->domainid_file);
    pthread_mutex_unlock(&(p->domainid_mutex));

    /* 削除URLファイルキャッシュ */
    pthread_mutex_lock(&(p->domainid_mutex));
    p->deletefile = (char *) YAP_realloc(p->deletefile, (ydfp->total_filenum / 8) + 1);
    fseek(ydfp->deletefile_file, 0L, SEEK_SET);
    p->deletefile_num = fread(p->deletefile, 1, (ydfp->total_filenum / 8) + 1, ydfp->deletefile_file);
    pthread_mutex_unlock(&(p->domainid_mutex));

    printf("load delete: %d/%d\n", p->deletefile_num, (ydfp->total_filenum / 8) + 1);

    p->total_filenum = ydfp->total_filenum;
    p->total_domainnum = ydfp->total_domainnum;
    p->total_keywordnum = ydfp->total_keywordnum;
  }

}
