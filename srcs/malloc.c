#include "ft_malloc.h"

t_heap *g_heap = NULL;

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
	if (size > SIZE_MAX - sizeof(t_heap) - sizeof(t_block) - sysconf(_SC_PAGESIZE) - 15 || !size)
		return NULL;
	size = (size + 15) & ~15;
	SizeType type = get_size_type(size);
	return check_available_heap(size, type);
}


// int main() {
// 	printf("%ld\n", sysconf(_SC_PAGESIZE));
// 	_Static_assert(sizeof(t_heap) % 16 == 0, "t_heap not aligned");
// 	_Static_assert(sizeof(t_block) % 16 == 0, "t_block not aligned");
// 	int *tabLARGE = malloc(1000 * sizeof(int));

// 	for (int i = 0; i < 5; i++)
// 		tabLARGE[i] = i + 50;
// 	printf("%p\n", tabLARGE);

// 	free(tabLARGE);
// 	for (int i = 0; i < 5; i++) 
// 		printf("tabLARGEE[%d] : %d\n", i, tabLARGE[i]);
// }