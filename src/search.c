/*
 *
 *コマンドラインからキーワード検索を行なう
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "yappo_db.h"
#include "yappo_index.h"
#include "yappo_index_filedata.h"
#include "yappo_alloc.h"
#include "yappo_ngram.h"
#include "yappo_search.h"

YAPPO_CACHE yappod_core_cache;

/*
 *検索結果を標示
 */
void search_result_print (YAPPO_DB_FILES *ydfp, SEARCH_RESULT *p)
{
  int i;
  FILEDATA filedata;

  if (p == 0) {
    printf("not found\n");
  } else {
    printf("Hit num: %d\n\n", p->keyword_docs_num);
    for (i = 0; i < p->keyword_docs_num; i++) {
      if (YAP_Index_Filedata_get(ydfp, p->docs_list[i].fileindex, &filedata) == 0) {
	struct tm *last_tm = localtime(&(filedata.lastmod));
	
	printf("----------------------------------------\n");
        printf("List: %d/%d\t[%d]\t(domainid:%d)\n", i, p->keyword_docs_num, p->docs_list[i].fileindex, filedata.domainid);
	printf("%s(SCORE:%.1f)\n", filedata.title, p->docs_list[i].score);
	printf("URL:%s\n", filedata.url);
	printf("(size:%d)(date:%d/%d/%d)\n", filedata.size, last_tm->tm_year + 1900, last_tm->tm_mon + 1, last_tm->tm_mday);
	printf("\n");
	
	YAP_Index_Filedata_free(&filedata);
      }else{
	printf("\nError: %d\n", p->docs_list[i].fileindex);
      }
    }
  }

}

/*メイン*/
int main(int argc, char *argv[])
{
  YAPPO_DB_FILES yappo_db_files;
  int i;
  char **keyword_list = NULL;
  int op = 0;/*0=AND 1=OR*/
  int keyword_list_num = 0;
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

      if (! strcmp(argv[i], "-l")) {
	/*インデックスディレクトリを取得*/
	i++;
	if (argc == i)
	  break;
        yappo_db_files.base_dir = argv[i];
      } else if (! strcmp(argv[i], "-a")) {
	/*AND*/
	op = 0;
      } else if (! strcmp(argv[i], "-o")) {
	/*OR*/
	op = 1;
      } else {
	/*キーワードを取得*/
	keyword_list = (char **) YAP_realloc( keyword_list, sizeof(char *) + keyword_list_num + 1);
	keyword_list[keyword_list_num] = (char *) YAP_malloc( strlen(argv[i]) + 1);
	strcpy( keyword_list[keyword_list_num], argv[i]);
	keyword_list_num++;
      }

      i++;
    }
  }

  stat(yappo_db_files.base_dir, &f_stats);
  if (! S_ISDIR(f_stats.st_mode)) {
    printf("Plase Index Dir: %s\n", yappo_db_files.base_dir);
    exit(-1);
  }

  /*
   *データベースの準備
   */
  YAP_Db_cache_init(&yappod_core_cache);
  yappo_db_files.mode = YAPPO_DB_READ;
  yappo_db_files.cache = &yappod_core_cache;
  YAP_Db_filename_set(&yappo_db_files);
  YAP_Db_base_open(&yappo_db_files);
  YAP_Db_cache_load(&yappo_db_files, &yappod_core_cache);
  YAP_Db_linklist_open(&yappo_db_files);
  
  if(0){
    int *list, list_len;
    /*    YAP_Index_get_keyword2( keyword_list[0], &list, &list_len);*/
    printf("address %d = %d = %d\n", list, list_len, list[0]);
    exit(-1);
  }

  {
    SEARCH_RESULT *result;
    result = YAP_Search(&yappo_db_files, keyword_list, keyword_list_num, 100000000, op);
    /*検索結果内のページ同士のリンク関係によりスコアを可変する*/
    printf("go link\n");
    YAP_Linklist_Score(&yappo_db_files, result);

    /*スコア順ソート*/
    printf("go sort\n");
    YAP_Search_result_sort_score(result);
    search_result_print(&yappo_db_files, result);
    YAP_Search_result_free(result);
    free(result);
  }

  YAP_Db_linklist_close(&yappo_db_files);
  YAP_Db_base_close(&yappo_db_files);

  YAP_Db_cache_destroy(&yappod_core_cache); 

}
