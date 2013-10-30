/*
 *N-gramに関する処理
 *
 *入力可能な文字コードはEUCのみ
 */
#include "yappo_ngram.h"
#include "yappo_alloc.h"


/*
 *出現位置リストの初期化
 */
NGRAM_LIST *__YAP_Ngram_List_init(void)
{
  NGRAM_LIST *p = (NGRAM_LIST *) YAP_malloc(sizeof(NGRAM_LIST));
  p->keyword = NULL;
  p->index_count = 0;
  p->index = NULL;
  p->next = NULL;
  return p;
}

/*
 *keywordが一致するレコードをかえす
 */
NGRAM_LIST *__YAP_Ngram_List_search(NGRAM_LIST *list, unsigned char *keyword)
{
  NGRAM_LIST *next;

  if (list == NULL) {
    return NULL;
  }
  next = list->next;
  /*
   *一致するレコードが見つかるか、全てのレコードを探索するまで続ける
   */
  while (1) {
    if (next == NULL) {
      return NULL;
    }
    if (! strcmp(next->keyword, keyword)) {
      /* 一致 */
      return next;
    }
    next = next->next;
  }
  return NULL;
}

/*
 *リストに追加
 *すでにkeywordが存在している場合は、indexのリストに追加する
 */
void __YAP_Ngram_List_add(NGRAM_LIST *list, unsigned char *keyword, int index)
{
  NGRAM_LIST *p, *add, *next;
  add = __YAP_Ngram_List_search(list, keyword);

  if (add == NULL) {
    /* 新規 */
    p = __YAP_Ngram_List_init();
    p->keyword = (unsigned char *) YAP_malloc(strlen(keyword) + 1);
    strcpy(p->keyword, keyword);
    p->index = (int *) YAP_malloc(sizeof(int));
    p->index[p->index_count] = index;
    p->index_count++;

    /* 最後のレコードを検索して、追加する */
    next = list;
    while (1) {
      if (next->next == NULL) {
	break;
      }
      next = next->next;
    }
    next->next = p;
  } else {
    /* 追加 */
    add->index_count++;
    add->index = (int *) YAP_realloc(add->index, (sizeof(int) * (add->index_count)));
    add->index[add->index_count-1] = index;
  }

}


/*
 *指定リスト以降に継れているリストの消去
 */
void YAP_Ngram_List_free(NGRAM_LIST *list)
{
  NGRAM_LIST *this, *next;

  next = list;
  while (1) {
    if (next == NULL) {
      return;
    }
    this = next;
    next = this->next;
    free(this->keyword);
    this->keyword = NULL;
    free(this->index);
    this->index = NULL;
    this->index_count = 0;

    free(this);
    this = NULL;
  }
  return;
}


/*
 *N-gramで文字列を分解
 */
NGRAM_LIST *YAP_Ngram_tokenize(char *body, int *keyword_num)
{
  unsigned char *tokp, *tokp_start, *tokp_next, *gram;
  NGRAM_LIST *ngram_item_list = __YAP_Ngram_List_init();

  tokp = body;
  tokp_start = body;



  while (*tokp) {
    gram = NULL;

    if (*tokp <= 0x20) {
      /* エスケープ文字は無視する */
      tokp++;
      continue;
    } else if (*tokp < 0x80) {
      /* 1byte文字 */

      if (isalnum(*tokp)) {
	/*
	 *アルファベットか数字
	 *
	 *1byte文字のN-gram取得
	 */
	gram = YAP_Ngram_get_1byte(tokp);

	tokp_next = tokp + strlen(gram);
      } else {
	/* 記号ならスキップ */
	tokp_next = tokp + 1;
      }
    } else {
      /* EUCコード */

      /* 2byte文字のN-gram取得 */
      gram = YAP_Ngram_get_2byte(tokp);

      if (*(tokp + Ngram_N) < 0x80) {
	/*
	 *Ngram_Nバイト先が1byte文字なら、次のN-gramが少ない文字数になってしまうので
	 *Ngram_Nバイト分移動する
	 */
	tokp_next = tokp + Ngram_N;
      } else {
	tokp_next = tokp + 2;
      }
    }

    if (gram != NULL) {
      /* ngramリストに追加 */
      __YAP_Ngram_List_add(ngram_item_list, gram, (tokp - tokp_start));
      (*keyword_num)++;
      free(gram);
    }
    tokp = tokp_next;
  }

  return ngram_item_list;
}




/*
 *1byte文字の場合は、1byte文字(記号を除く)が続く限り切り出す。
 */
char *YAP_Ngram_get_1byte(unsigned char *tokp)
{
  unsigned char *ret, *p, *a, *b;
  p = tokp;

  while (*p) {
    if (*p > 0x20 && *p < 0x80 && isalnum(*p)) {
      p++;
    } else {
      break;
    }
  }

  /* 小文字に変換しつつコピー */
  ret = YAP_malloc((p - tokp) +1);
  a = ret;
  b = tokp;
  while (b < p) {
    *a = towlower(*b);
    a++;
    b++;
  }
  return ret;
}


/*
 *2byte文字のN-gramを切り出す
 */
char *YAP_Ngram_get_2byte(unsigned char *tokp)
{
  unsigned char *ret, *p;
  p = tokp;

  while (*p && (p - tokp) < Ngram_N) {
    if (*p >= 0x80) {
      p++;
    } else {
      break;
    }
  }

  ret = YAP_malloc((p-tokp)+1);
  strncpy(ret, tokp, (p-tokp));

  return ret;
}


/*
 *N-gramで文字列を分解
 *キーワード検索用のN-gramリストを作成
 */
NGRAM_SEARCH_LIST *YAP_Ngram_tokenize_search(char *body, int *keyword_num)
{
  unsigned char *tokp, *tokp_start, *tokp_next, *gram;
  NGRAM_SEARCH_LIST *list = NULL;

  tokp = body;
  tokp_start = body;

  while (*tokp) {
    gram = NULL;

    if (*tokp <= 0x20) {
      /* エスケープ文字は無視する */
      tokp++;
      continue;
    } else if (*tokp < 0x80) {
      /* 1byte文字 */

      if (isalnum(*tokp)) {
	/*
	 *アルファベットか数字
	 *
	 *1byte文字のN-gram取得
	 */
	gram = YAP_Ngram_get_1byte(tokp);
	
	tokp_next = tokp + strlen(gram);
      } else {
	/* 記号ならスキップ */
	tokp_next = tokp + 1;
      }
    } else {
      /* EUCコード */

      /* 2byte文字のN-gram取得 */
      gram = YAP_Ngram_get_2byte(tokp);

      if (*(tokp + Ngram_N) > 0x7f) {
	/*
	 *次の単語も2バイト文字で、Nグラム分の2バイト文字数に満たない
	 *2バイト分移動する
	 */
	unsigned char *np;
	np = tokp + Ngram_N + 2;
	while (*np > 0x80 && np < tokp + (Ngram_N * 2)) {
	  np += 2;
	}
	tokp_next = np - Ngram_N;
      } else {
	tokp_next = tokp + Ngram_N;
      }
    }

    if (gram != NULL) {
      /* リストに追加 */
      list = (NGRAM_SEARCH_LIST *) YAP_realloc(list, sizeof(NGRAM_SEARCH_LIST) * (*keyword_num + 1));

      list[*keyword_num].keyword = (char *) YAP_malloc(strlen(gram) + 1);
      strcpy(list[*keyword_num].keyword, gram);
      list[*keyword_num].pos = (tokp - tokp_start);
      (*keyword_num)++;
      free(gram);
    }
    tokp = tokp_next;
  }

  return list;
}
