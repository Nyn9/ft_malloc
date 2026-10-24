#include "ft_malloc.h"

t_heap *g_heap = NULL;


void init_block(t_block *block, t_block *prev, size_t size) {
	block->next = NULL;
	block->prev = prev;
	block->size = size;
	block->free = FALSE;
	block->padding = 0;
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
		if (!block->next)
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

void *check_available_heap(size_t size, SizeType type) {
	if (!g_heap) {
		g_heap = init_heap(size, type);
		if (!g_heap)
			return NULL;
		t_block *block = check_available_block(g_heap, size);
		return (uint8_t *)block + sizeof(t_block);
	}

	t_heap *heap = g_heap;
	while (heap) {
		if (heap->type == type && type != LARGE && heap->availableSize >= size + sizeof(t_block)) {
			t_block *block = check_available_block(heap, size);
			if (block)
				return (uint8_t *)block + sizeof(t_block);
		}
		if (!heap->next)
			break;
		heap = heap->next;
	}

	t_heap *new = init_heap(size, type);
	if (!new)
		return NULL;
	t_block *block = check_available_block(new, size);
	new->prev = heap;
	heap->next = new;
	return (uint8_t *)block + sizeof(t_block);
}

void *malloc(size_t size) {
	if (is_max_size(size))
		return NULL;
	size_t rounded_size = (size + 15) & ~15;
	if (!rounded_size)
		rounded_size = 16;
	SizeType type = get_size_type(rounded_size);
	void *ptr = check_available_heap(rounded_size, type);
	if (!ptr)
		return NULL;
	t_block *b = (t_block *)((uint8_t *)ptr - sizeof(t_block));
	b->padding = b->size - size;
	return ptr;
}
