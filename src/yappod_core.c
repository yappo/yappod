/*
  サーバ
  フロントサーバから受け取った検索条件で検索して結果をそのまま返す

*/
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <pthread.h>

#include "yappo_alloc.h"
#include "yappo_search.h"
#include "yappo_db.h"
#include "yappo_index.h"

#define BUF_SIZE 1024
#define PORT 10086

/*
 *スレッド毎の構造
 */
typedef struct{
  int id;
  int socket;
  char *base_dir;
}YAP_THREAD_DATA;


/*スレッドの数*/
#define MAX_THREAD 5

/*キャッシュ*/
YAPPO_CACHE yappod_core_cache;

int count;


void YAP_Error( char *msg){
  printf( "%s\n", msg);
  exit(-1);
}


/*
 *検索結果をシリアライズしてクライアントに返す
 */
void search_result_print (YAPPO_DB_FILES *ydfp, FILE *socket, SEARCH_RESULT *p)
{
  int i;
  unsigned long keyword_id;
  int keyword_total_num, keyword_docs_num;

  if (p == NULL || 0 == p->keyword_docs_num) {
    /*リターンコードを送る*/
    i = 0;
    fwrite(&i, sizeof(int), 1, socket);
  } else {
    /*リターンコードを送る*/
    i = 1;
    fwrite(&i, sizeof(int), 1, socket);

    /*KEYWORDを送る*/
    keyword_id = p->keyword_id;
    keyword_total_num = p->keyword_total_num;
    keyword_docs_num = p->keyword_docs_num;
    fwrite(&keyword_id, sizeof(long), 1, socket);
    fwrite(&keyword_total_num, sizeof(int), 1, socket);
    fwrite(&keyword_docs_num, sizeof(int), 1, socket);

    /*SEARCH_DOCUMENTをまず送る*/
    fwrite(p->docs_list, sizeof(SEARCH_DOCUMENT), p->keyword_docs_num, socket);

    /*posを送る*/
    for (i = 0; i < p->keyword_docs_num; i++) {
      fwrite(p->docs_list[i].pos, sizeof(int), p->docs_list[i].pos_len, socket);
    }
  }

  fflush(socket);
}


/*
 *URLデコードを行なう
 */
char *urldecode (char *p) {
  char *ret, *retp;
  char f, l;

  if (p == NULL) {
    return NULL;
  }

  ret = (char *) YAP_malloc(strlen(p));
  retp = ret;

  while (*p) {

    if (*p != '%') {
      *retp = *p;
      retp++;
      p++;
    } else {
      p++;
      f = tolower(*p);
      p++;
      l = tolower(*p);
      p++;

      if (f >= 'a' && f <= 'f') {
	*retp = ((f - 'a') + 10) * 16;
      } else if (f >= '0' && f <= '9') {
	*retp = (f - '0') * 16;
      } else {
	continue;
      }
      if (l >= 'a' && l <= 'f') {
	*retp += l - 'a' + 10;
      } else if (l >= '0' && l <= '9') {
	*retp += l - '0';
      } else {
	continue;
      }

      retp++;
    }
  }
  *retp = '\0';

  return ret;
}

/*
 *検索処理のメインルーチン
 */
SEARCH_RESULT *search_core (YAPPO_DB_FILES *ydfp, char *dict, int max_size, char *op, char *keyword)
{
  int f_op;
  char **keyword_list;
  char *keyword_encode, *keyp, *keys, *keye;
  char *buf;
  int keyword_list_num = 1;
  int i;

  /*検索条件*/
  if (strcmp(op, "OR") == 0) {
    /*OR*/
    f_op = 1;
  } else {
    /*AND*/
    f_op = 0;
  }

  /*キーワードをリストに分割する
   *&の数を数える
   */
  keyp = keyword;
  while (*keyp) {
    if (*keyp == '&') {
      keyword_list_num++;
    }
    keyp++;
  }
  /*キーワードを分割する*/
  keyword_list = (char **) YAP_malloc(sizeof(char *) * keyword_list_num);
  i = 0;
  keys = keyword;
  keye = keyword;
  while (*keye) {
    if (*keye == '&') {
      buf = (char *) YAP_malloc(keye - keys + 1);
      strncpy(buf, keys, keye - keys);
      keyword_list[i] = urldecode(buf);
      keys = keye + 1;
      i++;
      free(buf);
    }
    keye++;
  }
  /*最後の1ワード*/
  buf = (char *) YAP_malloc(keye - keys + 1);
  strncpy(buf, keys, keye - keys);
  keyword_list[i] = urldecode(buf);
  free(buf);

  printf( "max: %d\n", max_size);

  /*
   *検索を行なう
   */
  return YAP_Search(ydfp, keyword_list, keyword_list_num, max_size, f_op);
}



/*
 *サーバの本体
 */
void thread_server (void *ip) 
{
  struct sockaddr_in *yap_sin;
  YAPPO_DB_FILES yappo_db_files;
  YAP_THREAD_DATA *p = (YAP_THREAD_DATA *) ip;
  
  /*
   *データベースの準備
   */
  memset(&yappo_db_files, 0, sizeof(YAPPO_DB_FILES)); 
  yappo_db_files.base_dir = p->base_dir;
  yappo_db_files.mode = YAPPO_DB_READ;
  yappo_db_files.cache = &yappod_core_cache;

  printf("GO\n");

  while (1) {
    SEARCH_RESULT *result;
    int sockaddr_len = sizeof(yap_sin);
    int accept_socket;
    char *line, *line_buf;
    FILE *socket;
    char *dict, *op, *keyword;/*リクエスト*/
    int buf_size;
    int max_size;
    int ret;

    accept_socket = accept(p->socket, (struct sockaddr *)&yap_sin, &sockaddr_len);
    socket = fdopen(accept_socket, "r+");
    
    if (accept_socket == -1) {
      YAP_Error( "accept error");
    }

    printf("accept: %d\n", p->id);

    while (1) {
      /*永遠と処理を続ける*/
      buf_size = 0;
      fread(&buf_size, sizeof(int), 1, socket);
      if (buf_size == 0) {
	/*このクライアントとの接続を解除する*/
	break;
      }

      printf("OK: %d/%d\n", p->id, socket);
      
      
      /*クライアントからリクエストを受け取る*/
      buf_size = 0;
      fread(&buf_size, sizeof(int), 1, socket);
      printf("SIZE[%d]: %d\n", p->id, buf_size);
      dict = (char *) YAP_malloc(buf_size + 1);
      fread(dict, sizeof(char), buf_size, socket);
      
      buf_size = 0;
      fread(&buf_size, sizeof(int), 1, socket);
      op = (char *) YAP_malloc(buf_size + 1);
      fread(op, sizeof(char), buf_size, socket);
      
      buf_size = 0;
      fread(&buf_size, sizeof(int), 1, socket);
      keyword = (char *) YAP_malloc(buf_size + 1);
      fread(keyword, sizeof(char), buf_size, socket);
      
      fread(&max_size, sizeof(int), 1, socket);
      
      printf("ok:%d: %s/%d/%s/%s\n", p->id, dict, max_size, op, keyword);

      YAP_Db_filename_set(&yappo_db_files);
      YAP_Db_base_open(&yappo_db_files);
      /*キャッシュ読みこみ*/
      YAP_Db_cache_load(&yappo_db_files, &yappod_core_cache);
      
      result = search_core(&yappo_db_files, dict, max_size, op, keyword);

      /*結果出力*/
      search_result_print(&yappo_db_files, socket, result);
    
      if (result != NULL) {
	YAP_Search_result_free(result);
	free(result);
      }
      free(dict); free(op); free(keyword);
      fflush(stdout);

      YAP_Db_base_close(&yappo_db_files);
    }

    fflush(stdout);
    fclose(socket);
    close(accept_socket);
  }
}

void start_deamon_thread(char *indextexts_dirpath) 
{
  int sock_optval = 1;
  int yap_socket;
  struct sockaddr_in yap_sin;
  int i;
  pthread_t *pthread;
  YAP_THREAD_DATA *thread_data;

  /*ソケットの作成*/
  yap_socket = socket( AF_INET, SOCK_STREAM, 0);
  if (yap_socket == -1)
    YAP_Error( "socket open error");

  /*ソケットの設定*/
  if (setsockopt(yap_socket, SOL_SOCKET, SO_REUSEADDR,
		&sock_optval, sizeof(sock_optval)) == -1) {
    YAP_Error( "setsockopt error");
  }

  /*bindする*/
  yap_sin.sin_family = AF_INET;
  yap_sin.sin_port = htons(PORT);
  yap_sin.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(yap_socket, (struct sockaddr *)&yap_sin, sizeof(yap_sin)) < 0) {
    YAP_Error( "bind error");
  }

  /*listen*/
  if (listen(yap_socket, SOMAXCONN) == -1) {
    YAP_Error( "listen error");
  }

  /*スレッドの準備*/
  pthread = (pthread_t *) YAP_malloc(sizeof(pthread_t) * MAX_THREAD);
  thread_data  = (YAP_THREAD_DATA *) YAP_malloc(sizeof(YAP_THREAD_DATA) * MAX_THREAD);
  for (i = 0; i < MAX_THREAD; i++) {

    /*起動準備*/
    thread_data[i].id = i;
    thread_data[i].base_dir = (char *) YAP_malloc(strlen(indextexts_dirpath) + 1);
    strcpy(thread_data[i].base_dir, indextexts_dirpath);
    thread_data[i].socket = dup(yap_socket);
    

    /*thread_server(dup(yap_socket), &yap_sin);*/
    printf( "start: %d:%s\n", i, thread_data[i].base_dir);
    pthread_create(&(pthread[i]), NULL, (void *) thread_server, (void *) &(thread_data[i]));

    /*    pthread_join(pthread[i], NULL); 
    //pthread_join(t1, NULL);
    */
    printf( "GO: %d\n", i);
  }

  /*メインループ*/
  while(1){
    sleep(120);
  }
  printf("end\n");
}



int main(int argc, char *argv[])
{
  int i, pid;
  char *indextexts_dirpath = NULL;
  struct stat f_stats;


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
	indextexts_dirpath = argv[i];
      }

      i++;
    }
  }

  stat(indextexts_dirpath, &f_stats);
  if (! S_ISDIR(f_stats.st_mode)) {
    printf("Plase Index Dir: %s\n", indextexts_dirpath);
    exit(-1);
  }

  count = 0;

  printf("Start\n");

  /*
   *デーモン化
   */
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);
  stdout = fopen("./core.log", "a");
  stderr = fopen("./core.error", "a");
  pid = fork();
  if (pid) {
    FILE *pidf = fopen("./core.pid", "w");
    fprintf(pidf, "%d", pid);
    fclose(pidf);
    exit(0);
  }

  /*
   *SIGPIPEを無視
   */
  signal(SIGPIPE, SIG_IGN);


  YAP_Db_cache_init(&yappod_core_cache);

  start_deamon_thread(indextexts_dirpath);

  YAP_Db_cache_destroy(&yappod_core_cache); 
}


