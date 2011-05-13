/*
 *���URL�ν���
 */

#include "yappo_db.h"
#include "yappo_index_deletefile.h"

/*
 *
 */
int YAP_Index_Deletefile_get(YAPPO_DB_FILES *ydfp, int fileindex)
{
  int seek, bit;
  unsigned char c = 0;

  if (ydfp->total_filenum < fileindex) {
    /*�оݤȤʤ�ID��¸�ߤ��Ƥ��ʤ�*/
    return -1;
  }

  seek = fileindex / 8;
  bit  = fileindex % 8;

  fseek(ydfp->deletefile_file, seek, SEEK_SET);
  fread(&c, 1, 1, ydfp->deletefile_file);

  if (c & (1 << bit)) {
    /*�����*/
    return 0;
  } else {
    /*̤���*/
    return -1;
  }
}

/*
 *
 */
int YAP_Index_Deletefile_put(YAPPO_DB_FILES *ydfp, int fileindex)
{
  int seek, bit;
  unsigned char c = 0;

  if (ydfp->mode == YAPPO_DB_READ) {
    /*�ɤߤ��ߥ⡼�ɤǤϥ��顼*/
    return -1;
  }

  seek = fileindex / 8;
  bit  = fileindex % 8;

  fseek(ydfp->deletefile_file, seek, SEEK_SET);
  fread(&c, 1, 1, ydfp->deletefile_file);

  if (c & (1 << bit)) {
    /*�����*/
    return -1;
  } else {
    /*̤���*/
    c |= (1 << bit);
    /*fseek(ydfp->deletefile_file, -1L, SEEK_CUR);*/
    fseek(ydfp->deletefile_file, seek, SEEK_SET);
    fwrite(&c, 1, 1, ydfp->deletefile_file);
    return 0;
  }
}

/*
 *
 */
int YAP_Index_Deletefile_del(YAPPO_DB_FILES *ydfp, int fileindex)
{
  int seek, bit;
  unsigned char c = 0;

  if (ydfp->mode == YAPPO_DB_READ) {
    /*�ɤߤ��ߥ⡼�ɤǤϥ��顼*/
    return -1;
  }

  seek = fileindex / 8;
  bit  = fileindex % 8;

  fseek(ydfp->deletefile_file, seek, SEEK_SET);
  fread(&c, 1, 1, ydfp->deletefile_file);

  if (c & (1 << bit)) {
    /*�����*/
    c -= (1 << bit);
    fseek(ydfp->deletefile_file, -1L, SEEK_CUR);
    fwrite(&c, 1, 1, ydfp->deletefile_file);
    return 0;
  } else {
    /*̤���*/
    return -1;
  }
}

