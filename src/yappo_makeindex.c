/*
 *
 *インデクサ
 *
 */
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "yappo_db.h"
#include "yappo_index.h"
#include "yappo_index_pos.h"
#include "yappo_index_filedata.h"
#include "yappo_index_deletefile.h"
#include "yappo_alloc.h"
#include "yappo_ngram.h"
#include "yappo_minibtree.h"

#define GZ_BUF_SIZE 1024
#define MAX_STACK_SIZE 2040

/*ファイル毎のキーワードインデックス情報を保存する構造体*/
typedef struct{
  FILEDATA filedata;
  NGRAM_LIST * ngram;
  int fileindex;
}INDEX_STACK;


/*minibtreeのdataメンバに保存するキーワードインデックスの一次データ用の構造体*/
typedef struct{ 
  int keyword_total_num;
  int keyword_docs_num;
  int data_len;
  char *data;/*エンコードされた出現位置情報*/
}BTREE_DATA;


/*
 *URL情報を登録する
 */
int add_url_dict(INDEX_STACK *index_stack, int stack_count, YAPPO_DB_FILES *ydfp)
{
  int i;
  int len, seek;
  double score = 1.0;

  for (i = 0; i < stack_count; i++) {
    if (YAP_Index_Deletefile_get(ydfp, index_stack[i].fileindex) == 0) {
      /*削除済なのでキーワードの索引は行なわない*/
      continue;
    }
    YAP_Index_Filedata_put(ydfp, index_stack[i].fileindex, &index_stack[i].filedata);      

    seek = sizeof(int) * index_stack[i].fileindex;

    fseek(ydfp->size_file, seek, SEEK_SET);
    fwrite(&(index_stack[i].filedata.size), sizeof(int), 1, ydfp->size_file);

    fseek(ydfp->domainid_file, seek, SEEK_SET);
    fwrite(&(index_stack[i].filedata.domainid), sizeof(int), 1, ydfp->domainid_file);

    fseek(ydfp->score_file, sizeof(double) * index_stack[i].fileindex, SEEK_SET);
    fwrite(&score, sizeof(double), 1, ydfp->score_file);

    fseek(ydfp->filekeywordnum_file, seek, SEEK_SET);
    fwrite(&(index_stack[i].filedata.keyword_num), sizeof(int), 1, ydfp->filekeywordnum_file);

    fseek(ydfp->urllen_file, seek, SEEK_SET);
    len = strlen(index_stack[i].filedata.url);
    fwrite(&len, sizeof(int), 1, ydfp->urllen_file);
  }

}


/*
 *キーワードを辞書に登録する
 *主にbtreeからDBに登録し、btreeのメモリを解放する
 *再帰的に処理される
 */
int add_keyword_dict_set(MINIBTREE *btree_this, YAPPO_DB_FILES *ydfp)
{
  unsigned long keyword_id;
  int keyword_total_num, keyword_docs_num;
  unsigned char *postings_buf;

  if (btree_this == NULL) {
    return 1;
  }

  if (btree_this->top == NULL) {
    /*rootノード*/
  } else {
    /*キーワードの登録開始*/
    keyword_id = 0;

    
    /*辞書に登録されているか調べる*/
    if (YAP_Index_get_keyword(ydfp, btree_this->key, &keyword_id)) {
      /*登録されていない
	キーワードの新規登録
      */
        ydfp->total_keywordnum++;
	keyword_id = ydfp->total_keywordnum;

	keyword_total_num = ((BTREE_DATA *) btree_this->data)->keyword_total_num;
	keyword_docs_num = ((BTREE_DATA *) btree_this->data)->keyword_docs_num;

	fseek(ydfp->keyword_totalnum_file, sizeof(int) * keyword_id, SEEK_SET);
	fwrite(&keyword_total_num, sizeof(int), 1, ydfp->keyword_totalnum_file);
	fseek(ydfp->keyword_docsnum_file, sizeof(int) * keyword_id, SEEK_SET);
	fwrite(&keyword_docs_num, sizeof(int), 1, ydfp->keyword_docsnum_file);

	YAP_Index_put_keyword(ydfp, btree_this->key, &keyword_id);

	/*出現リストの新規登録*/
	YAP_Index_Pos_put(ydfp, keyword_id, ((BTREE_DATA *) btree_this->data)->data, ((BTREE_DATA *) btree_this->data)->data_len);

	/*出現位置ファイルの登録数を加算*/
	ydfp->pos_num++;
      } else {
	/*登録されていた*/
	int ret;
	int postings_buf_len;

	/*数値の加算*/
	fseek(ydfp->keyword_totalnum_file, sizeof(int) * keyword_id, SEEK_SET);
	fread(&keyword_total_num, sizeof(int), 1, ydfp->keyword_totalnum_file);
	fseek(ydfp->keyword_docsnum_file, sizeof(int) * keyword_id, SEEK_SET);
	fread(&keyword_docs_num, sizeof(int), 1, ydfp->keyword_docsnum_file);

	keyword_total_num += ((BTREE_DATA *) btree_this->data)->keyword_total_num;
	keyword_docs_num += ((BTREE_DATA *) btree_this->data)->keyword_docs_num;

	fseek(ydfp->keyword_totalnum_file, sizeof(int) * -1, SEEK_CUR);
	fwrite(&keyword_total_num, sizeof(int), 1, ydfp->keyword_totalnum_file);
	fseek(ydfp->keyword_docsnum_file, sizeof(int) * -1, SEEK_CUR);
	fwrite(&keyword_docs_num, sizeof(int), 1, ydfp->keyword_docsnum_file);

	YAP_Index_put_keyword(ydfp, btree_this->key, &keyword_id);

	/*出現リストを取得*/
	ret = YAP_Index_Pos_get(ydfp, keyword_id, &postings_buf, &postings_buf_len);

	if (ret) {
	  /*出現リストの新規登録*/
	  YAP_Index_Pos_put(ydfp,  keyword_id, ((BTREE_DATA *) btree_this->data)->data, ((BTREE_DATA *) btree_this->data)->data_len);

	  /*出現位置ファイルの登録数を加算*/
	  ydfp->pos_num++;
	} else {
	  /*出現リストの末尾に追加*/
	  postings_buf = (unsigned char *) YAP_realloc(postings_buf,
						       ((BTREE_DATA *) btree_this->data)->data_len + postings_buf_len);
	  memcpy(postings_buf + postings_buf_len, ((BTREE_DATA *) btree_this->data)->data, ((BTREE_DATA *) btree_this->data)->data_len);
	  YAP_Index_Pos_put(ydfp, keyword_id, postings_buf, ((BTREE_DATA *) btree_this->data)->data_len + postings_buf_len);
	  
	  free(postings_buf);
	  postings_buf = NULL;
	}
      }

  }

  add_keyword_dict_set(btree_this->left, ydfp);
  add_keyword_dict_set(btree_this->right, ydfp);

  /*
   *メモリクリア
   */
  if (btree_this->data != NULL) {
    if (((BTREE_DATA *) btree_this->data)->data != NULL) {
      free(((BTREE_DATA *) btree_this->data)->data);
      ((BTREE_DATA *) btree_this->data)->data = NULL;
    }

    free(btree_this->data);
    btree_this->data = NULL;
  }
  if (btree_this->key != NULL) {
    free(btree_this->key);
    btree_this->key = NULL;
  }

  if (btree_this->top != NULL) {
    if (btree_this->top->left == btree_this) {
      btree_this->top->left = NULL;
    }
    if (btree_this->top->right == btree_this) {
      btree_this->top->right = NULL;
    }
    btree_this->top = NULL;
    free(btree_this);
    btree_this = NULL;
  }

  return 0;
}

/*
 *キーワードを辞書に登録する
 *主にbtreeにINDEX_STACKを整形
 */
int add_keyword_dict(INDEX_STACK *index_stack, int stack_count, YAPPO_DB_FILES *ydfp)
{
  int i, ii;
  int *index_list, index_last;
  NGRAM_LIST *this, *next;
  unsigned char *encode_8bit = NULL;
  MINIBTREE *btree_root, *btree_add, *btree_result;
  int btree_id = 0;
  int encode_8bit_len;


  ydfp->pos_fileindex_start_w = ydfp->pos_fileindex_end_w = 0;

  /*
   *追加するキーワードインデックスを作成する
   */

  btree_root = YAP_Minibtree_init();
  for (i = 0; i < stack_count; i++) {

    if (YAP_Index_Deletefile_get(ydfp, index_stack[i].fileindex) == 0) {
      /*削除済なのでキーワードの索引は行なわない*/
      continue;
    }

    /*キーワード出現位置ファイル向けの処理fileindeの先頭と終点を記録*/
    if (ydfp->pos_fileindex_start_w == 0) {
      ydfp->pos_fileindex_start_w = index_stack[i].fileindex;
    }
    if (ydfp->pos_fileindex_end_w < index_stack[i].fileindex) {
      ydfp->pos_fileindex_end_w = index_stack[i].fileindex;
    }

    next= index_stack[i].ngram->next;
    /*printf("insert keyword: %d\n", index_stack[i].fileindex);*/
    while (1) {

      if ( next == NULL) {
	break;
      }
      this = next;
      next = this->next;

      /*出現位置リストの整形 出現位置リストを差分表記にする*/
      index_list = (int *) YAP_malloc(sizeof(int) * (this->index_count + 2));
      index_last = 0;
      for (ii = 0; ii < this->index_count; ii++) {
	/*index_list[ii+2] = this->index[ii] - index_last + 1;//なぜ+1してしまったのかが分からない*/

	index_list[ii+2] = this->index[ii] - index_last;
	index_last = this->index[ii];
      }
      
      /*文書idと文書中の単語出現数の記録*/
      index_list[0] = index_stack[i].fileindex;
      index_list[1] = this->index_count;

      /*8bitエンコード*/
      encode_8bit = (unsigned char *) YAP_Index_8bit_encode(index_list, this->index_count + 2, &encode_8bit_len);


      /*
       *キーワードインデックスのbtreeへの登録
       */
      btree_result = YAP_Minibtree_search( btree_root, this->keyword);
      if (btree_result == NULL) {
	/*登録されていない
	//ノードを新規作成
	*/
	btree_id++;
	btree_add = NULL;
	btree_add = YAP_Minibtree_init();
	btree_add->id = btree_id;
	btree_add->key = (char *) YAP_malloc(strlen(this->keyword) + 1);
	strcpy( btree_add->key, this->keyword);

	/*データの登録*/
	btree_add->data = (BTREE_DATA *) YAP_malloc(sizeof(BTREE_DATA));
	((BTREE_DATA *) btree_add->data)->keyword_total_num = this->index_count;
	((BTREE_DATA *) btree_add->data)->keyword_docs_num = 1;

	((BTREE_DATA *) btree_add->data)->data_len = encode_8bit_len;

	((BTREE_DATA *) btree_add->data)->data = (unsigned char *) YAP_malloc(((BTREE_DATA *) btree_add->data)->data_len + 1);

	memcpy(((BTREE_DATA *) btree_add->data)->data, encode_8bit, encode_8bit_len);

	/*ノードに登録*/
	YAP_Minibtree_add( btree_root, btree_add);

      } else {
	/*登録されていた
	//数値の加算
	*/
	((BTREE_DATA *) btree_result->data)->keyword_total_num += this->index_count;
	((BTREE_DATA *) btree_result->data)->keyword_docs_num++;

	/*出現リストの末尾に追加*/
	((BTREE_DATA *) btree_result->data)->data =
	  (unsigned char *) YAP_realloc(((BTREE_DATA *) btree_result->data)->data,
					((BTREE_DATA *) btree_result->data)->data_len + encode_8bit_len + 1);

	memcpy(((BTREE_DATA *) btree_result->data)->data + ((BTREE_DATA *) btree_result->data)->data_len
	       , encode_8bit, encode_8bit_len);

	((BTREE_DATA *) btree_result->data)->data_len += encode_8bit_len;
      }

      free(encode_8bit);
      encode_8bit = NULL;
      free(index_list);
      index_list = NULL;
    }
  }

  /*
   *dbファイルを開く
   */
  {
    int pos_id = ydfp->total_filenum / MAX_POS_URL;
    printf("open db\n");
    YAP_Db_pos_open(ydfp, pos_id);
    printf("open db2\n");
  }

  /*登録開始*/
  printf("add_keyword_dict_set START\n");
  add_keyword_dict_set(btree_root, ydfp);
  printf("add_keyword_dict_set END\n");

  /*木のメモリクリア*/
  free(btree_root);
  btree_root = NULL;

  /*
   *DBファイルを閉じる
   */
  YAP_Db_pos_close(ydfp);

  printf("add_dict end\n");

}


/*
 *入力されたファイルをgz展開してインデックスを行なう
 */
int indexer_core(char *gz_filepath, time_t gz_file_mtime, YAPPO_DB_FILES *ydfp)
{
  char *gz_in, *gz_out, *line_buf;
  gzFile *gz_file;
  INDEX_STACK index_stack[MAX_STACK_SIZE];
  FILEDATA old_filedata;
  int stack_count = 0;/*メモリに確保されている処理済のurl数*/

  gz_in = (char *) YAP_malloc(GZ_BUF_SIZE);
  line_buf = (char *) YAP_malloc(GZ_BUF_SIZE);

  /*gzファイルを開く*/
  gz_file = gzopen(gz_filepath, "r"); 
  while (! gzeof(gz_file)) { 
    memset(gz_in, 0, GZ_BUF_SIZE);/*ゼロクリア*/

    /*読みこみ*/
    if ( gzgets(gz_file, gz_in, GZ_BUF_SIZE) == Z_NULL) {
      /*エラー*/
      break;
    }
    /*行バッファにコピーする*/
    if (line_buf[0] != '\0') {
      /*追記*/
      int new_buf_len = strlen(line_buf) + strlen(gz_in) + 1;
      line_buf = (char *) YAP_realloc(line_buf, new_buf_len);
    }

    line_buf = (char *) strcat(line_buf, gz_in);

    if ( gz_in[GZ_BUF_SIZE-2] == '\n' || strlen(gz_in) < GZ_BUF_SIZE - 1) {
      /*一行分取得できたので、ファイル毎の処理を行なう*/
      char *url, *command, *title, *body_size_c, *body, *tokp_start, *tokp_end, *body_tmp;
      int line_buf_len = strlen(line_buf);
      int domainid = 0;
      int body_size;
      int fileindex;

      /*最後の\nを削除する*/
      line_buf[line_buf_len-1] = '\0';

      /*URLタイトル本文の取得
      //url\tcommand\ttitle\tbody_size\tbody\n
      */
      tokp_start = line_buf;
      tokp_end = line_buf;

      url = tokp_start;
      while (*tokp_end) {
	if (*tokp_end == '\t') {
	  *tokp_end = '\0';
	  tokp_end++;
	  tokp_start = tokp_end;
	  break;
	}
	tokp_end++;
      }

      command = tokp_start;
      while (*tokp_end) {
	if (*tokp_end == '\t') {
	  *tokp_end = '\0';
	  tokp_end++;
	  tokp_start = tokp_end;
	  break;
	}
	tokp_end++;
      }

      title = tokp_start;
      while (*tokp_end) {
	if (*tokp_end == '\t') {
	  *tokp_end = '\0';
	  tokp_end++;
	  tokp_start = tokp_end;
	  break;
	}
	tokp_end++;
      }

      body_size_c = tokp_start;
      while (*tokp_end) {
	if (*tokp_end == '\t') {
	  *tokp_end = '\0';
	  tokp_end++;
	  tokp_start = tokp_end;
	  break;
	}
	tokp_end++;
      }
      body_size = atoi(body_size_c);

      body = tokp_start;


      if (body_size < 24 || body_size > 102400) {
	/*ファイルサイズが小さすぎるか大きすぎるので索引に加えない
	
	  //行バッファをクリア
	  */
	  free(line_buf);
	  body = NULL;
	  line_buf = NULL;
	  line_buf = (char *) YAP_malloc(GZ_BUF_SIZE);
	  continue;
      }


      /*bodyだけメモリ確保*/
      body_tmp = (char *) YAP_malloc(strlen(body) + 1);
      strcpy(body_tmp, body);
      body = body_tmp;

      /*titleを牽引に加える*/
      if (strlen(title)) {
	body = (char *) YAP_realloc(body, strlen(body) + strlen(title) + 2);
	strcat(body, " ");
	strcat(body, title);
      }
      /*URLを牽引に加える*/
      if (strlen(url)) {
	body = (char *) YAP_realloc(body, strlen(body) + strlen(url) + 2);
	strcat(body, " ");
	strcat(body, url);
      }
      /*
      //printf("%s|/|%s|/|%s|/|%s/%d\n", url, title, command, body_size_c, body_size);
      */

      /*
       *現在のurlが登録済か調べる
       */
      if ( YAP_Index_get_fileindex(ydfp, url, &fileindex) != 0) {
	/*登録されていないので新規登録をする*/
	if (! strcmp( command, "DELETE")) {
	  /*削除扱いなので終了する

	  //行バッファをクリア*/
	  free(body);
	  body = NULL;
	  free(line_buf);
	  line_buf = NULL;
	  line_buf = (char *) YAP_malloc(GZ_BUF_SIZE);

	  continue;
	}
      } else {
	/*登録されていたら、登録されているデータを削除扱いにする*/
	time_t old_mtime;
	int old_body_size, ret;

	/*その前に現在の登録情報の方が新しいかを調べる*/
	ret = YAP_Index_Filedata_get(ydfp, fileindex, &old_filedata);

	if (ret == 0) {
	  old_mtime = old_filedata.lastmod;
	  old_body_size = old_filedata.size;

	  /*メモリの解放&初期化*/
	  YAP_Index_Filedata_free(&old_filedata);
	  
	  if (old_mtime > gz_file_mtime ||
	      (old_mtime == gz_file_mtime && old_body_size == body_size)) {
	    /*現在の登録情報の更新日時が新しいか、本文が同じならスキップ
	    
	    //行バッファをクリア
	    */
	    free(body);
	    body = NULL;
	    free(line_buf);
	    line_buf = NULL;
	    line_buf = (char *) YAP_malloc(GZ_BUF_SIZE);
	    
	    continue;
	  }
	} else {
	  /*現在索引中のデータの場合は、今回の登録をスキップする*/
	  continue;
	}

	/*昔のレコードを削除*/
	YAP_Index_del_fileindex(ydfp, url);
	YAP_Index_Filedata_del(ydfp, fileindex);
	YAP_Index_Deletefile_put(ydfp, fileindex);
      }
    
      if (! strcmp( command, "DELETE")) {
	/*削除処理なので、登録処理を行なわない*/
	printf("delete\n");
      } else {
	/*登録処理*/
	int keyword_num = 0;

	/*urlとfileindexの対応をDBに登録*/
	ydfp->total_filenum++;
	fileindex = ydfp->total_filenum;
	YAP_Index_put_fileindex(ydfp, url, &fileindex);

	/*FILEDATAに追加する*/
	index_stack[stack_count].filedata.url = (char *) YAP_malloc(strlen(url) + 1);
	strcpy(index_stack[stack_count].filedata.url, url);
	index_stack[stack_count].filedata.title = (char *) YAP_malloc(strlen(title) + 1);
	strcpy(index_stack[stack_count].filedata.title, title);
	index_stack[stack_count].filedata.lastmod = gz_file_mtime;
	index_stack[stack_count].filedata.size = body_size;

	/*コメント作成機能は未実装*/
	index_stack[stack_count].filedata.comment = NULL;

	/*その外は未実装*/
	index_stack[stack_count].filedata.other = NULL;
	index_stack[stack_count].filedata.other_len = 0;

	
	domainid = 0;
	if (strlen(url) > 11) {
	  /*ドメインの取得*/
	  if (strncmp(url, "http://", 7) == 0) {
	    int len = 0;
	    char *domain, *domain_p, *domain_s;
	    domain_s = domain_p = url + 7;

	    /* /を探す*/
	    while (*domain_p) {
	      domain_p++;
	      if (*domain_p == '/') {
		len = domain_p - domain_s;
		break;
	      }
	    }

	    if (len > 0) {
	      /*ドメインが見つかった*/
	      domain = (char *) YAP_malloc(len + 1);
	      memcpy(domain, domain_s, len);

	      if (YAP_Index_get_domainindex(ydfp, domain, &domainid)) {
		/*ドメインの新規登録*/
		ydfp->total_domainnum++;
		domainid = ydfp->total_domainnum;
		YAP_Index_put_domainindex(ydfp, domain, &domainid);
	      }
	      free(domain);
	    }
	  }
	}
	index_stack[stack_count].filedata.domainid = domainid;


	/*N-gramで文字列の切り出し*/
	index_stack[stack_count].ngram = YAP_Ngram_tokenize(body, &keyword_num);

	index_stack[stack_count].filedata.keyword_num = keyword_num;

	index_stack[stack_count].fileindex = fileindex;

	stack_count++;

	printf("Stack: %d/%d(%d)\n", stack_count, fileindex, body_size);
      }

      /*行バッファをクリア*/
      free(body);
      body = NULL;
      free(line_buf);
      line_buf = NULL;
      line_buf = (char *) YAP_malloc(GZ_BUF_SIZE);
    }


    if ( stack_count >= MAX_STACK_SIZE) {
      /*スタックの最大数に達したので、DBファイルへの反映を行なう*/
      int i;

      printf("loop end addstert\n");

      add_keyword_dict(index_stack, stack_count, ydfp);
      add_url_dict(index_stack, stack_count, ydfp);
      
      for (i = 0; i < MAX_STACK_SIZE; i++) {

	/*メモリの解放&初期化*/
	if (index_stack[i].filedata.url != NULL) {
	  free(index_stack[i].filedata.url);
	  index_stack[i].filedata.url = NULL;
	}
	if (index_stack[i].filedata.title != NULL) {
	  free(index_stack[i].filedata.title);
	  index_stack[i].filedata.title = NULL;
	}
	if (index_stack[i].filedata.comment != NULL) {
	  free(index_stack[i].filedata.comment);
	  index_stack[i].filedata.comment = NULL;
	}
	if (index_stack[i].filedata.other != NULL) {
	  free(index_stack[i].filedata.other);
	  index_stack[i].filedata.other = NULL;
	}
	index_stack[i].filedata.lastmod = 0;
	index_stack[i].filedata.size = 0;
	index_stack[i].filedata.keyword_num = 0;
	index_stack[i].filedata.other_len = 0;

	YAP_Ngram_List_free(index_stack[i].ngram);
	index_stack[i].ngram = NULL;

	index_stack[i].fileindex = 0;

	stack_count = 0;
      }
 
  printf("SLEEPING\n");

    }

  }
  gzclose(gz_file);

  /*行バッファの解放*/
  if (line_buf != NULL) {
    free(line_buf);
  }

  printf("loop end: stack_count=%d\n", stack_count);

  /*処理仕切れていない分を処理する*/
  {
    int i;
    add_keyword_dict(index_stack, stack_count, ydfp);
    add_url_dict(index_stack, stack_count, ydfp);
    
    for (i = 0; i < stack_count; i++) {

      /*メモリの解放&初期化*/
      if (index_stack[i].filedata.url != NULL) {
	free(index_stack[i].filedata.url);
	index_stack[i].filedata.url = NULL;
      }
      if (index_stack[i].filedata.title != NULL) {
	free(index_stack[i].filedata.title);
	index_stack[i].filedata.title = NULL;
      }
      if (index_stack[i].filedata.comment != NULL) {
	free(index_stack[i].filedata.comment);
	index_stack[i].filedata.comment = NULL;
      }
      if (index_stack[i].filedata.other != NULL) {
	free(index_stack[i].filedata.other);
	index_stack[i].filedata.other = NULL;
      }
      index_stack[i].filedata.lastmod = 0;
      index_stack[i].filedata.size = 0;
      index_stack[i].filedata.keyword_num = 0;
      index_stack[i].filedata.other_len = 0;

      YAP_Ngram_List_free(index_stack[i].ngram);
      index_stack[i].ngram = NULL;

      index_stack[i].fileindex = 0;
    }
    stack_count = 0;
  }

  printf("SLEEPING END\n");

  return 0;
}


int main(int argc, char *argv[])
{
  YAPPO_DB_FILES yappo_db_files;
  int i;
  char *indextext_filepath = NULL;
  char *indextexts_dirpath = NULL;
  DIR *input_dir;
  struct stat f_stats;

  memset(&yappo_db_files, 0, sizeof(YAPPO_DB_FILES));

  /*
   *オプションを取得
   */
  if (argc > 1) {
    i = 1;
    while (1) {
      if ( argc == i)
	break;

      if (! strcmp(argv[i], "-f")) {
	/*入力ファイルを取得*/
	i++;
	if (argc == i)
	  break;
	indextext_filepath = argv[i];
      } else if (! strcmp(argv[i], "-l")) {
	/*入力先を取得*/
	i++;
	if (argc == i)
	  break;
	indextexts_dirpath = argv[i];
      } else if (! strcmp(argv[i], "-d")) {
	/*出力先を取得*/
	i++;
	if (argc == i)
	  break;
	yappo_db_files.base_dir = argv[i];
      }
      i++;
    }
  }

  /*入力ファイルが指定されていない*/
  if (( indextext_filepath == NULL && indextexts_dirpath == NULL) || 
      yappo_db_files.base_dir == NULL) {
    printf("Usage: %s [-f inputfile | -l inputfilesdir] -d outputdir\n", argv[0]);
    exit(-1);
  }

  stat(yappo_db_files.base_dir, &f_stats);
  if (! S_ISDIR(f_stats.st_mode)) {
    printf("Plase dir\n");
    exit(-1);
  }


  /*
   *データベースの準備
   */
  yappo_db_files.mode = YAPPO_DB_WRITE;
  YAP_Db_filename_set(&yappo_db_files);
  YAP_Db_base_open(&yappo_db_files);


  if (indextext_filepath != NULL) {
    /*
    //単一ファイルの処理

    //入力ファイルが存在するか調べる
    */
    stat(indextext_filepath, &f_stats);
    if (! S_ISREG(f_stats.st_mode)) {
      printf("Plase file\n");
      exit(-1);
    }

    /*圧縮ファイルの展開をしつつインデックスを行なう*/
    indexer_core(indextext_filepath, f_stats.st_mtime, &yappo_db_files);
  } else {
    struct dirent *direntp;
    /*ディレクトリ中の.gzファイルを処理*/
    stat(indextexts_dirpath, &f_stats);
    if (! S_ISDIR(f_stats.st_mode)) {
      printf("Plase dir\n");
      exit(-1);
    }

    /*ディレクトリを開く*/
    input_dir = opendir(indextexts_dirpath);
    while ((direntp = readdir(input_dir)) != NULL) {
      char *name = direntp->d_name;
      int len = strlen(name);
      if (name[len-3] == '.' && name[len-2] == 'g' && name[len-1] == 'z') {
	/*.gzファイルのみを処理*/
	indextext_filepath = (char *) YAP_malloc(strlen(indextexts_dirpath) + strlen(name) + 2);
	sprintf(indextext_filepath, "%s/%s", indextexts_dirpath, name);

 	stat(indextext_filepath, &f_stats);
	if (! S_ISREG(f_stats.st_mode)) {
	  printf("Plase file\n");
	  exit(-1);
	}

	indexer_core(indextext_filepath, f_stats.st_mtime, &yappo_db_files);
	printf("%s\n", indextext_filepath);
	free(indextext_filepath);
      }
    }
    closedir(input_dir);
  }


  /*
   *データベースを閉じる
   */
  YAP_Db_base_close(&yappo_db_files);

  exit(0);
}
