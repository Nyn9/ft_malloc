#include "ft_malloc.h"

static t_heap *next_heap(t_heap *last) {
    t_heap *best = NULL;
    t_heap *heap = g_heap;
    while (heap) {
        if ((!last || heap > last) && (!best || heap < best))
            best = heap;
        heap = heap->next;
    }
    return best;
}

void show_alloc_mem() {
    size_t total = 0;
    t_heap *heap = next_heap(NULL);

    while (heap) {
        if (heap->type == TINY)
            ft_printf("TINY : %p\n", heap);
        else if (heap->type == SMALL)
            ft_printf("SMALL : %p\n", heap);
        else
            ft_printf("LARGE : %p\n", heap);

        t_block *block = (t_block *)((char *)heap + sizeof(t_heap));
        while (block) {
            if (block->free == FALSE) {
                char *start = (char *)block + sizeof(t_block);
                size_t real = block->size - block->padding;
                ft_printf("%p - %p : %u bytes\n", start, start + real, (unsigned int)real);
                total += real;
            }
            block = block->next;
        }
        heap = next_heap(heap);
    }
    ft_printf("Total : %u bytes\n", (unsigned int)total);
}