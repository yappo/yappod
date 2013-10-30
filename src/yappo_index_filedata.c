/*
 *URL�᥿�ǡ�����谷��
 */

#include "yappo_index_filedata.h"
#include "yappo_alloc.h"

/*
 *fileindex�򥭡��˸����򤷤�FILEDATA�����
 */
int YAP_Index_Filedata_get(YAPPO_DB_FILES *ydfp, int fileindex, FILEDATA *filedata)
{
  int ret;
  int filedata_size, filedata_index;
  int seek;
  int str_len;
  unsigned char *buf, *bufp;

  if (ydfp->total_filenum < fileindex) {
    /*�оݤȤʤ�ID��¸�ߤ��Ƥ��ʤ�*/
    return -1;
  }

  seek = sizeof(int) * fileindex;

  /*���������ɤߤ���*/
  fseek(ydfp->filedata_size_file, seek, SEEK_SET);
  ret = fread(&filedata_size, sizeof(int), 1, ydfp->filedata_size_file);
  if (ret == 0) {
    return -1;
  }

  if (filedata_size == 0) {
    /*��������0�ʤΤ���Ͽ����Ƥ��ʤ�*/
    return -1;
  }

  /*index���ɤߤ���*/
  fseek(ydfp->filedata_index_file, seek, SEEK_SET);
  ret = fread(&filedata_index, sizeof(int), 1, ydfp->filedata_index_file);
  if (ret == 0) {
    return -1;
  }

  /*�ǡ������ɤߤ���*/
  buf = (unsigned char *) YAP_malloc(filedata_size);
  fseek(ydfp->filedata_file, filedata_index, SEEK_SET);
  ret = fread(buf, 1, filedata_size, ydfp->filedata_file);
  if (ret != filedata_size) {
    free(buf);
    buf == NULL;
    return -1;
  }

  memset(filedata, 0, sizeof(FILEDATA));

  bufp = buf;

  /*FIELDATA�򥷥ꥢ�饤������
   * url\0title\0comment\0size\0keyword_num\0lastmod\0domainid\0other_len\0other
   */
  str_len = *((size_t *) bufp);
  bufp += sizeof(size_t);
  if (str_len > 0) {
    filedata->url = (char *) YAP_malloc(str_len + 1);
    memcpy(filedata->url, bufp, str_len);
    bufp += str_len;
  } else {
    filedata->url = NULL;
  }

  str_len = *((size_t *) bufp);
  bufp += sizeof(size_t);
  if (str_len > 0) {
    filedata->title = (char *) YAP_malloc(str_len + 1);
    memcpy(filedata->title, bufp, str_len);
    bufp += str_len;
  } else {
    filedata->title = NULL;
  }

  str_len = *((size_t *) bufp);
  bufp += sizeof(size_t);
  if (str_len > 0) {
    filedata->comment = (char *) YAP_malloc(str_len + 1);
    memcpy(filedata->comment, bufp, str_len);
    bufp += str_len;
  } else {
    filedata->comment = NULL;
  }

  filedata->size = *((int *) bufp);
  bufp += sizeof(int);
  filedata->keyword_num = *((int *) bufp);
  bufp += sizeof(int);
  filedata->lastmod = *((time_t *) bufp);
  bufp += sizeof(time_t);
  filedata->domainid = *((int *) bufp);
  bufp += sizeof(int);

  filedata->other_len = *((int *) bufp);
  bufp += sizeof(int);
  if (filedata->other_len > 0) {
    filedata->other = (char *) YAP_malloc(filedata->other_len + 1);
    memcpy(filedata->other, bufp, filedata->other_len);
    bufp += filedata->other_len;
  } else {
    filedata->other = NULL;
  }

  free(buf);

  return 0;
}

/*
 *fileindex�򥭡��˸����򤷤�FILEDATA������
 */
int YAP_Index_Filedata_put(YAPPO_DB_FILES *ydfp, int fileindex, FILEDATA *filedata)
{
  int filedata_index;
  int seek;
  unsigned char *buf, *bufp;
  int buf_len = 0;

  if (ydfp->mode == YAPPO_DB_READ) {
    /*�ɤߤ��ߥ⡼�ɤǤϥ��顼*/
    return -1;
  }


  /*FIELDATA�򥷥ꥢ�饤������
   *url\0title\0comment\0size\0keyword_num\0lastmod\0domainid\0other_len\0other
   */

  buf_len += sizeof(size_t);
  if (filedata->url != NULL) {
    buf_len += strlen(filedata->url);
  }
  buf_len += sizeof(size_t);
  if (filedata->title != NULL) {
    buf_len += strlen(filedata->title);
  }
  buf_len += sizeof(size_t);
  if (filedata->comment != NULL) {
    buf_len += strlen(filedata->comment);
  }
  buf_len += sizeof(filedata->size) + sizeof(filedata->keyword_num) + sizeof(filedata->lastmod)
     + sizeof(filedata->domainid) + sizeof(filedata->other_len);
  if (filedata->other != NULL) {
    buf_len += filedata->other_len;
  }

  buf = (unsigned char *) YAP_malloc(buf_len);
  bufp = buf;

  if (filedata->url != NULL) {
    *((size_t *) bufp) = strlen(filedata->url);
    bufp += sizeof(size_t);
    strcpy(bufp, filedata->url);
    bufp += strlen(filedata->url);
  } else {
    *((size_t *) bufp) = 0;
    bufp += sizeof(size_t);
  }
  if (filedata->title != NULL) {
    *((size_t *) bufp) = strlen(filedata->title);
    bufp += sizeof(size_t);
    strcpy(bufp, filedata->title);
    bufp += strlen(filedata->title);
  } else {
    *((size_t *) bufp) = 0;
    bufp += sizeof(size_t);
  }
  if (filedata->comment != NULL) {
    *((size_t *) bufp) = strlen(filedata->comment);
    bufp += sizeof(size_t);
    strcpy(bufp, filedata->comment);
    bufp += strlen(filedata->comment);
  } else {
    *((size_t *) bufp) = 0;
    bufp += sizeof(size_t);
  }

  *((int *) bufp) = filedata->size;
  bufp += sizeof(int);
  *((int *) bufp) = filedata->keyword_num;
  bufp += sizeof(int);
  *((time_t *) bufp) = filedata->lastmod;
  bufp += sizeof(time_t);
  *((int *) bufp) = filedata->domainid;
  bufp += sizeof(int);

  *((int *) bufp) = filedata->other_len;
  bufp += sizeof(int);
  if ( filedata->other != NULL) {
    strcpy(bufp, filedata->other);
  }

  /*��Ͽ*/

  seek = sizeof(int) * fileindex;

  /*�������ν񤭤���*/
  fseek(ydfp->filedata_size_file, seek, SEEK_SET);
  fwrite(&buf_len, sizeof(int), 1, ydfp->filedata_size_file);

  /*�ǡ����ν񤭤���*/
  fseek(ydfp->filedata_file, 0L, SEEK_END);
  filedata_index = ftell(ydfp->filedata_file);
  fwrite(buf, 1, buf_len, ydfp->filedata_file);

  /*index�ν񤭤���*/
  fseek(ydfp->filedata_index_file, seek, SEEK_SET);
  fwrite(&filedata_index, sizeof(int), 1, ydfp->filedata_index_file);

  free(buf);

  return 0;
}

/*
 *fileindex�򥭡��˸����򤷤�FILEDATA����
 */
int YAP_Index_Filedata_del(YAPPO_DB_FILES *ydfp, int fileindex)
{
  int c = 0;
  int seek;

  if (ydfp->mode == YAPPO_DB_READ) {
    /*�ɤߤ��ߥ⡼�ɤǤϥ��顼*/
    return -1;
  }

  seek = sizeof(int) * fileindex;

  /*�������ν񤭤���*/
  fseek(ydfp->filedata_size_file, seek, SEEK_SET);
  fwrite(&c, sizeof(int), 1, ydfp->filedata_size_file);

  /*index�ν񤭤���*/
  fseek(ydfp->filedata_index_file, seek, SEEK_SET);
  fwrite(&c, sizeof(int), 1, ydfp->filedata_index_file);

  return 0;
}

/*
 *FILEDATA�Υ���򥯥ꥢ����
 */
int YAP_Index_Filedata_free (FILEDATA *p)
{
  if (p->url != NULL) {
    free(p->url);
    p->url = NULL;
  }
  if (p->title != NULL) {
    free(p->title);
    p->title = NULL;
  }
  if (p->comment != NULL) {
    free(p->comment);
    p->comment = NULL;
  }
  if (p->other != NULL) {
    free(p->other);
    p->other = NULL;
  }
  p->lastmod = 0;
  p->size = 0;
  p->keyword_num = 0;
  p->other_len = 0;
}


/*
 *�᥿�ե�����Τ���������Ԥʤ�
 *�᥿�ե������open���Ƥ���ץ�����̵�����Ȥ�����
 */
int YAP_Index_Filedata_gc(YAPPO_DB_FILES *ydfp, char *filedata, char *filedata_size, char *filedata_index)
{
  int i;
  int seek, index, index_tmp, size, tmp;
  char *filedata_tmp, *filedata_index_tmp;
  FILE *filedata_file, *filedata_size_file, *filedata_index_file;
  FILE *filedata_tmp_file, *filedata_index_tmp_file;
  int buf_len = 0;
  char *buf = NULL;

  printf("Start YAP_Index_Filedata_gc\n");

  /*����ե�����̾�κ���*/
  filedata_tmp = (char *) YAP_malloc(strlen(filedata) + 5);
  sprintf(filedata_tmp, "%s_tmp", filedata);
  filedata_index_tmp = (char *) YAP_malloc(strlen(filedata_index) + 5);
  sprintf(filedata_index_tmp, "%s_tmp", filedata_index);

  /*�ե�����򳫤�*/
  filedata_file = fopen(filedata, "r");
  filedata_size_file = fopen(filedata_size, "r");
  filedata_index_file = fopen(filedata_index, "r");
  filedata_tmp_file = fopen(filedata_tmp, "w");
  filedata_index_tmp_file = fopen(filedata_index_tmp, "w");


  fseek(filedata_size_file, sizeof(int), SEEK_SET);
  fseek(filedata_index_file, sizeof(int), SEEK_SET);
  fseek(filedata_index_tmp_file, sizeof(int), SEEK_SET);

  /*���־���Υ��ԡ�*/
  for (i = 1; i <= ydfp->total_filenum; i++) {
    seek = sizeof(int) * i;

    /*���������ɤߤ���*/
    fread(&size, sizeof(int), 1, filedata_size_file);

    if (size > 0) {
      /*��Ͽ��ͭ��*/

      /*index���ɤߤ���*/
      fseek(filedata_index_file, seek, SEEK_SET);
      fread(&index, sizeof(int), 1, filedata_index_file);

      /*�ǡ������ɤߤ���*/
      if (buf_len < size) {
	buf = (char *) YAP_realloc(buf, size);
	buf_len = size;
      }
      fseek(filedata_file, index, SEEK_SET);
      fread(buf, 1, size, filedata_file);

      /*�ǡ����ν񤭤���*/
      index_tmp = ftell(filedata_tmp_file);
      fwrite(buf, 1, size, filedata_tmp_file);
    } else {
      index_tmp = 0;
    }
    /*index�ν񤭤���*/
    fwrite(&index_tmp, sizeof(int), 1, filedata_index_tmp_file);
  }

  if (buf != NULL) {
    free(buf);
  }
  
  /*�ե�������Ĥ���*/
  fclose(filedata_file);
  fclose(filedata_size_file);
  fclose(filedata_index_file);
  fclose(filedata_tmp_file);
  fclose(filedata_index_tmp_file);

  /*�ե���������촹����*/
  if (fork()) { int s; wait(&s);} else {
    execl("/bin/mv", "/bin/mv", filedata_tmp, filedata, (char *) 0);
    exit(0);
  }
  if (fork()) { int s; wait(&s);} else {
    execl("/bin/mv", "/bin/mv", filedata_index_tmp, filedata_index, (char *) 0);
    exit(0);
  }

  free(filedata_tmp);
  free(filedata_index_tmp);

  printf("End YAP_Index_Filedata_gc\n");

}
