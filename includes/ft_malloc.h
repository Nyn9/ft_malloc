#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
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

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);