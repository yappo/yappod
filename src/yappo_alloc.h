/*
 *memory回りの管理
 */

#include <stdio.h>
#include <stdlib.h>

void *__YAP_malloc(char *filename, int line, size_t size);
#define YAP_malloc(size) (__YAP_malloc(__FILE__, __LINE__, size))

void *__YAP_realloc(char *filename, int line, void *inp, size_t size);
#define YAP_realloc(inp,size) (__YAP_realloc(__FILE__, __LINE__, inp, size))

