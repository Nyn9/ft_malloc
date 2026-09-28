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

void init_block(t_block *block, t_block *prev, size_t size) {
	block->next = NULL;
	block->prev = prev;
	block->size = size;
	block->free = FALSE;
	printf("[BLOCK STATUS] block : %p / next : %p / prev : %p / size : %ld\n", block, block->next, block->prev, block->size);
}

void check_available_block(t_heap *heap, size_t size) {
	t_block *current_block = &heap->block;
	t_block *prev;
	size_t pos = 0;

	if (heap->available_size == heap->total_size - sizeof(t_heap)) {

		printf("First Block\n");
		init_block(current_block, NULL, size);
		heap->offset += size;
		return ;
	}
	while (current_block) {
		if (current_block->free == TRUE && current_block->size >= size) {
			printf("Existing Block\n");
			current_block->size = size;
			current_block->free = FALSE;
			heap->offset += sizeof(t_block) + size;
			return ;
		}
		pos += current_block->size;
		prev = current_block;
		printf("[BLOCK STATUS LOOP] block : %p / next : %p / prev : %p / size : %ld\n", current_block, current_block->next, current_block->prev, current_block->size);
		current_block = current_block->next;
	}
	t_block *new = (t_block *)((char *)heap + sizeof(t_heap) + heap->offset);
	printf("New Block\n");
	prev->next = new;
	printf("[PREV STATUS] block : %p / next : %p / prev : %p / size : %ld\n", prev, prev->next, prev->prev, prev->size);
	// printf("size : %ld\n", new.size);
	init_block(new, prev, size);
	heap->offset += sizeof(t_block) + size;
}

t_heap *init_heap(size_t size, SizeType type) {
	t_heap *heap;
	size_t heapSize = get_heap_size(type, size);
	heap = mmap(NULL, heapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	printf("HeapSize : %p\n",heap);
	heap->total_size = heapSize;
	heap->available_size = heapSize - sizeof(t_heap);
	heap->offset = 0;
	heap->type = type;
	printf("[HEAP STATUS] total : %ld / available : %ld\n", heap->total_size, heap->available_size);
	return heap;
}

void *check_available_heap(size_t size, SizeType type, t_heap **heap) {
	if (!*heap) {
		printf("First Heap\n");
		*heap = init_heap(size, type);
	}
	t_heap *current_heap = *heap;
	t_heap *prev;
	while (current_heap)
	{
		if (current_heap->type == type && current_heap->available_size >= size) {
			printf("Existing Heap\n");
			check_available_block(current_heap, size);
			current_heap->available_size -= size;
			return (uint8_t *)current_heap + current_heap->offset;
		}
		prev = current_heap;
		current_heap = current_heap->next;
	}
	printf("New Heap\n");
	t_heap *new = init_heap(size, type);
	new->prev = prev;
	prev->next = new;
	check_available_block(new, size);
	new->available_size -= size;
	return (uint8_t *)new + new->offset;
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