#include "ft_malloc.h"

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

void merge_block(t_block *p, t_block *n) {
	t_block *next = n->next;
	size_t newSize = p->size + n->size + sizeof(t_block);
	p->next = next;
	p->size = newSize;
	if (next)
		next->prev = p;
}

t_block *find_block(t_heap *heap, void *ptr) {
	t_block *block = (t_block *)((char *)heap + sizeof(t_heap));

	while (block) {
		if ((char *)block + sizeof(t_block) == (char *)ptr)
			return block;
		if ((char *)block + sizeof(t_block) > (char *)ptr)
			return NULL;
		block = block->next;
	}
	return NULL;
}

int is_max_size(size_t size) {
	if (size > SIZE_MAX - sizeof(t_heap) - sizeof(t_block) - sysconf(_SC_PAGESIZE) - 15)
		return 1;
	return 0;
}