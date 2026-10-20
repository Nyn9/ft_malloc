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
	if (size > SIZE_MAX - sizeof(t_heap) - sizeof(t_block) - sysconf(_SC_PAGESIZE) - 15 || !size)
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

	if (type != heap->type || type == LARGE)
		return change_address(block, ptr, size);

	if (block->size == size)
		return ptr;
	else if (size >= block->size) {
		size_t diff = block->size - size;
		if (diff < sizeof(t_block)) {
			heap->availableSize -= sizeof(t_block);
			heap->availableSize += diff;
			block = split_block(block, size);
			return ptr;
		} else
			return change_address(block, ptr, size);
	} else {
		if (block->next && block->next->free == TRUE && size <= block->size + block->next->size + sizeof(t_block)) {
			size_t diff = size - block->size - sizeof(t_block)
			if (block->next->size - diff - sizeof(t_block))
				return change_address(block, ptr, size);
			merge_block(block, block->next, heap, diif);
			return ptr;
		} else
			return change_address(block, ptr, size);
	}
}