#include "ft_malloc.h"
#include "stdio.h"
#include <sys/mman.h>

#define TINY_HEAP_SIZE (4 * sysconf(_SC_PAGESIZE))
#define TINY_BLOCK_SIZE (TINY_HEAP_SIZE / 128)

#define SMALL_HEAP_SIZE (16 * sysconf(_SC_PAGESIZE))
#define SMALL_BLOCK_SIZE (SMALL_HEAP_SIZE / 128)


SizeType get_size_type(size_t size) {
	if (size <= TINY_BLOCK_SIZE)
		return TINY;
	if (size <= SMALL_BLOCK_SIZE)
		return SMALL;
	return LARGE;
}

size_t get_heap_size(SizeType type, size_t size) {
	if (type == TINY)
		return TINY_HEAP_SIZE;
	if (type == SMALL)
		return SMALL_HEAP_SIZE;
	return (size + sizeof(t_heap));
}

void init_block(t_block *block, size_t size) {
	block->next = NULL;
	block->size = size;
	block->free = FALSE;
}

t_block *check_available_block(t_heap *heap, size_t size) {
	t_block *block = (t_block *)((char *)heap + sizeof(t_heap));

	if (heap->availableSize + sizeof(t_heap) == heap->totalSize) {
		init_block(block, size);
		return block;
	}

	while (block) {
		// TODO : Split le block
		if (block->free == TRUE && block->size >= size) {
			block->size = size;
			block->free = FALSE;
			return block;
		}
		if (block->next == NULL)
			break;
		block = block->next;
	}
	t_block *new = (t_block *)((char *)block + sizeof(t_block) + block->size);
	block->next = new;
	init_block(new, size);
	return new;
}

t_heap *init_heap(size_t size, SizeType type) {
	t_heap *heap;
	size_t heapSize = get_heap_size(type, size);
	heap = mmap(NULL, heapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	heap->next = NULL;
	heap->totalSize = heapSize;
	heap->availableSize = heapSize - sizeof(t_heap);
	heap->type = type;
	return heap;
}

void *check_available_heap(size_t size, SizeType type, t_heap **heap) {
	if (!*heap)
		*heap = init_heap(size, type);
	t_heap *current_heap = *heap;
	size_t dataSize = size + sizeof(t_block);
	while (current_heap)
	{
		// TODO : Check non suffisant
		if (current_heap->type == type && current_heap->availableSize >= dataSize) {
			t_block *block = check_available_block(current_heap, size);
			current_heap->availableSize -= dataSize;
			return (uint8_t *)block + sizeof(t_block);
		}
		if (current_heap->next == NULL)
			break;
		current_heap = current_heap->next;
	}
	t_heap *new = init_heap(size, type);
	t_block *block = check_available_block(new, size);
	new->availableSize -= dataSize;
	current_heap->next = new;
	return (uint8_t *)block + sizeof(t_block);
}

void *ft_malloc(size_t size) {
	static t_heap *heap = NULL;
	SizeType type = get_size_type(size);
	t_heap *addr = check_available_heap(size, type, &heap);
	// if (!heap)
		
	// static void *small = mmap(NULL, SMALL_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	return addr;
}

int main() {
	int *tab1 = ft_malloc(3 * sizeof(int));
	puts("-----");
	int *tab2 = ft_malloc(5 * sizeof(int));
	puts("-----");
	int *tab3 = ft_malloc(10 * sizeof(int));
	puts("-----");
	int *tabSMALL = ft_malloc(100 * sizeof(int));
	puts("-----");
	int *tabLARGE = ft_malloc(1000 * sizeof(int));
	puts("-----");
	int *tabLARGEE = ft_malloc(1000 * sizeof(int));
	// int *tab4 = ft_malloc(3 * sizeof(int));
	// int *tab5 = ft_malloc(3 * sizeof(int));
	// int *tab6 = ft_malloc(3 * sizeof(int));

	for (int i = 0; i < 5; i++) {
		tab1[i] = i;
		tab2[i] = i + 10;
		tab3[i] = i + 20;
		tabSMALL[i] = i + 30;
		tabLARGE[i] = i + 40;
		tabLARGEE[i] = i + 50;
	}

	printf("%p\n", tab1);
	printf("%p\n", tab2);
	printf("%p\n", tab3);
	printf("%p\n", tabSMALL);
	printf("%p\n", tabLARGE);
	printf("%p\n", tabLARGEE);

	for (int i = 0; i < 5; i++) {
		printf("tab1[%d] : %d\n", i, tab1[i]);
		printf("tab2[%d] : %d\n", i, tab2[i]);
		printf("tab3[%d] : %d\n", i, tab3[i]);
		printf("tabSMALL[%d] : %d\n", i, tabSMALL[i]);
		printf("tabLARGE[%d] : %d\n", i, tabLARGE[i]);
		printf("tabLARGEE[%d] : %d\n", i, tabLARGEE[i]);
	}
}