/*
 *������ɽи����֥ǡ�����谷��
 */
#include <stdio.h>

#include "yappo_index_pos.h"
#include "yappo_index.h"
#include "yappo_alloc.h"



/*
 *keyword_id�򥭡��˸����򤷤ư��֥ꥹ�Ȥ����
 */
int YAP_Index_Pos_get(YAPPO_DB_FILES *ydfp, unsigned long keyword_id,
		      unsigned char **postings_buf, int *postings_buf_len)
{
  int ret;
  int pos_size, pos_index;
  int seek;

  if (ydfp->pos_num < keyword_id) {
    /*�оݤȤʤ�ID��¸�ߤ��Ƥ��ʤ�*/
    return -1;
  }

  seek = sizeof(int) * keyword_id;

  /*���������ɤߤ���*/
  fseek(ydfp->pos_size_file, seek, SEEK_SET);
  ret = fread(&pos_size, sizeof(int), 1, ydfp->pos_size_file);
  if (ret == 0) {
    return -1;
  }

  if (pos_size == 0) {
    /*��������0�ʤΤ���Ͽ����Ƥ��ʤ�*/
    return -1;
  }


  /*index���ɤߤ���*/
  fseek(ydfp->pos_index_file, seek, SEEK_SET);
  ret = fread(&pos_index, sizeof(int), 1, ydfp->pos_index_file);
  if (ret == 0) {
    return -1;
  }


  /*�ǡ������ɤߤ���*/
  *postings_buf = (unsigned char *) YAP_malloc(pos_size);
  fseek(ydfp->pos_file, pos_index, SEEK_SET);
  ret = fread(*postings_buf, 1, pos_size, ydfp->pos_file);
  if (ret != pos_size) {
    free(*postings_buf);
    *postings_buf == NULL;
    return -1;
  }
  *postings_buf_len = pos_size;

  return 0;
}

/*
 *keyword_id�򥭡��˸����򤷤ư��֥ꥹ�Ȥ�����
 *��˥ե�������������ɲä���
 */
int YAP_Index_Pos_put(YAPPO_DB_FILES *ydfp, unsigned long keyword_id,
		      unsigned char *postings_buf, int postings_buf_len)
{
  int pos_index;
  int seek;

  if (ydfp->mode == YAPPO_DB_READ) {
    /*�ɤߤ��ߥ⡼�ɤǤϥ��顼*/
    return -1;
  }

  seek = sizeof(int) * keyword_id;

  /*�������ν񤭤���*/
  fseek(ydfp->pos_size_file, seek, SEEK_SET);
  fwrite(&postings_buf_len, sizeof(int), 1, ydfp->pos_size_file);

  /*�ǡ����ν񤭤���*/
  fseek(ydfp->pos_file, 0L, SEEK_END);
  pos_index = ftell(ydfp->pos_file);
  fwrite(postings_buf, 1, postings_buf_len, ydfp->pos_file);

  /*index�ν񤭤���*/
  fseek(ydfp->pos_index_file, seek, SEEK_SET);
  fwrite(&pos_index, sizeof(int), 1, ydfp->pos_index_file);

  return 0;
}

/*
 *keyword_id�򥭡��˸����򤷤ư��֥ꥹ�Ȥ���
 */
int YAP_Index_Pos_del(YAPPO_DB_FILES *ydfp, unsigned long keyword_id)
{
  int c = 0;
  int seek;

  if (ydfp->mode == YAPPO_DB_READ) {
    /*�ɤߤ��ߥ⡼�ɤǤϥ��顼*/
    return -1;
  }

  seek = sizeof(int) * keyword_id;

  /*�������ν񤭤���*/
  fseek(ydfp->pos_size_file, seek, SEEK_SET);
  fwrite(&c, sizeof(int), 1, ydfp->pos_size_file);

  /*index�ν񤭤���*/
  fseek(ydfp->pos_index_file, seek, SEEK_SET);
  fwrite(&c, sizeof(int), 1, ydfp->pos_index_file);

  return 0;
}


/*
 *���־���ե�����Τ���������Ԥʤ�
 *���־���ե������open���Ƥ���ץ�����̵�����Ȥ�����
 */
int YAP_Index_Pos_gc(YAPPO_DB_FILES *ydfp, char *pos, char *pos_size, char *pos_index)
{
  int i;
  long pos_num;
  int seek, index, index_tmp, size, tmp;
  char *pos_tmp, *pos_index_tmp;
  FILE *pos_file, *pos_size_file, *pos_index_file;
  FILE *pos_tmp_file, *pos_index_tmp_file;
  int buf_len = 0;
  char *buf = NULL;

  printf("Start YAP_Index_Pos_gc\n");

  /*����ե�����̾�κ���*/
  pos_tmp = (char *) YAP_malloc(strlen(pos) + 5);
  sprintf(pos_tmp, "%s_tmp", pos);
  pos_index_tmp = (char *) YAP_malloc(strlen(pos_index) + 5);
  sprintf(pos_index_tmp, "%s_tmp", pos_index);

  /*�ե�����򳫤�*/
  pos_file = fopen(pos, "r");
  pos_size_file = fopen(pos_size, "r");
  pos_index_file = fopen(pos_index, "r");
  pos_tmp_file = fopen(pos_tmp, "w");
  pos_index_tmp_file = fopen(pos_index_tmp, "w");


  /*���ܾ���򥳥ԡ�����*/
  fread(&pos_num, sizeof(long), 1, pos_file);
  fwrite(&pos_num, sizeof(long), 1, pos_tmp_file);
  fread(&tmp, sizeof(int), 1, pos_file);
  fwrite(&tmp, sizeof(int), 1, pos_tmp_file);
  fread(&tmp, sizeof(int), 1, pos_file);
  fwrite(&tmp, sizeof(int), 1, pos_tmp_file);


  fseek(pos_size_file, sizeof(int), SEEK_SET);
  fseek(pos_index_file, sizeof(int), SEEK_SET);
  fseek(pos_index_tmp_file, sizeof(int), SEEK_SET);

  /*���־���Υ��ԡ�*/
  for (i = 1; i <= ydfp->total_keywordnum; i++) {
    seek = sizeof(int) * i;

    /*���������ɤߤ���*/
    fread(&size, sizeof(int), 1, pos_size_file);

    if (size > 0) {
      /*��Ͽ��ͭ��*/

      /*index���ɤߤ���*/
      fseek(pos_index_file, seek, SEEK_SET);
      fread(&index, sizeof(int), 1, pos_index_file);

      /*�ǡ������ɤߤ���*/
      if (buf_len < size) {
	buf = (char *) YAP_realloc(buf, size);
	buf_len = size;
      }
      fseek(pos_file, index, SEEK_SET);
      fread(buf, 1, size, pos_file);

      /*�ǡ����ν񤭤���*/
      index_tmp = ftell(pos_tmp_file);
      fwrite(buf, 1, size, pos_tmp_file);
    } else {
      index_tmp = 0;
    }
    /*index�ν񤭤���*/

    fwrite(&index_tmp, sizeof(int), 1, pos_index_tmp_file);
  }

  if (buf != NULL) {
    free(buf);
  }
  
  /*�ե�������Ĥ���*/
  fclose(pos_file);
  fclose(pos_size_file);
  fclose(pos_index_file);
  fclose(pos_tmp_file);
  fclose(pos_index_tmp_file);

  /*�ե���������촹����*/
  if (fork()) { int s; wait(&s);} else {
    execl("/bin/mv", "/bin/mv", pos_tmp, pos, (char *) 0);
    exit(0);
  }
  if (fork()) { int s; wait(&s);} else {
    execl("/bin/mv", "/bin/mv", pos_index_tmp, pos_index, (char *) 0);
    exit(0);
  }

  free(pos_tmp);
  free(pos_index_tmp);

  printf("End YAP_Index_Pos_gc\n");

}
