#include "malloc.h"

static void coalesce(t_zone *zone)
{
    t_block *curr = (t_block *)((char *)zone + sizeof(t_zone));

    while (curr && curr->next)
    {
        if (curr->free && curr->next->free)
        {
            curr->size += sizeof(t_block) + curr->next->size;
            curr->next  = curr->next->next;
        }
        else
            curr = curr->next;
    }
}

void free(void *ptr)
{
    if (!ptr)
        return ;

    t_zone *zone = g_memory.tiny_zones;
    while (zone)
    {
        if (ptr > (void *)zone && ptr < (void *)((char *)zone + zone->total_size))
        {
            ((t_block *)ptr - 1)->free = 1;
            coalesce(zone);
            return ;
        }
        zone = zone->next;
    }

    zone = g_memory.small_zones;
    while (zone)
    {
        if (ptr > (void *)zone && ptr < (void *)((char *)zone + zone->total_size))
        {
            ((t_block *)ptr - 1)->free = 1;
            coalesce(zone);
            return ;
        }
        zone = zone->next;
    }

    t_large_zone *lzone = g_memory.large_zones;
    t_large_zone *prev  = NULL;
    while (lzone)
    {
        if (lzone->ptr == ptr)
        {
            if (prev)
                prev->next = lzone->next;
            else
                g_memory.large_zones = lzone->next;
            munmap(lzone, lzone->size + sizeof(t_large_zone));
            return ;
        }
        prev  = lzone;
        lzone = lzone->next;
    }
}