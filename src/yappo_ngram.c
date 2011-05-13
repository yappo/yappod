/*
 *N-gram�˴ؤ������
 *
 *���ϲ�ǽ��ʸ�������ɤ�EUC�Τ�
 */
#include "yappo_ngram.h"
#include "yappo_alloc.h"


/*
 *�и����֥ꥹ�Ȥν����
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
 *keyword�����פ���쥳���ɤ򤫤���
 */
NGRAM_LIST *__YAP_Ngram_List_search(NGRAM_LIST *list, unsigned char *keyword)
{
  NGRAM_LIST *next;

  if (list == NULL) {
    return NULL;
  }
  next = list->next;
  /*
   *���פ���쥳���ɤ����Ĥ��뤫�����ƤΥ쥳���ɤ�õ������ޤ�³����
   */
  while (1) {
    if (next == NULL) {
      return NULL;
    }
    if (! strcmp(next->keyword, keyword)) {
      /* ���� */
      return next;
    }
    next = next->next;
  }
  return NULL;
}

/*
 *�ꥹ�Ȥ��ɲ�
 *���Ǥ�keyword��¸�ߤ��Ƥ�����ϡ�index�Υꥹ�Ȥ��ɲä���
 */
void __YAP_Ngram_List_add(NGRAM_LIST *list, unsigned char *keyword, int index)
{
  NGRAM_LIST *p, *add, *next;
  add = __YAP_Ngram_List_search(list, keyword);

  if (add == NULL) {
    /* ���� */
    p = __YAP_Ngram_List_init();
    p->keyword = (unsigned char *) YAP_malloc(strlen(keyword) + 1);
    strcpy(p->keyword, keyword);
    p->index = (int *) YAP_malloc(sizeof(int));
    p->index[p->index_count] = index;
    p->index_count++;

    /* �Ǹ�Υ쥳���ɤ򸡺����ơ��ɲä��� */
    next = list;
    while (1) {
      if (next->next == NULL) {
	break;
      }
      next = next->next;
    }
    next->next = p;
  } else {
    /* �ɲ� */
    add->index_count++;
    add->index = (int *) YAP_realloc(add->index, (sizeof(int) * (add->index_count)));
    add->index[add->index_count-1] = index;
  }

}


/*
 *����ꥹ�Ȱʹߤ˷Ѥ�Ƥ���ꥹ�Ȥξõ�
 */
NGRAM_LIST *YAP_Ngram_List_free(NGRAM_LIST *list)
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
 *N-gram��ʸ�����ʬ��
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
      /* ����������ʸ����̵�뤹�� */
      tokp++;
      continue;
    } else if (*tokp < 0x80) {
      /* 1byteʸ�� */

      if (isalnum(*tokp)) {
	/*
	 *����ե��٥åȤ�����
	 *
	 *1byteʸ����N-gram����
	 */
	gram = YAP_Ngram_get_1byte(tokp);

	tokp_next = tokp + strlen(gram);
      } else {
	/* ����ʤ饹���å� */
	tokp_next = tokp + 1;
      }
    } else {
      /* EUC������ */

      /* 2byteʸ����N-gram���� */
      gram = YAP_Ngram_get_2byte(tokp);

      if (*(tokp + Ngram_N) < 0x80) {
	/*
	 *Ngram_N�Х����褬1byteʸ���ʤ顢����N-gram�����ʤ�ʸ�����ˤʤäƤ��ޤ��Τ�
	 *Ngram_N�Х���ʬ��ư����
	 */
	tokp_next = tokp + Ngram_N;
      } else {
	tokp_next = tokp + 2;
      }
    }

    if (gram != NULL) {
      /* ngram�ꥹ�Ȥ��ɲ� */
      __YAP_Ngram_List_add(ngram_item_list, gram, (tokp - tokp_start));
      (*keyword_num)++;
      free(gram);
    }
    tokp = tokp_next;
  }

  return ngram_item_list;
}




/*
 *1byteʸ���ξ��ϡ�1byteʸ��(��������)��³���¤��ڤ�Ф���
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

  /* ��ʸ�����Ѵ����Ĥĥ��ԡ� */
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
 *2byteʸ����N-gram���ڤ�Ф�
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
 *N-gram��ʸ�����ʬ��
 *������ɸ����Ѥ�N-gram�ꥹ�Ȥ����
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
      /* ����������ʸ����̵�뤹�� */
      tokp++;
      continue;
    } else if (*tokp < 0x80) {
      /* 1byteʸ�� */

      if (isalnum(*tokp)) {
	/*
	 *����ե��٥åȤ�����
	 *
	 *1byteʸ����N-gram����
	 */
	gram = YAP_Ngram_get_1byte(tokp);
	
	tokp_next = tokp + strlen(gram);
      } else {
	/* ����ʤ饹���å� */
	tokp_next = tokp + 1;
      }
    } else {
      /* EUC������ */

      /* 2byteʸ����N-gram���� */
      gram = YAP_Ngram_get_2byte(tokp);

      if (*(tokp + Ngram_N) > 0x7f) {
	/*
	 *����ñ���2�Х���ʸ���ǡ�N�����ʬ��2�Х���ʸ�����������ʤ�
	 *2�Х���ʬ��ư����
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
      /* �ꥹ�Ȥ��ɲ� */
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
