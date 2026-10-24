#include "ft_malloc.h"

Bool heap_should_be_free(t_heap *heap) {
	Bool res = FALSE;

	t_block *first = (t_block *)((char *)heap + sizeof(t_heap));
	if (first->free == TRUE && !first->next)
		res = TRUE;

	if (heap->type == LARGE || res == FALSE)
		return res;

	t_heap *check_heap = g_heap;
	while (check_heap) {
		if (check_heap != heap && check_heap->type == heap->type)
			break;
		check_heap = check_heap->next;
	}
	if (!check_heap)
		res = FALSE;

	return res;
}

void free(void *ptr) {
	t_heap *heap = find_heap(ptr);
	if (!heap)
		return ;

	t_block *block = find_block(heap, ptr);
	if (!block || block->free == TRUE)
		return ;

	block->free = TRUE;
	heap->availableSize += block->size;

	if (block->prev && block->prev->free == TRUE) {
		merge_block(block->prev, block);
		heap->availableSize += sizeof(t_block);
		block = block->prev;
	}
	if (block->next && block->next->free == TRUE) {
		merge_block(block, block->next);
		heap->availableSize += sizeof(t_block);
	}

	if (heap_should_be_free(heap) == TRUE) {
		t_heap *prev = heap->prev;
		t_heap *next = heap->next;
		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;
		if (heap == g_heap)
			g_heap = next;
		munmap(heap, heap->totalSize);
	}
}