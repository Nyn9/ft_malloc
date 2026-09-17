#include "ft_malloc.h"
#include "stdio.h"
#include <sys/mman.h>

#define TINY_HEAP_SIZE (4 * sysconf(_SC_PAGESIZE))
#define TINY_BLOCK_SIZE (TINY_HEAP_SIZE / 128)

#define SMALL_HEAP_SIZE (16 * sysconf(_SC_PAGESIZE))
#define SMALL_BLOCK_SIZE (SMALL_HEAP_SIZE / 128)

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
	t_block *block;
	size_t total_size;
	size_t available_size;
	SizeType type;
} t_heap;

SizeType get_size_type(size_t size) {
	if (size <= TINY_BLOCK_SIZE)
		return TINY;
	if (size <= SMALL_BLOCK_SIZE)
		return SMALL;
	return LARGE;
}

size_t get_heap_size(SizeType type) {
	if (type == TINY)
		return TINY_HEAP_SIZE;
	if (type == SMALL)
		return SMALL_HEAP_SIZE;
}

t_block *check_available_block(t_heap *current_heap, size_t size) {
	t_block *current_block = current_heap->block;
	if (!current_block) {
		current_block->next = NULL;
		current_block->prev = NULL;
		current_block->size = size;
		current_block->free = FALSE;
		return current_block;
	} else {
		t_block *prev;
		size_t available = current_heap->available_size;
		while (current_block) {
			prev = current_block;
			if (current_block->free == FALSE) {
				current_block = current_block->next;
				available -= current_block->size;
				continue;
			}
			if (current_block->size < size) {
				current_block = current_block->next;
				available -= current_block->size;
				continue;
			}
			current_block->size = size;
			current_block->free = FALSE;
			return current_block;
		}
		if (!current_block && available >= size) {
			prev->next = current_block;
			current_block->next = NULL;
			current_block->prev = prev;
			current_block->size = size;
			current_block->free = FALSE;
			return current_block;
		}
		return NULL;
	}
}

void *check_available_heap(size_t size, SizeType type, t_heap **heap) {
	if (!*heap) {
		*heap = mmap(NULL, get_heap_size(type), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		//TODO : set type
	}
	t_heap *current_heap = *heap;
	while (current_heap) {
		if (current_heap->type != type) {
			current_heap = current_heap->next;
			continue;
		}
		if (current_heap->available_size < size) {
			current_heap = current_heap->next;
			continue;
		}
		t_block *block = check_available_block(current_heap, size);
		if (block == NULL) {
			current_heap = current_heap->next;
			continue;
		}
		return current_heap;
		// current_heap->block = block;
	}
	if (!current_heap) {
		*heap = mmap(NULL, get_heap_size(type), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		t_block *block = check_available_block(current_heap, size);
		return current_heap;
		// current_heap->block = block;
	}
}

void *ft_malloc(size_t size) {
	t_heap *heap = NULL;
	SizeType type = get_size_type(size);
	t_heap *addr = check_available_heap(size, type, &heap);
	if (!heap)
		puts("Here !");
	// static void *small = mmap(NULL, SMALL_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	return addr;
}

int main() {
	printf("%p\n", ft_malloc(5));
	printf("%p\n", malloc(5));
}