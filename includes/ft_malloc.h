#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

#include <signal.h>
#include <sys/wait.h>
#include "libft.h"

typedef	enum {
	TINY,
	SMALL,
	LARGE
}		SizeType;

typedef	enum {
	TRUE,
	FALSE
}		Bool;

typedef struct s_block {
	struct s_block *next;
	struct s_block *prev;
	size_t size;
	Bool free;
} __attribute__((aligned(16))) t_block;

typedef struct s_heap {
	struct s_heap *next;
	struct s_heap *prev;
	size_t totalSize;
	size_t availableSize;
	SizeType type;
} __attribute__((aligned(16))) t_heap;

#define TINY_HEAP_SIZE (4 * sysconf(_SC_PAGESIZE))
#define TINY_BLOCK_SIZE (TINY_HEAP_SIZE / 128)

#define SMALL_HEAP_SIZE (16 * sysconf(_SC_PAGESIZE))
#define SMALL_BLOCK_SIZE (SMALL_HEAP_SIZE / 128)

extern t_heap *g_heap;

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);

SizeType get_size_type(size_t size);
size_t get_heap_size(SizeType type, size_t size);
t_block *split_block(t_block *block, size_t size);
void merge_block(t_block *p, t_block *n);
t_block *find_block(t_heap *heap, void *ptr);
int is_max_size(size_t size);