#include "malloc.h"

static int is_large(void *ptr)
{
    t_large_zone *lzone = g_memory.large_zones;

    while (lzone)
    {
        if (lzone->ptr == ptr)
            return 1;
        lzone = lzone->next;
    }
    return 0;
}

static size_t large_size(void *ptr)
{
    t_large_zone *lzone = g_memory.large_zones;

    while (lzone)
    {
        if (lzone->ptr == ptr)
            return lzone->size;
        lzone = lzone->next;
    }
    return 0;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }
    size = ALIGN(size);

    if (is_large(ptr))
    {
        size_t old_size = large_size(ptr);
        if (old_size >= size)
            return ptr;
        void *new_ptr = malloc(size);
        if (!new_ptr)
            return NULL;
        ft_memcpy(new_ptr, ptr, old_size);
        free(ptr);
        return new_ptr;
    }

    t_block *block = (t_block *)ptr - 1;

    if (block->size >= size)
    {
        if (block->size >= size + sizeof(t_block) + ALIGNMENT)
            split_block(block, size);
        return ptr;
    }

    if (block->next && block->next->free &&
        (block->size + sizeof(t_block) + block->next->size) >= size)
    {
        block->size += sizeof(t_block) + block->next->size;
        block->next  = block->next->next;
        if (block->size >= size + sizeof(t_block) + ALIGNMENT)
            split_block(block, size);
        return ptr;
    }

    void *new_ptr = malloc(size);
    if (!new_ptr)
        return NULL;
    ft_memcpy(new_ptr, ptr, block->size);
    free(ptr);
    return new_ptr;
}