/*
 *memory���δ���
 */

#include "yappo_alloc.h"

/*
 *malloc�Υ�å�
 */
void *__YAP_malloc(char *filename, int line, size_t size)
{
  void *p;

  p = malloc(size);
  if (p == NULL) {
    printf( "YAP_malloc: out of memory:%s:%d\n", filename, line);
    exit(-1);
  }

  memset(p, 0, size);

  return p;
}

/*
 *calloc�Υ�å�
 */
void *__YAP_realloc(char *filename, int line, void *inp, size_t size)
{
  void *p;

  p = realloc(inp, size);
  if (p == NULL) {
    printf( "YAP_realloc: out of memory:%s:%d\n", filename, line);
    exit(-1);
  }

  return p;
}

