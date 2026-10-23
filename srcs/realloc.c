#include "ft_malloc.h"

void *change_address(t_block *block, void *ptr, size_t size) {
	void *new = malloc(size);
	if (new) {
		size_t min = block->size < size ? block->size : size;
		ft_memcpy(new, ptr, min);
		free(ptr);
	}
	return new;
}

void *realloc(void *ptr, size_t size) {
	if (!ptr)
		return malloc(size);
	if (!size) {
		free(ptr);
		return NULL;
	}
	if (is_max_size(size))
		return NULL;
	size = (size + 15) & ~15;
	SizeType type = get_size_type(size);

	t_heap *heap = g_heap;
	while (heap) {
		if ((char *)ptr >= (char *)heap && (char *)ptr < (char *)heap + heap->totalSize)
			break;
		heap = heap->next;
	}
	if (!heap)
		return NULL;

	t_block *block = find_block(heap, ptr);
	if (!block || block->free == TRUE)
		return NULL;

	if (block->size == size)
		return ptr;

	if (type != heap->type || type == LARGE)
		return change_address(block, ptr, size);

	if (size < block->size) {
		size_t diff = block->size - size;
		if (diff >= sizeof(t_block) + 16) {
			heap->availableSize -= sizeof(t_block);
			heap->availableSize += diff;
			split_block(block, size);
			if (block->next->next && block->next->next->free == TRUE)
				merge_block(block->next, block->next->next);
		}
		return ptr;
	} else {
		if (block->next && block->next->free == TRUE && size <= block->size + block->next->size + sizeof(t_block)) {
			heap->availableSize -= block->next->size + sizeof(t_block);
			merge_block(block, block->next);
			if (block->size - size >= sizeof(t_block) + 16) {
				heap->availableSize += block->size - size - sizeof(t_block);
				split_block(block, size);
			}
			return ptr;
		} else
			return change_address(block, ptr, size);
	}
}