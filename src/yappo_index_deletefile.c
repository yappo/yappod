/*
 *削除URLの処理
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
    /*対象となるIDは存在していない*/
    return -1;
  }

  seek = fileindex / 8;
  bit  = fileindex % 8;

  fseek(ydfp->deletefile_file, seek, SEEK_SET);
  fread(&c, 1, 1, ydfp->deletefile_file);

  if (c & (1 << bit)) {
    /*削除済*/
    return 0;
  } else {
    /*未削除*/
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
    /*読みこみモードではエラー*/
    return -1;
  }

  seek = fileindex / 8;
  bit  = fileindex % 8;

  fseek(ydfp->deletefile_file, seek, SEEK_SET);
  fread(&c, 1, 1, ydfp->deletefile_file);

  if (c & (1 << bit)) {
    /*削除済*/
    return -1;
  } else {
    /*未削除*/
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
    /*読みこみモードではエラー*/
    return -1;
  }

  seek = fileindex / 8;
  bit  = fileindex % 8;

  fseek(ydfp->deletefile_file, seek, SEEK_SET);
  fread(&c, 1, 1, ydfp->deletefile_file);

  if (c & (1 << bit)) {
    /*削除済*/
    c -= (1 << bit);
    fseek(ydfp->deletefile_file, -1L, SEEK_CUR);
    fwrite(&c, 1, 1, ydfp->deletefile_file);
    return 0;
  } else {
    /*未削除*/
    return -1;
  }
}

