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
} t_block;

typedef struct s_heap {
	struct s_heap *next;
	struct s_heap *prev;
	t_block block;
	size_t offset;
	size_t total_size;
	size_t available_size;
	SizeType type;
} t_heap;

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);