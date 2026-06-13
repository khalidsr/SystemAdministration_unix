#include "malloc.h"

t_memory g_memory = {NULL, NULL, NULL};

void *malloc(size_t size)
{
    if (size == 0)
        return NULL;
    size = ALIGN(size);
    if (size <= TINY_MAX)
        return allocate_in_zone(&g_memory.tiny_zones, size, TINY_ZONE_SIZE);
    else if (size <= SMALL_MAX)
        return allocate_in_zone(&g_memory.small_zones, size, SMALL_ZONE_SIZE);
    else
        return large_malloc(size);
}

void *large_malloc(size_t size)
{
    size_t       total_size = size + sizeof(t_large_zone);
    size_t       page_size  = get_page_size();
    t_large_zone *zone;

    if (total_size % page_size != 0)
        total_size = ((total_size / page_size) + 1) * page_size;

    zone = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (zone == MAP_FAILED)
        return NULL;

    zone->size           = size;
    zone->ptr            = (void *)(zone + 1);
    zone->next           = g_memory.large_zones;
    g_memory.large_zones = zone;

    return zone->ptr;
}

void *allocate_in_zone(t_zone **zone, size_t size, size_t zone_size)
{
    t_zone *current = *zone;
    void   *ptr     = NULL;

    while (current)
    {
        ptr = find_free_block(current->blocks, size);
        if (ptr)
            return ptr;
        current = current->next;
    }

    t_zone *new_zone = mmap(NULL, zone_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_zone == MAP_FAILED)
        return NULL;

    new_zone->total_size    = zone_size;
    new_zone->used_size     = sizeof(t_zone);
    new_zone->next          = *zone;
    *zone                   = new_zone;
    new_zone->blocks        = (t_block *)((char *)new_zone + sizeof(t_zone));
    new_zone->blocks->size  = zone_size - sizeof(t_zone) - sizeof(t_block);
    new_zone->blocks->free  = 1;
    new_zone->blocks->next  = NULL;

    return find_free_block(new_zone->blocks, size);
}

void split_block(t_block *block, size_t size)
{
    t_block *new_block = (t_block *)((char *)(block + 1) + size);

    new_block->size = block->size - size - sizeof(t_block);
    new_block->free = 1;
    new_block->next = block->next;
    block->size     = size;
    block->next     = new_block;
}

void *find_free_block(t_block *block, size_t size)
{
    while (block)
    {
        if (block->free && block->size >= size)
        {
            if (block->size >= size + sizeof(t_block) + ALIGNMENT)
                split_block(block, size);
            block->free = 0;
            return (void *)(block + 1);
        }
        block = block->next;
    }
    return NULL;
}