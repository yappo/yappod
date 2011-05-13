/*
 *
 *複数のキーワード出現DBを１つのDBにまとめる
 *
 */
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

#include "yappo_db.h"
#include "yappo_index.h"
#include "yappo_alloc.h"
#include "yappo_ngram.h"
#include "yappo_minibtree.h"

#define DB_MULTIPLE_BUF 1024 * 1024

#define MAP_TEMPLATE "/tmp/YAPPOMAKEMARGE_XXXXXX"

/*
http id:783
*/

char **buf;
unsigned long *buf_len;

typedef struct {
  char *data;
  char *index;
  char *size;

  FILE *data_fp;
  FILE *index_fp;
  FILE *size_fp;

  int seek_stop;
} pos_t;


int main(int argc, char *argv[])
{
  char *output_file, *input_dir;

  char *delete_file;
  FILE *delete_fp;

  char *key_num_file;
  FILE *key_num_fp;
  int key_num, key_pos;

  pos_t **inputs;
  pos_t *output;
  int start, end, num, i;
  int tmp_size;
  struct stat f_stats;
  int new_index;
  int seek_stops = 0;

  /*
   *オプションを取得
   */
  if (argc > 1) {
    i = 1;
    while (1) {
      if ( argc == i)
	break;

      if (! strcmp(argv[i], "-s")) {
	/*まとめるDBの始め*/
	i++;
	if (argc == i)
	  break;
	start = atoi(argv[i]);
      } else if (! strcmp(argv[i], "-e")) {
	/*まとめるDBの終り*/
	i++;
	if (argc == i)
	  break;
	end = atoi(argv[i]);
      } else if (! strcmp(argv[i], "-l")) {
	/*入力を取得*/
	i++;
	if (argc == i)
	  break;
	input_dir = argv[i];
      } else if (! strcmp(argv[i], "-d")) {
	/*出力先を取得*/
	i++;
	if (argc == i)
	  break;
	output_file = argv[i];
      }
      i++;
    }
  }

  num = end - start + 1;
  if (num < 1) {
    printf("Usage: %s -l input_index -d output_file -s start -e end\n", argv[0]);
    printf("pos num error: %d - %d = %d\n", end, start, num);
    exit(-1);
  }

  if ( input_dir == NULL) {
    printf("Usage: %s -l input_index -d output_file -s start -e end\n", argv[0]);
    exit(-1);
  }

  stat(input_dir, &f_stats);
  if (! S_ISDIR(f_stats.st_mode)) {
    printf("Usage: %s -l input_index -d output_file -s start -e end\n", argv[0]);
    printf("Plase index dir\n");
    exit(-1);
  }

  /*
   * 出力先、入力先のファイル設定
   */
  output = (pos_t *) YAP_malloc(sizeof(pos_t));
  output->data = (char *) YAP_malloc(strlen(output_file) + 1);
  strcpy(output->data, output_file);
  output->index = (char *) YAP_malloc(strlen(output_file) + 7);
  sprintf(output->index, "%s_index", output_file);
  output->size = (char *) YAP_malloc(strlen(output_file) + 6);
  sprintf(output->size, "%s_size", output_file);

  inputs = (pos_t **) YAP_malloc(sizeof(pos_t *) * num);
  tmp_size = fprintf(stderr, "%d", end) + strlen(input_dir) + 64;
  for (i = 0;i < num;i++) {
    inputs[i] = (pos_t *) YAP_malloc(sizeof(pos_t));

    inputs[i]->data = (char *) YAP_malloc(tmp_size);
    sprintf(inputs[i]->data, "%s/pos/%d", input_dir, start + i);
    inputs[i]->index = (char *) YAP_malloc(tmp_size);
    sprintf(inputs[i]->index, "%s/pos/%d_index", input_dir, start + i);
    inputs[i]->size = (char *) YAP_malloc(tmp_size);
    sprintf(inputs[i]->size, "%s/pos/%d_size", input_dir, start + i);

    inputs[i]->seek_stop = 0;
  }

  /*
   * 出力先、入力先のファイルを開く
   */
  output->data_fp = fopen(output->data, "w");
  output->index_fp = fopen(output->index, "w");
  output->size_fp = fopen(output->size, "w");

  for (i = 0;i < num;i++) {
    inputs[i]->data_fp = fopen(inputs[i]->data, "r");
    inputs[i]->index_fp = fopen(inputs[i]->index, "r");
    inputs[i]->size_fp = fopen(inputs[i]->size, "r");

    if (inputs[i]->data_fp == NULL ||
	inputs[i]->index_fp == NULL ||
	inputs[i]->size_fp == NULL) {
      inputs[i]->seek_stop = 1;
      seek_stops++;
    }
  }

  /*
   * 削除ファイルを開く
   */
  delete_file = (char *) YAP_malloc(strlen(input_dir) + 16);
  sprintf(delete_file, "%s/deletefile", input_dir);
  delete_fp = fopen(delete_file, "r");

  /*
   * キーワード数を求める
   */
  key_num_file = (char *) YAP_malloc(strlen(input_dir) + 16);
  sprintf(key_num_file, "%s/keywordnum", input_dir);
  key_num_fp = fopen(key_num_file, "r");
  fread(&key_num, sizeof(int), 1, key_num_fp);
  fclose(key_num_fp);

  printf("KEYWORD NUM: %d\n-------------------\n", key_num);


  /*
   * キーワードIDの数だけループする
   */
  new_index = 0;
  for (key_pos = 0;key_pos < key_num;key_pos++) {
    int ret;
    int total_size = 0;
    int *pos, pos_len, pos_i;
    int *new_pos, new_pos_len, new_pos_i;
    unsigned char *bufs = NULL;
    bufs = (unsigned char *) YAP_malloc(1);

    /*
    printf("KEYNUM:%d/%d\n", key_pos, key_num);
*/
    
    if (seek_stops == num) {
      /* 末尾まで来たので中断 */
      break;
    }

    /*
     * DBの数だけくり返す
     */
    for (i = 0;i < num;i++) {
      int size, index;
      unsigned char *buf;
      /*      printf("KEYNUM:%d\tPOS:%d\n", key_pos, i + start);*/

      if (!inputs[i]->seek_stop) {
	ret = fread(&size, sizeof(int), 1, inputs[i]->size_fp);
	if (!ret) {
	  inputs[i]->seek_stop = 1;
	  seek_stops++;
	} else {
	  ret = fread(&index, sizeof(int), 1, inputs[i]->index_fp);
	  if (ret) {
	    /* 読み込み */
	    /*	    printf("size:%d\tindex:%d\n", size, index);*/

	    if (size) {
	      buf = (unsigned char *) YAP_malloc(size);
	      fseek(inputs[i]->data_fp, index, SEEK_SET);
	      fread(buf, 1, size, inputs[i]->data_fp);
	      
	      bufs = (unsigned char *) YAP_realloc(bufs, size + total_size);
	      memcpy(bufs + total_size, buf, size);
	      free(buf);
	      total_size += size;
	    }
	  }
	}
      }
    }

    /*
     * ポシションリストを解凍する
     */
    pos = YAP_Index_8bit_decode(bufs, &pos_len, total_size);
    new_pos = (int *) YAP_malloc(sizeof(int) * pos_len);
    new_pos_len = 0;
    new_pos_i = 0;

    /*    printf("POSLEN: %d/%d\n", total_size, pos_len);*/
    if (pos_len > 0) {
      int fileindex, filepos_len;
      int delete_seek, delete_bit, delete_f;
      int key_pos_seek;
      unsigned char *new_bufs;

      i = 0;
      while (i < pos_len) {
	fileindex = pos[i];
	i++;
	filepos_len = pos[i];
	i++;

	/*
	 * 削除済かどうか調べる
	 */
	delete_f = 0;
	delete_seek = fileindex / 8;
	delete_bit  = fileindex % 8;
	fseek(delete_fp, delete_seek, SEEK_SET);
	fread(&delete_f, 1, 1, delete_fp);
	if (delete_f & (1 << delete_bit)) {
	  /*printf("[D]");*/
	} else {
	  new_pos[new_pos_i] = fileindex;
	  new_pos_i++;
	  new_pos[new_pos_i] = filepos_len;
	  new_pos_i++;
	  memcpy(new_pos + new_pos_i, pos + i, sizeof(int) * filepos_len);
	  new_pos_i += filepos_len;
	}
	i += filepos_len;
	/*printf("{%d/%d}", fileindex, filepos_len);*/
      }

      new_bufs = YAP_Index_8bit_encode(new_pos, new_pos_i, &new_pos_len);
      if (new_pos_len > 0) {
	int ret;

	/*	printf("\nNEW SIZE: %d\n\n\n", new_pos_len);*/

	/*
	 * 保存する
	 */
	key_pos_seek = sizeof(int) * key_pos;
	fseek(output->index_fp, key_pos_seek, SEEK_SET);
	fseek(output->size_fp, key_pos_seek, SEEK_SET);

	ret = fwrite(new_bufs, sizeof(char), new_pos_len, output->data_fp);
	ret = fwrite(&new_index, sizeof(int), 1, output->index_fp);
	ret = fwrite(&new_pos_len, sizeof(int), 1, output->size_fp);

	new_index += new_pos_len;
	free(new_bufs);
      }
      free(pos);
      free(new_pos);
    }

    /*
    for (pos_i = 0;pos_i < pos_len;pos_i++){
      printf("[%d/%d]", pos_i, pos[pos_i]);
    }
    */

    free(bufs);
    if (key_pos > 10 && 0) {
      exit(-1);
    }
  }



  fclose(delete_fp);




  /*
   * 出力先、入力先のファイルを閉じる
   */
  fclose(output->data_fp);
  fclose(output->index_fp);
  fclose(output->size_fp);

  for (i = 0;i < num;i++) {
    fclose(inputs[i]->data_fp);
    fclose(inputs[i]->index_fp);
    fclose(inputs[i]->size_fp);
  }
}


#ifdef AAAA

/*
 *keywordDBからURLをキーに検索をしてレコードIDを取得する
 *前方一致により複数のキーワードIDを返す
 */
int get_keyword_pos(YAPPO_DB_FILES *ydfp, int byte)
{
  DBT key, value;
  DBC *cur;
  int ret;
  int count = 0;
  int size = 0;

  memset(&key, 0,sizeof(DBT));
  memset(&value, 0,sizeof(DBT));

  if (byte == 1) {
    ret = ydfp->pos1byte_db->cursor(ydfp->pos1byte_db, NULL, &cur, 0);
  } else {
    ret = ydfp->pos2byte_db->cursor(ydfp->pos2byte_db, NULL, &cur, 0);
  }
  
  /*最初のレコードをみつける*/
  ret = cur->c_get(cur, &key, &value, DB_FIRST);
  if (ret == 0) {
    /*レコードが有った*/
    while(1){
      /*カーソルを調べる*/
      ret = cur->c_get(cur, &key, &value, DB_NEXT);
      if(ret != 0) {
	/*カーソル終了*/
	printf("END: %d\n", ret);
	break;
      }
      /*一致する*/
      
      {
	int *id = key.data;

	size += value.size;
	count++;

	printf("id: %d/ len %d\n", *id, value.size);

	buf[*id] = (char *) YAP_realloc(buf[*id], buf_len[*id] + value.size);
	memcpy(buf[*id] + buf_len[*id], value.data, value.size);
	buf_len[*id] += value.size;
	printf("buf len %d\n", buf_len[*id]);

      }
    }
  }

  printf("count: %d\n", count);
  printf("size: %d\n", size);
  return ret;
}


/*キーワードのマージ*/
void core (YAPPO_DB_FILES *in_p, YAPPO_DB_FILES *out_p,
	   int start, int end, int byte
	   ) 
{
  int i, ii, ret;
  unsigned long ui;
  YAPPO_DB_FILES *pos;

  char *buf_file, *buf_len_file;
  int buf_fd, buf_len_fd;
  int long_len, char_len;
  int c = 0;

  /*マップファイルの初期化*/
  buf_file = (char *) YAP_malloc(strlen(MAP_TEMPLATE));
  buf_len_file = (char *) YAP_malloc(strlen(MAP_TEMPLATE));
  strcpy(buf_file, MAP_TEMPLATE);
  strcpy(buf_len_file, MAP_TEMPLATE);
  buf_fd = mkstemp(buf_file);
  buf_len_fd = mkstemp(buf_len_file);
  lseek(buf_fd, sizeof(char) * (in_p->total_keywordnum + 1), SEEK_SET);
  lseek(buf_len_fd, sizeof(long) * (in_p->total_keywordnum + 1), SEEK_SET);
  write(buf_fd, &c, sizeof(char));
  write(buf_len_fd, &c, sizeof(char));

  buf = (char **) mmap(0, sizeof(char) * (in_p->total_keywordnum + 1),
			 PROT_WRITE | PROT_READ, MAP_SHARED, buf_fd, 0);
  buf_len = (long *) mmap(0, sizeof(long) * (in_p->total_keywordnum + 1),
			 PROT_WRITE | PROT_READ, MAP_SHARED, buf_len_fd, 0);

  memset(buf, 0, sizeof(char) * (in_p->total_keywordnum + 1));
  memset(buf_len, 0, sizeof(long) * (in_p->total_keywordnum + 1));

  printf("start\n");


  /*
  //入力元の位置情報DBを全部開く
  pos = (YAPPO_DB_FILES *) YAP_malloc(sizeof(YAPPO_DB_FILES) * (end - start + 1));
  for (i = start; i <= end; i++) {
    int num = i - start;
    memcpy(pos + num, in_p, sizeof(YAPPO_DB_FILES));
    YAP_Db_pos_open(pos + num, num);
    printf("%d\n", pos + num);
  }

  for (ui = 0; ui < (unsigned long) in_p->total_keywordnum; ui++) {
    for (ii = start; ii <= end; ii++) {
      DBT key, value;
      int ret;
      YAPPO_DB_FILES *p = pos + ii - start;

      memset(&key, 0,sizeof(DBT));
      memset(&value, 0,sizeof(DBT));

      key.data = &ui;
      key.size = sizeof(long);

      //printf( "byte=%d/ %d\n", byte, ui);
      if (byte == 1) {
	//1byte
	ret = p->pos1byte_db->get(p->pos1byte_db, NULL, &key, &value, 0);
      } else {
	//2byte
	ret = p->pos2byte_db->get(p->pos2byte_db, NULL, &key, &value, 0);
      }
      //printf("ret: %d\n", ret);

    }

    if ( ui % 1000 == 0) {
      printf( "End: %d\n", ui);
    }
  }
  */
  
  for (i = start; i <= end; i++) {
    int ret = YAP_Db_pos_open(in_p, i);
    get_keyword_pos(in_p, 2);
    YAP_Db_pos_close(in_p);    
  }



  close(buf_fd);
  close(buf_len_fd);
  unlink(buf_file);
  unlink(buf_len_file);

  
  /*
  //入力元の位置情報DBを全部閉じる
  for (i = start; i <= end; i++) {
    int num = i - start;
    YAP_Db_pos_close(pos + num);
  }
  */
}


int main(int argc, char *argv[])
{
  YAPPO_DB_FILES in_index, out_index;
  int i, ii;
  int start, end;
  DIR *dir;
  struct stat f_stats;

  start = end = 0;
  memset(&in_index, 0, sizeof(YAPPO_DB_FILES));
  memset(&out_index, 0, sizeof(YAPPO_DB_FILES));

  /*
   *オプションを取得
   */
  if (argc > 1) {
    i = 1;
    while (1) {
      if ( argc == i)
	break;

      if (! strcmp(argv[i], "-s")) {
	/*まとめるDBの始め*/
	i++;
	if (argc == i)
	  break;
	start = atoi(argv[i]);
      } else if (! strcmp(argv[i], "-e")) {
	/*まとめるDBの終わり*/
	i++;
	if (argc == i)
	  break;
	end = atoi(argv[i]);
      } else if (! strcmp(argv[i], "-l")) {
	/*入力先を取得*/
	i++;
	if (argc == i)
	  break;
	in_index.base_dir = argv[i];
      } else if (! strcmp(argv[i], "-d")) {
	/*出力先を取得*/
	i++;
	if (argc == i)
	  break;
	out_index.base_dir = argv[i];
      }
      i++;
    }
  }

  /*オプションが指定されていない*/
  if ( in_index.base_dir == NULL || out_index.base_dir == NULL) {
    printf("Usage: %s -l input_index -d output_index -s start -e end\n", argv[0]);
    exit(-1);
  }

  stat(in_index.base_dir, &f_stats);
  if (! S_ISDIR(f_stats.st_mode)) {
    printf("Plase index dir\n");
    exit(-1);
  }

  stat(out_index.base_dir, &f_stats);
  if (! S_ISDIR(f_stats.st_mode)) {
    printf("Plase index dir\n");
    exit(-1);
  }

  in_index.mode = YAPPO_DB_READ;
  out_index.mode = YAPPO_DB_READ;

  /*DBを開く*/
  YAP_Db_filename_set(&in_index);
  YAP_Db_base_open(&in_index);

  core(&in_index, &out_index, start, end, 2);

  YAP_Db_base_close(&in_index);
  exit(0);
}

#endif
