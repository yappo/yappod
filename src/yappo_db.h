/*
 *
 *DB���ν�����Ԥʤ�
 *
 */
#ifndef __YAPPO_DB_H__
#define __YAPPO_DB_H__

typedef unsigned long u_long;

#include <sys/types.h>
#include <db.h>
#include <pthread.h>

/* 1�Ĥνи����֥ե�����ǰ���URL�� */
#define MAX_POS_URL 10000


/* DB�򳫤��Ȥ��Υ��ץ���� */
#define YAPPO_DB_WRITE 0
#define YAPPO_DB_READ 1

/* 
 *  �������˻��Ѥ����Ƽ省��å���
*/
typedef struct{ 
  /* ��URL�� */
  unsigned int total_filenum;

  /* ��DOMAIN�� */
  unsigned int total_domainnum;

  /* ������ɿ� */
  unsigned int total_keywordnum;

  /* �������ե����륭��å��� */
  double *score;
  unsigned int score_num;
  pthread_mutex_t score_mutex;

  /* �ե����륵��������å��� */
  int *size;
  unsigned int size_num;
  pthread_mutex_t size_mutex;

  /* URLʸ�����ե����륭��å��� */
  int *urllen;
  unsigned int urllen_num;
  pthread_mutex_t urllen_mutex;

  /* ��URL�Υ�����ɿ��ե����륭��å��� */
  int filekeywordnum_num;
  unsigned int *filekeywordnum;
  pthread_mutex_t filekeywordnum_mutex;

  /* domain id�ե����륭��å��� */
  int *domainid;
  unsigned int domainid_num;
  pthread_mutex_t domainid_mutex;

  /* ���URL�ե����륭��å��� */
  unsigned char *deletefile;
  unsigned int deletefile_num;
  pthread_mutex_t deletefile_mutex;
}YAPPO_CACHE;


/*
 *DB�Υե�����ϥ�ɥ�
 */
typedef struct{
  int  mode;/* db�򳫤��Ȥ��Υ��ץ���� */

  char *base_dir;/* db�δ��ܥǥ��쥯�ȥ� */

  char *keyword_2byte_name, *postings_2byte_name;
  char *keyword_1byte_name, *postings_1byte_name;
  char *filedata_name;

  /* URL/ʸ��ID�б�ɽ */
  char *fileindex;
  char *fileindex_tmp;
  DB *fileindex_db;

  /* domain/domain ID�б�ɽ */
  char *domainindex;
  char *domainindex_tmp;
  DB *domainindex_db;

  /* ����ե�����(2byte) */
  char *key2byte;
  char *key2byte_tmp;
  FILE *key2byte_file;

  /* ����ե�����(1byte) */
  char *key1byte;
  char *key1byte_tmp;
  DB *key1byte_db;

  /* ��URL�� */
  unsigned int total_filenum;
  char *filenum;
  FILE *filenum_file;

  /* ��DOMAIN�� */
  unsigned int total_domainnum;
  char *domainnum;
  FILE *domainnum_file;

  /* ������ɿ� */
  unsigned int total_keywordnum;
  char *keywordnum;
  FILE *keywordnum_file;

  /* ��URL��DOMAIN ID */
  char *domainid;
  FILE *domainid_file;

  /* ��URL�Υ����� */
  char *size;
  FILE *size_file;

  /* ��URL�ΤΥ١��������� */
  char *score;
  FILE *score_file;

  /* ��URL�Υ�����ɿ� */
  char *filekeywordnum;
  FILE *filekeywordnum_file;

  /* URL��Ĺ�� */
  char *urllen;
  FILE *urllen_file;

  /* ������ɤ���и��� */
  char *keyword_totalnum;
  FILE *keyword_totalnum_file;

  /* ������ɤ���и�URL�� */
  char *keyword_docsnum;
  FILE *keyword_docsnum_file;

  /*
   *linklist�ط�
   */
  /* linklist�Ǽ谷�äƤ���URL�� */
  int linklist_num;
  char *linklist;
  FILE *linklist_file;
  char *linklist_size;
  FILE *linklist_size_file;
  char *linklist_index;
  FILE *linklist_index_file;

  /*
   *�и����֥ե�����ط�
   */
  unsigned long pos_num;
  int pos_fileindex_start, pos_fileindex_end;
  /* ����ǥå����������ǻ��Ѥ��� ���߽������fileindex */
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
   *URL�᥿�ǡ����ط�
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
   *�����URL�ե�����
   */
  char *deletefile;
  char *deletefile_tmp;
  FILE *deletefile_file;


  /* �������˻��Ѥ����Ƽ省��å��� */
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
