#include "ft_malloc.h"
#include "stdio.h"
#include <sys/mman.h>

#define TINY_HEAP_SIZE (4 * sysconf(_SC_PAGESIZE))
#define TINY_BLOCK_SIZE (TINY_HEAP_SIZE / 128)

#define SMALL_HEAP_SIZE (16 * sysconf(_SC_PAGESIZE))
#define SMALL_BLOCK_SIZE (SMALL_HEAP_SIZE / 128)


SizeType get_size_type(size_t size) {
	if (size <= (size_t)TINY_BLOCK_SIZE)
		return TINY;
	if (size <= (size_t)SMALL_BLOCK_SIZE)
		return SMALL;
	return LARGE;
}

size_t get_heap_size(SizeType type, size_t size) {
	if (type == TINY)
		return TINY_HEAP_SIZE;
	if (type == SMALL)
		return SMALL_HEAP_SIZE;
	size_t pageSize = sysconf(_SC_PAGESIZE);
	size_t total = size + sizeof(t_heap) + sizeof(t_block);
	return (total + pageSize - 1) / pageSize * pageSize; 
}

void init_block(t_block *block, t_block *prev, size_t size) {
	block->next = NULL;
	block->prev = prev;
	block->size = size;
	block->free = FALSE;
}

t_block *split_block(t_block *block, size_t size) {
	t_block *next = block->next;
	t_block *new = (t_block *)((char *)block + sizeof(t_block) + size);
	new->size = block->size - size - sizeof(t_block);
	new->free = TRUE;
	new->next = next;
	new->prev = block;
	if (next)
		next->prev = new;
	block->size = size;
	block->free = FALSE;
	block->next = new;
	return block;
}

t_block *check_available_block(t_heap *heap, size_t size) {
	t_block *block = (t_block *)((char *)heap + sizeof(t_heap));
	size_t dataSize = size + sizeof(t_block);

	if (heap->availableSize + sizeof(t_heap) == heap->totalSize) {
		init_block(block, NULL, size);
		heap->availableSize -= dataSize;
		return block;
	}

	while (block) {
		if (block->free == TRUE && block->size >= size) {
			if (block->size > dataSize) {
				heap->availableSize -= dataSize;
				return split_block(block, size);
			}
			heap->availableSize -= block->size;
			block->free = FALSE;
			return block;
		}
		if (block->next == NULL)
			break;
		block = block->next;
	}

	char *end = (char *)block + sizeof(t_block) + block->size;
	char *heapEnd = (char *)heap + heap->totalSize;
	if (end + dataSize > heapEnd)
		return NULL;

	block->next = (t_block *)end;
	init_block((t_block *)end, block, size);
	heap->availableSize -= dataSize;
	return (t_block *)end;
}

t_heap *init_heap(size_t size, SizeType type) {
	t_heap *heap;
	size_t heapSize = get_heap_size(type, size);
	heap = mmap(NULL, heapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (heap == MAP_FAILED)
		return NULL;
	heap->next = NULL;
	heap->prev = NULL;
	heap->totalSize = heapSize;
	heap->availableSize = heapSize - sizeof(t_heap);
	heap->type = type;
	return heap;
}

void *check_available_heap(size_t size, SizeType type, t_heap **heap) {
	if (!*heap) {
		*heap = init_heap(size, type);
		if (*heap == NULL)
			return NULL;
	}
	t_heap *current_heap = *heap;
	while (current_heap)
	{
		if (current_heap->type == type && current_heap->availableSize >= size + sizeof(t_block)) {
			t_block *block = check_available_block(current_heap, size);
			if (block != NULL)
				return (uint8_t *)block + sizeof(t_block);
		}
		if (current_heap->next == NULL)
			break;
		current_heap = current_heap->next;
	}
	t_heap *new = init_heap(size, type);
	if (new == NULL)
		return NULL;
	t_block *block = check_available_block(new, size);
	new->prev = current_heap;
	current_heap->next = new;
	return (uint8_t *)block + sizeof(t_block);
}

void *ft_malloc(size_t size) {
	static t_heap *heap = NULL;
	if (size > SIZE_MAX - sizeof(t_heap) - sizeof(t_block) - sysconf(_SC_PAGESIZE) - 15 || !size)
		return NULL;
	size = (size + 15) & ~15;
	SizeType type = get_size_type(size);
	return check_available_heap(size, type, &heap);
}
