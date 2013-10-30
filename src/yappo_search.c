/*
 *
 *キーワードを保有するドキュメント番号の取得等
 *
 */
#include <stdio.h>
#include <math.h>
#include <time.h>


#include "yappo_index.h"
#include "yappo_index_pos.h"
#include "yappo_index_filedata.h"
#include "yappo_index_deletefile.h"
#include "yappo_alloc.h"
#include "yappo_ngram.h"
#include "yappo_search.h"



/*
 *SEARCH_RESULTの中身のメモリを解放
 */
void YAP_Search_result_free (SEARCH_RESULT *p)
{
  int i;
  if (p == NULL) {
    return;
  }
  for (i = 0; i < p->keyword_docs_num; i++) {
    free(p->docs_list[i].pos);
    p->docs_list[i].pos = NULL;
  }
  free(p->docs_list);
  p->docs_list = NULL;
}


/*
 *出現位置リストのソートを行なう
 */
int _YAP_Search_result_sort_pos (const void *a, const void *b)
{
  int *ia = ((int *) a);
  int *ib = ((int *) b);

  if (*ib > *ia) {
    return -1;
  } else if (*ib < *ia) {
    return 1;
  } else {
    return 0;
  }
}
void YAP_Search_result_sort_pos (SEARCH_DOCUMENT *p)
{
  if (p != NULL) {
    qsort(p->pos, p->pos_len, sizeof(int), _YAP_Search_result_sort_pos); 
  }
}


/*
 *スコアを元にソートを行なう
 */
int _YAP_Search_result_sort_score (const void *a, const void *b)
{
  if (((SEARCH_DOCUMENT *)b)->score > ((SEARCH_DOCUMENT *)a)->score) {
    return 1;
  } else if (((SEARCH_DOCUMENT *)b)->score < ((SEARCH_DOCUMENT *)a)->score) {
    return -1;
  } else {
    return ((SEARCH_DOCUMENT *)a)->fileindex - ((SEARCH_DOCUMENT *)b)->fileindex;
  }
}
void YAP_Search_result_sort_score (SEARCH_RESULT *p)
{
  if (p != NULL) {
    qsort(p->docs_list, p->keyword_docs_num, sizeof(SEARCH_DOCUMENT), _YAP_Search_result_sort_score); 
  }
}

/*
 *指定サイズ以下のURLのみを残す
 */
SEARCH_RESULT *YAP_Search_result_delete_size (YAPPO_DB_FILES *ydfp, SEARCH_RESULT *p, int max_size)
{
  SEARCH_RESULT *result;
  int i, docs_num, total_num;
  int size;

  if (p == NULL) {
    return NULL;
  }

  result = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
  result->docs_list = (SEARCH_DOCUMENT *) YAP_malloc(sizeof(SEARCH_DOCUMENT) * p->keyword_docs_num);

  docs_num = total_num = 0;

  /* ロック開始 */
  pthread_mutex_lock(&(ydfp->cache->size_mutex));
  for (i = 0; i < p->keyword_docs_num; i++) {
    size = 0;

    if (ydfp->cache->size_num > p->docs_list[i].fileindex) { 
      size = ydfp->cache->size[p->docs_list[i].fileindex]; 
    } else {
      continue;
    }

    if (size < max_size) {
      result->docs_list[docs_num].fileindex = p->docs_list[i].fileindex;
      result->docs_list[docs_num].score = p->docs_list[i].score;
      result->docs_list[docs_num].pos = NULL;/* 出現位置はコピーしない */
      result->docs_list[docs_num].pos_len =  0;/* 長さ情報も同上 */
      total_num += p->docs_list[i].pos_len;
      docs_num++;
    }
  }
  /* ロック解除 */
  pthread_mutex_unlock(&(ydfp->cache->size_mutex));

  if (docs_num == 0) {
    /* 一件も一致しなかった */
    free(result->docs_list);
    free(result);
    return NULL;
  } else {
    result->keyword_id = p->keyword_id;
    result->keyword_total_num  = total_num;
    result->keyword_docs_num   = docs_num;
    return result;
  }
}

/*
 *検索結果から削除文書を取り除く
 *または、異常な検索結果を取り除く
 */
SEARCH_RESULT *YAP_Search_result_delete (YAPPO_DB_FILES *ydfp, SEARCH_RESULT *p)
{
  SEARCH_RESULT *result;
  int i, docs_num, total_num;
  int seek, bit;

  if (p == NULL) {
    return NULL;
  }

  result = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
  result->docs_list = (SEARCH_DOCUMENT *) YAP_malloc(sizeof(SEARCH_DOCUMENT) * p->keyword_docs_num);

  docs_num = total_num = 0;

  /* ロック開始 */
  pthread_mutex_lock(&(ydfp->cache->domainid_mutex));
  for (i = 0; i < p->keyword_docs_num; i++) {
    if (p->docs_list[i].fileindex == 0) {
      continue;
    }

    seek = p->docs_list[i].fileindex / 8;
    bit  = p->docs_list[i].fileindex % 8;

    if ( ! (*(ydfp->cache->deletefile + seek) & (1 << bit))) {
      result->docs_list[docs_num].fileindex = p->docs_list[i].fileindex;
      result->docs_list[docs_num].score = p->docs_list[i].score;
      result->docs_list[docs_num].pos = NULL;/* 出現位置はコピーしない */
      result->docs_list[docs_num].pos_len =  0;/* 長さ情報も同上 */
      total_num += p->docs_list[i].pos_len;
      docs_num++;
    }
  }

  /*ロック解除 */
  pthread_mutex_unlock(&(ydfp->cache->domainid_mutex));


  if (docs_num == 0) {
    /* 一件も一致しなかった */
    free(result->docs_list);
    free(result);
    return NULL;
  } else {
    result->keyword_id = p->keyword_id;
    result->keyword_total_num  = total_num;
    result->keyword_docs_num   = docs_num;
    return result;
  }
}


/*
 *二の出現位置リストを調べて、それぞれの単語の位置がorderバイトはなれているか調べる
 *
 *一致したら0を返す
 *
 */
int YAP_Search_word_pos (int *left_pos, int left_pos_len, int *right_pos, int right_pos_len, int order)
{
  int base_len, target_len;
  int base_i, target_i;
  int base_last, target_last;
  int *base, *target;

  /* 配列が短いほうを基本にする */
  if (left_pos_len > right_pos_len) {
    base = right_pos;
    base_len = right_pos_len;
    target = left_pos;
    target_len = left_pos_len;

    /* orderの逆転 */
    order = 0 - order;
  } else {
    base = left_pos;
    base_len = left_pos_len;
    target = right_pos;
    target_len = right_pos_len;
  }

  base_i = target_i = 0;
  base_last = base[0];
  target_last = target[0];
  while (1) {
    if (target_last - base_last == order) {
      /* ヒット */
      return 0;
    } else if (base_last > target_last) {
      /* targetのカウンタを上げる */
      if (target_i < target_len) {
	target_i++;
	target_last += target[target_i];
      }
    } else if (base_last < target_last) {
      /* baseのカウンタを上げる */
      if (base_i < base_len) {
	base_i++;
	base_last += base[base_i];
      }
    } else {
      /*
       *両方同じなので(要するに同じ文字列の連続判定をする事になる)
       *targetのカウンタを上げる
       */
      if (target_i < target_len) {
	target_i++;
	target_last += target[target_i];
      }
    }

    if (! (target_i <target_len && base_i < base_len)) {
      /* 一致しなかった */
      return 1;
    }
  }

  return 1;
}


/*
 *2つの検索結果からANDの検索結果を返す
 *フレーズ検索用
 *order leftのキーワードとrightがorderバイト分はなれていたら一致
 *order が0なら普通のAND
 */
SEARCH_RESULT *YAP_Search_op_and (SEARCH_RESULT *left, SEARCH_RESULT *right, int order)
{
  SEARCH_RESULT *result, *base, *target;
  int base_i, target_i, found_i;
  int l_i, r_i;
  int new_i;
  int total_num = 0, docs_num = 0;

  /* NULL対策 */
  if (left == NULL || right == NULL) {
    return NULL;
  }

  /* 検索結果の少ない方をメインにする */
  if ( left->keyword_docs_num > right->keyword_docs_num) {
    base   = right;
    target = left;
  } else {
    base   = left;
    target = right;
  }
  result = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
  result->docs_list = (SEARCH_DOCUMENT *) YAP_malloc(sizeof(SEARCH_DOCUMENT) * base->keyword_docs_num);


  /*
   *baseと同じ文書IDをもつdocs_listを探す。
   *前堤としてdocs_listがfileindexで昇順になっている必要が有る
   */
  target_i = 0;
  for (base_i = 0; base_i < base->keyword_docs_num; base_i++) {
    l_i = target_i;/* 左側をターゲットのiに設定 */
    r_i = target->keyword_docs_num;/* 右側をターゲットの末尾に設定 */
    found_i = -1;
    while (l_i < r_i) {
      int c_i = (l_i + r_i) / 2;/* 左と右の真ん中 */
      if (base->docs_list[base_i].fileindex < target->docs_list[c_i].fileindex) {
	/* 左を探す */
	r_i = c_i;
      } else if (base->docs_list[base_i].fileindex > target->docs_list[c_i].fileindex) {
	/* 右を探す */
	l_i = c_i + 1;
      } else {
	/* 見つかった */
	if (order > 0) {
	  /* 出現位置リストを調べる */	  
	  int ret;
	  int _r, _l;
	  if (left == base) {
	    _l = base_i;
	    _r = c_i;
	  } else {
	    _l = c_i;
	    _r = base_i;
	  }
	  if (YAP_Search_word_pos(
				  left->docs_list[_l].pos, left->docs_list[_l].pos_len,
				  right->docs_list[_r].pos, right->docs_list[_r].pos_len,
				  order)) {
	    /* 不一致 */
	    l_i++;
	    found_i = -1;
	  } else {
	    /* 一致 */
	    found_i = c_i;
	  }
	} else {
	  found_i = c_i;
	}
	break;
      }
    }

    if (found_i == -1) {
      /* 見つからなかった */
      target_i = l_i;
    } else {
      /*
       *見つかった
       *
       *右側のキーワードを基本に新しい結果を作成する
       */
      if (base == right) {
	new_i = base_i;
      } else {
	new_i = found_i;
      }

      result->docs_list[docs_num].fileindex = right->docs_list[new_i].fileindex;
      result->docs_list[docs_num].score = base->docs_list[base_i].score + target->docs_list[found_i].score;

      if (order > 0) {
	/*
	 *出現位置の距離を比較するので
	 *出現位置をコピーする
	 */
	result->docs_list[docs_num].pos = (int *) YAP_malloc(sizeof(int) * right->docs_list[new_i].pos_len);      
	memcpy(result->docs_list[docs_num].pos, right->docs_list[new_i].pos, right->docs_list[new_i].pos_len);
	result->docs_list[docs_num].pos_len = right->docs_list[new_i].pos_len;
      } else {
	/* 出現位置はコピーしない */
	result->docs_list[docs_num].pos = NULL;
	result->docs_list[docs_num].pos_len =  0;
      }

      total_num += right->docs_list[new_i].pos_len;
      docs_num++;

      target_i = found_i + 1;
    }
  }

  if (docs_num == 0) {
    /* 一件も一致しなかった */
    free(result->docs_list);
    free(result);
    return NULL;
  } else {
    result->keyword_id = right->keyword_id;
    result->keyword_total_num  = total_num;
    result->keyword_docs_num   = docs_num;
    return result;
  }
}


/*
 *2つの検索結果からORの検索結果を返す
 *
 */
SEARCH_RESULT *YAP_Search_op_or (SEARCH_RESULT *left, SEARCH_RESULT *right)
{
  SEARCH_RESULT *result, *new;
  int left_i, right_i, new_i;
  int total_num = 0, docs_num = 0;
  double score;

  /* NULL対策 */
  if (left == NULL || right == NULL) {
    return NULL;
  }

  result = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
  result->docs_list = (SEARCH_DOCUMENT *) YAP_malloc(sizeof(SEARCH_DOCUMENT) * 
						     (left->keyword_docs_num + right->keyword_docs_num));

  /*
   *前堤としてdocs_listがfileindexで昇順になっている必要が有る
   */
  left_i = right_i = 0;
  while (1) {

    /*
      printf("I:l%d/%d=%d : r%d/%d=%d\n", left->keyword_data.docs_num, left_i, left->docs_list[left_i].fileindex,
      right->keyword_data.docs_num, right_i, right->docs_list[right_i].fileindex);
    */

    if (left->keyword_docs_num > left_i && right->keyword_docs_num > right_i) {
      /* 左右のカウンタが限界でないとき */

      if (left->docs_list[left_i].fileindex > right->docs_list[right_i].fileindex) {
	/* 左側の文書IDが大きいので、右側の結果をコピーする */
	new = right;
	new_i = right_i;
	score = new->docs_list[new_i].score;

	right_i++;
      } else if (left->docs_list[left_i].fileindex < right->docs_list[right_i].fileindex) {
	/* 右側の文書IDが大きいので、左側の結果をコピーする */
	new = left;
	new_i = left_i;
	score = new->docs_list[new_i].score;

	left_i++;
      } else {
	/* 両方ひっかかるので右側の結果をコピー */
	new = right;
	new_i = right_i;
	score = left->docs_list[left_i].score + right->docs_list[right_i].score;

	left_i++;
	right_i++;
      }
    } else if (left->keyword_docs_num > left_i && right->keyword_docs_num <= right_i) {
      /* 右の処理が終了したので左の処理のみを行なう */
      new = left;
      new_i = left_i;
      score = new->docs_list[new_i].score;

      left_i++;
    } else if (left->keyword_docs_num <= left_i && right->keyword_docs_num > right_i) {
      /* 左の処理が終了したので右の処理のみを行なう */
      new = right;
      new_i = right_i;
      score = new->docs_list[new_i].score;

      right_i++;
    } else {
    /* 終了 */
      break;
    }

    /*
      printf("num:%d id:%d\n",  docs_num, new->docs_list[new_i].fileindex);
    */

    /* データのコピー */
    result->docs_list[docs_num].fileindex = new->docs_list[new_i].fileindex;
    result->docs_list[docs_num].score = score;
    result->docs_list[docs_num].pos = NULL;/* 出現位置はコピーしない */
    result->docs_list[docs_num].pos_len =  0;/* 長さ情報も同上 */
    
    /*
      printf("MATCH BASE:%d:%d/%.4f\n", docs_num, result->docs_list[docs_num].fileindex, result->docs_list[docs_num].score);
    */

    total_num += new->docs_list[new_i].pos_len;
    docs_num++;
  }

  if (docs_num == 0) {
    /* 一件も一致しなかった */
    free(result->docs_list);
    free(result);
    return NULL;
  } else {
    result->keyword_id = right->keyword_id;
    result->keyword_total_num  = total_num;
    result->keyword_docs_num   = docs_num;
    return result;
  }
}



/*
 *2つの検索結果からORの検索結果を返す
 *出現位置リストも記録
 *
 */
SEARCH_RESULT *YAP_Search_op_or_add_position (SEARCH_RESULT *left, SEARCH_RESULT *right)
{
  SEARCH_RESULT *result, *new;
  int left_i, right_i, new_i;
  int total_num = 0, docs_num = 0;
  double score;

  /* NULL対策 */
  if (left == NULL || right == NULL) {
    return NULL;
  }

  result = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
  result->docs_list = (SEARCH_DOCUMENT *) YAP_malloc(sizeof(SEARCH_DOCUMENT) * 
						     (left->keyword_docs_num + right->keyword_docs_num));

  /*
   *前堤としてdocs_listがfileindexで昇順になっている必要が有る
   */
  left_i = right_i = 0;
  while (1) {

    /*
      printf("I:l%d/%d=%d : r%d/%d=%d\n", left->keyword_data.docs_num, left_i, left->docs_list[left_i].fileindex,
      right->keyword_data.docs_num, right_i, right->docs_list[right_i].fileindex);
    */
    if (left->keyword_docs_num > left_i && right->keyword_docs_num > right_i) {
      /* 左右のカウンタが限界でないとき */

      if (left->docs_list[left_i].fileindex > right->docs_list[right_i].fileindex) {
	/* 左側の文書IDが大きいので、右側の結果をコピーする */
	result->docs_list[docs_num].fileindex = right->docs_list[right_i].fileindex;
	result->docs_list[docs_num].score = right->docs_list[right_i].score;

	/* 出現位置をコピー */
	result->docs_list[docs_num].pos = (int *) YAP_malloc(sizeof(int) * right->docs_list[right_i].pos_len);
	memcpy(result->docs_list[docs_num].pos, right->docs_list[right_i].pos, right->docs_list[right_i].pos_len * sizeof(int));
	result->docs_list[docs_num].pos_len = right->docs_list[right_i].pos_len;

	right_i++;
      } else if (left->docs_list[left_i].fileindex < right->docs_list[right_i].fileindex) {
	/* 右側の文書IDが大きいので、左側の結果をコピーする */
	result->docs_list[docs_num].fileindex = left->docs_list[left_i].fileindex;
	result->docs_list[docs_num].score = left->docs_list[left_i].score;

	/* 出現位置をコピー */
	result->docs_list[docs_num].pos = (int *) YAP_malloc(sizeof(int) * left->docs_list[left_i].pos_len);
	memcpy(result->docs_list[docs_num].pos, left->docs_list[left_i].pos, left->docs_list[left_i].pos_len * sizeof(int));
	result->docs_list[docs_num].pos_len = left->docs_list[left_i].pos_len;

	left_i++;
      } else {
	/* 両方ひっかかるので両方の結果をコピー */
	result->docs_list[docs_num].fileindex = right->docs_list[right_i].fileindex;
	result->docs_list[docs_num].score = right->docs_list[right_i].score;

	/* 出現位置をコピー */
	result->docs_list[docs_num].pos = (int *) YAP_malloc(sizeof(int) * 
							     (right->docs_list[right_i].pos_len + left->docs_list[left_i].pos_len));


	memcpy(result->docs_list[docs_num].pos, left->docs_list[left_i].pos, left->docs_list[left_i].pos_len * sizeof(int));
	memcpy(result->docs_list[docs_num].pos + left->docs_list[left_i].pos_len, right->docs_list[right_i].pos, right->docs_list[right_i].pos_len * sizeof(int));

	result->docs_list[docs_num].pos_len = right->docs_list[right_i].pos_len + left->docs_list[left_i].pos_len;

	left_i++;
	right_i++;
      }
    } else if (left->keyword_docs_num > left_i && right->keyword_docs_num <= right_i) {
      /* 右の処理が終了したので左の処理のみを行なう */
      result->docs_list[docs_num].fileindex = left->docs_list[left_i].fileindex;
      result->docs_list[docs_num].score = left->docs_list[left_i].score;
      
      /* 出現位置をコピー */
      result->docs_list[docs_num].pos = (int *) YAP_malloc(sizeof(int) * left->docs_list[left_i].pos_len);
      memcpy(result->docs_list[docs_num].pos, left->docs_list[left_i].pos, left->docs_list[left_i].pos_len * sizeof(int));
      result->docs_list[docs_num].pos_len = left->docs_list[left_i].pos_len;

      left_i++;
    } else if (left->keyword_docs_num <= left_i && right->keyword_docs_num > right_i) {
      /* 左の処理が終了したので右の処理のみを行なう */
      result->docs_list[docs_num].fileindex = right->docs_list[right_i].fileindex;
      result->docs_list[docs_num].score = right->docs_list[right_i].score;

      /* 出現位置をコピー */
      result->docs_list[docs_num].pos = (int *) YAP_malloc(sizeof(int) * right->docs_list[right_i].pos_len);
      memcpy(result->docs_list[docs_num].pos, right->docs_list[right_i].pos, right->docs_list[right_i].pos_len * sizeof(int));
      result->docs_list[docs_num].pos_len = right->docs_list[right_i].pos_len;

      right_i++;
    } else {
      /* 終了 */
      break;
    }

    /* データのコピー */
    total_num += result->docs_list[docs_num].pos_len;
    docs_num++;
  }

  if (docs_num == 0) {
    /* 一件も一致しなかった */
    free(result->docs_list);
    free(result);
    return NULL;
  } else {
    result->keyword_id = right->keyword_id;
    result->keyword_total_num  = total_num;
    result->keyword_docs_num   = docs_num;
    return result;
  }
}



/*
 *Nグラムで切り出したキーワードでフレーズ検索を行なう
 */
SEARCH_RESULT *YAP_Search_phrase (YAPPO_DB_FILES *ydfp, NGRAM_SEARCH_LIST *ngram_list, int ngram_list_len)
{
  SEARCH_RESULT *left, *right, *result;
  int i;
  int last_pos;

  if (ngram_list == NULL || ngram_list_len == 0) {
    return NULL;
  }

  printf("start phrease\n");

  /* まずは１つ検索する */
  result = YAP_Search_gram(ydfp, ngram_list[0].keyword);
  if (ngram_list_len == 1) {
    /* １つしか無かった */
    return result;
  }
  last_pos = ngram_list[0].pos;
  printf("phrease %s %d\n", ngram_list[0].keyword, time(NULL));

  for (i = 1; i < ngram_list_len; i++) {
    left = result;
    /* 右を検索 */
    right =  YAP_Search_gram(ydfp, ngram_list[i].keyword);
    printf("phrease %s %d\n", ngram_list[i].keyword, time(NULL));
    /* 出現位置を指定してANDマージ */
    result = YAP_Search_op_and(left, right, ngram_list[i].pos - last_pos);
    printf("phrease and %d\n", time(NULL));

    /* メモリ解放 */
    YAP_Search_result_free(left);
    free(left);
    YAP_Search_result_free(right);
    free(right);

    if (result == NULL) {
      /* 検索不一致 */
      break;
    }
    last_pos = ngram_list[i].pos;
  }

  return result;
}


/*
 *キーワードIDに対応する位置リストを返す
 */
SEARCH_DOCUMENT *YAP_Search_position_get (YAPPO_DB_FILES *ydfp, char *key, int keyword_id, int total_num, int docs_num, int *ret_docs_num)
{
  int ret, i, df;
  unsigned char *posbuf, *posbuf_tmp;
  int posbuf_len, posbuf_len_tmp;
  int *pos, *pos_tmp, pos_len, pos_len_tmp;
  SEARCH_DOCUMENT *docs_list;
  FILEDATA filedata;
  double base_idf;

  /* 全文書中の対象文書の出現率を求める */
  base_idf = log( ydfp->total_filenum / docs_num) + 1.0;

  printf("get %s/%d pos start: %d\n", key, keyword_id, time(NULL));

  /*
   *ポジションリストを取得
   *全ポジションファイルから掻き集める
   */
  posbuf_tmp = (unsigned char *) YAP_malloc(sizeof(int) * (total_num + docs_num + docs_num) * 2);
  posbuf_len_tmp = 0;
  for (i = 0; i <= ydfp->total_filenum / MAX_POS_URL; i++) {
    if (YAP_Db_pos_open(ydfp, i)) {

      ret = YAP_Index_Pos_get(ydfp, keyword_id, &posbuf, &posbuf_len); 

      /*
      {
	int ai;
	for(ai = 0; ai < posbuf_len; ai++) {
	  printf("[%d]", posbuf[ai]);
	}
	printf("END: %d/ %d\n\n", i, keyword_id);
      }
      */

      YAP_Db_pos_close(ydfp);

      printf("[%d]ret: %d/ len: %d\n", i, ret, posbuf_len);
      if (ret == 0) {
	memcpy(posbuf_tmp + posbuf_len_tmp, posbuf, posbuf_len);
	posbuf_len_tmp += posbuf_len;
	free(posbuf);
      }
    }
  }
  printf("get pos decode start: %d\n", time(NULL));
  pos = YAP_Index_8bit_decode(posbuf_tmp, &pos_len, posbuf_len_tmp);
  free(posbuf_tmp);


  printf("get pos end: %d\n", time(NULL));

  if (pos_len > 0) { 
    int ret;
    int ii;
    int size, urllen, filekeywordnum;
    int last_index, now_index, now_seek;
    double tf, idf, tmp, score;

    docs_list = (SEARCH_DOCUMENT *) YAP_malloc(sizeof(SEARCH_DOCUMENT) * docs_num * 2);

    /* ロック開始 */
    pthread_mutex_lock(&(ydfp->cache->score_mutex));
    pthread_mutex_lock(&(ydfp->cache->size_mutex));
    pthread_mutex_lock(&(ydfp->cache->urllen_mutex));
    pthread_mutex_lock(&(ydfp->cache->filekeywordnum_mutex));

    df = 0;
    i = 0;
    last_index = 0;
    while (i < pos_len) {

      /*
	printf("pos: %d / i = %d /  df = %d / %d\n",pos_len, i, df, docs_num);
      */

      /* バグ対策 */
      if (pos[i] == 0) {
	printf("pos ZERO BUG: %d\n", i);
	i++;
	continue;
      }

      docs_list[df].fileindex = pos[i];
      i++;
      docs_list[df].pos_len   = pos[i];
      i++;

      /* 位置リストのコピー */
      /*
	printf("pos: %d/ lon: %d / %d\n", pos_len, docs_list[df].pos_len, docs_list[df].fileindex);
	printf("0=%d/1=%d/2=%d/3=%d/4=%d/5=%d/6=%d/7=%d/8=%d/9=%d\n", pos[i], pos[i+1], pos[i+2], pos[i+3], pos[i+4], pos[i+5], pos[i+6], pos[i+7], pos[i+8], pos[i+9]);
	printf("size: %d\n", sizeof(int) * docs_list[df].pos_len);
      */

      docs_list[df].pos = (int *) YAP_malloc(sizeof(int) * docs_list[df].pos_len);

      /*
	printf("addr: %d\n", docs_list[df].pos);
      */

      memcpy(docs_list[df].pos, &(pos[i]), sizeof(int) * docs_list[df].pos_len);
      i += docs_list[df].pos_len;

      {
	score = 1.0;
	size = urllen = filekeywordnum = 1;
	now_index = docs_list[df].fileindex;

	/* スコアファイル取得 */
	if (ydfp->cache->score_num > now_index) {
	  score = ydfp->cache->score[now_index];
	}

	/* ファイルサイズ取得 */
	if (ydfp->cache->size_num > now_index) {
	  size = ydfp->cache->size[now_index];
	}
	
	/* URLの長さ取得 */
	if (ydfp->cache->urllen_num > now_index) {
	  urllen = ydfp->cache->urllen[now_index];
	}

	/* 文書中のキーワード数の取得 */
	if (ydfp->cache->filekeywordnum_num > now_index) {
	  filekeywordnum = ydfp->cache->filekeywordnum[now_index];
	}
      }


      /*
       *スコアの計算
       */
      {
	tf =  idf = tmp = 0.0;
	score = log(score) + 1.0;

	/* idfを文書サイズで正規化 */
	idf = ((base_idf + 1.0) / (log10(size) + 1.0) * 100.0) + 1.0;
	idf = score / idf * 100.0;/* スコアで正規化 */


	/* 文書中のキーワード出現率 */
	tf = ((double) docs_list[df].pos_len / (double) filekeywordnum * 100.0) + 1.0;



	/* スコアリング */
	/*
	  score = score * score * score;
	*/
	docs_list[df].score = idf * tf * (log10(tf) + 1.0)
	* score  / (double) (urllen * urllen);

      }
      
      df++;
    }

    /* ロック解除 */
    pthread_mutex_unlock(&(ydfp->cache->score_mutex));
    pthread_mutex_unlock(&(ydfp->cache->size_mutex));
    pthread_mutex_unlock(&(ydfp->cache->urllen_mutex));
    pthread_mutex_unlock(&(ydfp->cache->filekeywordnum_mutex));


    free(pos);

    printf("RETURN get pos end end: %d %d\n", time(NULL), df);

    *ret_docs_num = df;/* 本来の文書数を返す */
    return docs_list;
  }
  *ret_docs_num = 0;/* 本来の文書数を返す */
  return NULL;
}

/*
 *キーワード辞書よりkeyを検索してヒットした文書一覧等を返す
 *keyはN-gramで分割された文字列若しくはasc文字
 */
SEARCH_RESULT *YAP_Search_gram (YAPPO_DB_FILES *ydfp, unsigned char *key)
{
  SEARCH_RESULT *result;
  int ret;
  int i, docs_num;
  unsigned long keyword_id;
  int keyword_total_num, keyword_docs_num;

  if (! isalnum(key[0]) && strlen(key) < Ngram_N && 0) { 
    /*
     *Ngram_Nバイト以下の2バイト文字
     *あまりにも処理量が多いので一時休止
     */
    int key_list_len;
    char **key_list;
    ret = YAP_Index_get_keyword_prefix(ydfp, key, &key_list, &key_list_len);
    if (ret == 0) {
      /* キーワードが有った */
      SEARCH_RESULT *left, *right;

      /* 先ずは一つめ */
      docs_num = keyword_total_num = keyword_docs_num = keyword_id = 0;
      result = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
      ret = YAP_Index_get_keyword(ydfp, key_list[0], &keyword_id);

      fseek(ydfp->keyword_totalnum_file, sizeof(int) * keyword_id, SEEK_SET);
      fread(&keyword_total_num, sizeof(int), 1, ydfp->keyword_totalnum_file);
      fseek(ydfp->keyword_docsnum_file, sizeof(int) * keyword_id, SEEK_SET);
      fread(&keyword_docs_num, sizeof(int), 1, ydfp->keyword_docsnum_file);
      result->keyword_id = keyword_id;
      result->keyword_total_num = keyword_total_num;
      result->keyword_docs_num = keyword_docs_num;

      result->docs_list = YAP_Search_position_get(ydfp, key_list[0],
						  result->keyword_id, result->keyword_total_num,
						  result->keyword_docs_num, &docs_num);
      result->keyword_docs_num = docs_num;

      if (result->docs_list != NULL) {
	for (i = 1; i < key_list_len; i++) {
	  left = result;


	  /* 右辺を取得 */
	  docs_num = keyword_total_num = keyword_docs_num = keyword_id = 0;
	  right = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
	  ret = YAP_Index_get_keyword(ydfp, key_list[i], &keyword_id);
	  
	  fseek(ydfp->keyword_totalnum_file, sizeof(int) * keyword_id, SEEK_SET);
	  fread(&keyword_total_num, sizeof(int), 1, ydfp->keyword_totalnum_file);
	  fseek(ydfp->keyword_docsnum_file, sizeof(int) * keyword_id, SEEK_SET);
	  fread(&keyword_docs_num, sizeof(int), 1, ydfp->keyword_docsnum_file);
	  right->keyword_id = keyword_id;
	  right->keyword_total_num = keyword_total_num;
	  right->keyword_docs_num = keyword_docs_num;

	  right->docs_list = YAP_Search_position_get(ydfp, key_list[i],
						    right->keyword_id, right->keyword_total_num,
						    right->keyword_docs_num, &docs_num);
	  right->keyword_docs_num = docs_num;
	  if (right->docs_list == NULL) {
	    /* 終了 */
	    free(right);
	    break;
	  }

	  /* 左辺と右辺をマージ */
	  result = YAP_Search_op_or_add_position(left, right);
	  YAP_Search_result_free(left);
	  free(left);
	  YAP_Search_result_free(right);
	  free(right);

	  /*
	    printf("id: (%s)\n", key_list[i]);
	  */
	}

      } else {
	free(result);
      }


      for (i = 0; i < result->keyword_docs_num; i++) {
	int ii;
	int len = result->docs_list[i].pos_len;
	/*
	  printf("LEN: %d\n", len);
	*/
	YAP_Search_result_sort_pos(&result->docs_list[i]);

      }

      return result;
    }
    return NULL;
  }

  docs_num = keyword_total_num = keyword_docs_num = keyword_id = 0;
  result = (SEARCH_RESULT *) YAP_malloc(sizeof(SEARCH_RESULT));
  ret = YAP_Index_get_keyword(ydfp, key, &keyword_id);

  if (ret == 0) {
    /* キーワードが有った */

    fseek(ydfp->keyword_totalnum_file, sizeof(int) * keyword_id, SEEK_SET);
    fread(&keyword_total_num, sizeof(int), 1, ydfp->keyword_totalnum_file);
    fseek(ydfp->keyword_docsnum_file, sizeof(int) * keyword_id, SEEK_SET);
    fread(&keyword_docs_num, sizeof(int), 1, ydfp->keyword_docsnum_file);
    result->keyword_id = keyword_id;
    result->keyword_total_num = keyword_total_num;
    result->keyword_docs_num = keyword_docs_num;

    printf("ok:0\n");
    result->docs_list = YAP_Search_position_get(ydfp, key, result->keyword_id, result->keyword_total_num,
						result->keyword_docs_num, &docs_num);
    printf("ok:1\n");
    result->keyword_docs_num = docs_num;

    printf("ok\n");
    if (result->docs_list == NULL) {
      free(result);
      return NULL;
    } else {
      return result;
    }
  }

  /* 無かった */
  free(result);
  return NULL;
}

/*
 *キーワードをN-gram変換して検索を行なう
 */
SEARCH_RESULT *YAP_Search_word (YAPPO_DB_FILES *ydfp, char *keyword)
{
  SEARCH_RESULT *result;
  NGRAM_SEARCH_LIST *ngram_list;
  int ngram_list_len = 0;
  int i;

  ngram_list = YAP_Ngram_tokenize_search( keyword, &ngram_list_len);
  result = YAP_Search_phrase(ydfp, ngram_list, ngram_list_len);

  /* n-gramキーワードのメモリ解放 */
  for (i = 0; i < ngram_list_len; i++) {
    free(ngram_list[i].keyword);
  }
  free(ngram_list);

  return result;
}

/*
 *キーワードリストを元に検索を行なう
 */
SEARCH_RESULT *YAP_Search (YAPPO_DB_FILES *ydfp, char **keyword_list, int keyword_list_num, int max_size, int op)
{
  NGRAM_SEARCH_LIST *ngram_key;
  SEARCH_RESULT *result, *left, *right, *result_tmp;
  int ngram_key_len;
  int i;

  if (keyword_list == NULL || keyword_list_num == 0) {
    return NULL;
  }

  /* 先ずは一つめのキーワードの処理を行なう */
  result = YAP_Search_word(ydfp, keyword_list[0]);
  if (keyword_list_num != 1) {
    for (i = 1; i < keyword_list_num; i++) {
      left = result;
      /* 右を検索 */
      right =  YAP_Search_word(ydfp, keyword_list[1]);
      if (op == 0) {
	/* AND */
	result = YAP_Search_op_and(left, right, 0);
      } else if(op == 1) {
	/* OR */
	result = YAP_Search_op_or(left, right);
	if (result == NULL) {
	  /* どちらかの検索結果が無かった */
	  if (left != NULL) {
	    result = left;
	    left = NULL;
	  } else if (right != NULL) {
	    result = right;
	    right = NULL;
	  }
	}
      }

      /* メモリ解放 */
      if (left != NULL) {
	YAP_Search_result_free(left);
	free(left);
      }
      if (right != NULL) {
	YAP_Search_result_free(right);
	free(right);
      }
      
      if (result == NULL) {
	/* 検索不一致 */
	break;
      }
    }
    
  }

  if (result != NULL) {
    /* 削除扱いのURLを除外 */
    result_tmp = YAP_Search_result_delete(ydfp, result);
    YAP_Search_result_free(result);
    free(result);

    /* 指定サイズより大きいURLを除外 */
    result = YAP_Search_result_delete_size(ydfp, result_tmp, max_size);
    YAP_Search_result_free(result_tmp);
    free(result_tmp);

    return result;
  } else {
    return NULL;
  }
}
