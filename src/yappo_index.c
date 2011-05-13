/*
 *インデクシング回り
 */
#include "yappo_index.h"
#include "yappo_db.h"
#include "yappo_alloc.h"


/*
 *domainindexDBからURLをキーに検索をしてレコードIDを取得する
 */
int YAP_Index_get_domainindex(YAPPO_DB_FILES *ydfp, char *domain, int *record_id)
{
  DBT key, value;
  int ret;

  memset(&key, 0,sizeof(DBT));
  memset(&value, 0,sizeof(DBT));

  key.data = domain;
  key.size = strlen(domain);

  value.data = record_id;
  value.size = sizeof(int);
  value.ulen = sizeof(int);
  value.flags = DB_DBT_USERMEM;

  ret = ydfp->domainindex_db->get(ydfp->domainindex_db, NULL, &key, &value, 0);

  return ret;
}

/*
 *domainindexDBからURLをキーに検索をしてレコードIDを設定
 */
int YAP_Index_put_domainindex(YAPPO_DB_FILES *ydfp, char *domain, int *record_id)
{
  DBT key, value;
  int ret;

  memset(&key, 0,sizeof(DBT));
  memset(&value, 0,sizeof(DBT));

  key.data = domain;
  key.size = strlen(domain);

  value.data = record_id;
  value.size = sizeof(int);

  ret = ydfp->domainindex_db->put(ydfp->domainindex_db, NULL, &key, &value, 0);

  return ret;
}

/*
 *domainindexDBからURLをキーに検索をしてレコードを削除
 */
int YAP_Index_del_domainindex(YAPPO_DB_FILES *ydfp, char *domain)
{
  DBT key;
  int ret;

  memset(&key, 0,sizeof(DBT));

  key.data = domain;
  key.size = strlen(domain);

  ret = ydfp->domainindex_db->del(ydfp->domainindex_db, NULL, &key, 0);

  return ret;
}



/*
 *fileindexDBからURLをキーに検索をしてレコードIDを取得する
 */
int YAP_Index_get_fileindex(YAPPO_DB_FILES *ydfp, char *url, int *record_id)
{
  DBT key, value;
  int ret;

  memset(&key, 0,sizeof(DBT));
  memset(&value, 0,sizeof(DBT));

  key.data = url;
  key.size = strlen(url);

  value.data = record_id;
  value.size = sizeof(int);
  value.ulen = sizeof(int);
  value.flags = DB_DBT_USERMEM;

  ret = ydfp->fileindex_db->get(ydfp->fileindex_db, NULL, &key, &value, 0);

  return ret;
}

/*
 *fileindexDBからURLをキーに検索をしてレコードIDを設定
 */
int YAP_Index_put_fileindex(YAPPO_DB_FILES *ydfp, char *url, int *record_id)
{
  DBT key, value;
  int ret;

  memset(&key, 0,sizeof(DBT));
  memset(&value, 0,sizeof(DBT));

  key.data = url;
  key.size = strlen(url);

  value.data = record_id;
  value.size = sizeof(int);

  ret = ydfp->fileindex_db->put(ydfp->fileindex_db, NULL, &key, &value, 0);

  return ret;
}

/*
 *fileindexDBからURLをキーに検索をしてレコードを削除
 */
int YAP_Index_del_fileindex(YAPPO_DB_FILES *ydfp, char *url)
{
  DBT key;
  int ret;

  memset(&key, 0,sizeof(DBT));

  key.data = url;
  key.size = strlen(url);

  ret = ydfp->fileindex_db->del(ydfp->fileindex_db, NULL, &key, 0);

  return ret;
}



/*
 * 2Byte文字はEUCコード 0xa0a0〜0xffffまでの範囲を扱う
 *
 *
 *
 */


/*
 *keywordDBからURLをキーに検索をしてレコードIDを取得する
 */
int YAP_Index_get_keyword(YAPPO_DB_FILES *ydfp, unsigned char *keyword, unsigned long *keyword_id)
{
  if (isalnum(keyword[0])) {
    /* 1byte */
    DBT key, value;

    memset(&key, 0,sizeof(DBT));
    memset(&value, 0,sizeof(DBT));
    
    key.data = keyword;
    key.size = strlen(keyword);
    
    value.data = keyword_id;
    value.size = sizeof(long);
    value.ulen = sizeof(long);
    value.flags = DB_DBT_USERMEM;

    return ydfp->key1byte_db->get(ydfp->key1byte_db, NULL, &key, &value, 0);
  } else {
    /* 2byte */
    int index, key_len, byte, i;
    unsigned long id;
    char c;

    /* 文字列を数値化する */
    index = 0;
    key_len = strlen(keyword);
    byte = 1;
    for (i = key_len - 1; i >= 0; i--) {
      c = *(keyword + i) - 160;
      if (c < 0 || c > 95) {
	/* 規格外 */
	printf("out: %d\n", c);
	c = 95;
      }
      index += c * byte;
      /*
	printf("%d: %d * %d\n", index, c, byte);
      */
      if (byte == 1) {
	byte = 96;
      } else {
	byte *= 96;
      }
    }

    /*
      printf("IDX: %d\t%s\n", index, keyword);
    */
    id = 0;
    fseek(ydfp->key2byte_file, sizeof(long) * index, SEEK_SET);
    fread(&id, sizeof(long), 1, ydfp->key2byte_file);

    if (id == 0) {
      return -1;
    } else {
      *keyword_id = id;
      return 0;
    }
  }
}

/*
 *keywordDBからURLをキーに検索をしてレコードIDを設定
 */
int YAP_Index_put_keyword(YAPPO_DB_FILES *ydfp, unsigned char *keyword, unsigned long *keyword_id)
{
  if (isalnum(keyword[0])) {
    /* 1byte */
    DBT key, value;
    int ret;
    
    memset(&key, 0,sizeof(DBT));
    memset(&value, 0,sizeof(DBT));
    
    key.data = keyword;
    key.size = strlen(keyword);
    
    value.data = keyword_id;
    value.size = sizeof(long);
    
    return ydfp->key1byte_db->put(ydfp->key1byte_db, NULL, &key, &value, 0);
  } else {
    /* 2byte */
    int index, key_len, byte, i;
    char c;

    /* 文字列を数値化する */
    index = 0;
    key_len = strlen(keyword);
    byte = 1;
    for (i = key_len - 1; i >= 0; i--) {
      c = *(keyword + i) - 160;
      if (c < 0 || c > 95) {
	/* 規格外 */
	c = 95;
      }
      index += c * byte;
      if (byte == 1) {
	byte = 96;
      } else {
	byte *= 96;
      }
      /*
      index |= (*(keyword + i) & 0x7f) << byte;
      byte += 7;
      */
    }

    fseek(ydfp->key2byte_file, sizeof(long) * index, SEEK_SET);
    fwrite(keyword_id, sizeof(long), 1, ydfp->key2byte_file);

    return 0;
  }
}



/*
 *8bit符号にエンコードする
 */
unsigned char *YAP_Index_8bit_encode(int *list, int list_len, int *ret_len)
{
  int i;
  unsigned char *encode, *encode_p, *ret;
  encode = (unsigned char *) YAP_malloc(sizeof(int) * ( list_len * 2));
  encode_p = encode;

  *ret_len = 0;
  for (i = 0; i < list_len; i++) {
    unsigned int bit8 = list[i];
    while (1) {
      int bit7 = bit8 & 0x7f;/* 7bitだけ取りだす */
      bit8 = bit8 >> 7;/* 元の数値を7bit分右にずらす */

      if ( bit8 == 0) {
	/* 最後の7bit */
	*encode_p = (unsigned char) bit7;
	encode_p++;
	(*ret_len)++;
	break;
      } else {
	/* 次の7bitも有る */
	*encode_p = (unsigned char) (bit7 | 0x80);
	encode_p++;
	(*ret_len)++;
      }
    }
  }

  ret = (unsigned char *) YAP_malloc(*ret_len + 1);
  memcpy(ret, encode, *ret_len);
  free(encode);

  return ret;
}


/*
 *8bit符号の文字列をデコードする
 */
int *YAP_Index_8bit_decode(unsigned char *encode, int *list_len, int len)
{
  int i;
  int *list, *ret;
  unsigned char *encode_p;
  int bits;

  *list_len = 0;
  bits = 0;

  list = (int *) YAP_malloc(sizeof(int) * sizeof(int) * ( len + 1));
  encode_p = encode;

  list[*list_len] = 0;
  for (i = 0; i < len; i++) {

    list[*list_len] = list[*list_len] + ((*encode_p & 0x7f) << bits);
    bits += 7;
    if ((*encode_p & 0x80) == 0) {
      /* 最後の7bit */
      (*list_len)++;
      list[*list_len] = 0;
      bits = 0;
    }
    encode_p++;
  }

  /*
  ret = (int *) YAP_malloc((sizeof(int) * (*list_len)));
  memcpy(ret, list, sizeof(int) * (*list_len));
  free(list);
  return ret;
  */

  return list;
}
