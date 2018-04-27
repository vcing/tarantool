#ifndef INCLUDES_TARANTOOL_TEST_UNIT_ROPE_COMMON_H
#define INCLUDES_TARANTOOL_TEST_UNIT_ROPE_COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static inline char *
str_getn(void *ctx, char *data, size_t size, size_t offset)
{
	(void) ctx;
	return data + offset;
}

static inline void
str_print(char *data, size_t n)
{
	printf("%.*s", (int) n, data);
}

static inline void *
mem_alloc(void *data, size_t size)
{
	(void) data;
	return malloc(size);
}

static inline void
mem_free(void *data, void *ptr)
{
	(void) data;
	free(ptr);
}

#define ROPE_ALLOC_F mem_alloc
#define ROPE_FREE_F mem_free
#define ROPE_SPLIT_F str_getn
#define rope_data_t char *
#define rope_alloc_ctx_t void *
#define rope_split_ctx_t void *

#include "salad/rope.h"

static inline struct rope *
test_rope_new()
{
	return rope_new(NULL, NULL);
}

static inline void
test_rope_insert(struct rope *rope, rope_size_t offset, char *str)
{
	printf("insert offset = %zu, str = '%s'\n", (size_t) offset, str);
	rope_insert(rope, offset, str, strlen(str));
	rope_pretty_print(rope, str_print);
	rope_check(rope);
}

static inline void
test_rope_erase(struct rope *rope, rope_size_t offset)
{
	printf("erase offset = %zu\n", (size_t) offset);
	rope_erase(rope, offset);
	rope_pretty_print(rope, str_print);
	rope_check(rope);
}

#endif
