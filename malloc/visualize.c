


#include "malloc.h"


size_t get_page_size(void) 
{
    size_t page_size = 0;
    page_size = sysconf(_SC_PAGESIZE);
    return page_size;
}

static int zone_has_alloc(t_zone *zone)
{
    t_block *block = zone->blocks;

    while (block)
    {
        if (!block->free)
            return 1;
        block = block->next;
    }
    return 0;
}

static void print_zone_blocks(t_zone *zone, size_t *total)
{
    t_block *block = zone->blocks;

    while (block)
    {
        if (!block->free)
        {
            ft_printf("%p - %p : %zu bytes\n",
                (void *)(block + 1),
                (void *)((char *)(block + 1) + block->size),
                block->size);
            *total += block->size;
        }
        block = block->next;
    }
}

void show_alloc_mem(void)
{
    size_t        total = 0;
    t_zone       *zone;
    t_large_zone *lzone;

    zone = g_memory.tiny_zones;
    while (zone)
    {
        if (zone_has_alloc(zone))
        {
            ft_printf("TINY : %p\n", zone);
            print_zone_blocks(zone, &total);
        }
        zone = zone->next;
    }

    zone = g_memory.small_zones;
    while (zone)
    {
        if (zone_has_alloc(zone))
        {
            ft_printf("SMALL : %p\n", zone);
            print_zone_blocks(zone, &total);
        }
        zone = zone->next;
    }

    lzone = g_memory.large_zones;
    while (lzone)
    {
        ft_printf("LARGE : %p\n", lzone->ptr);
        ft_printf("%p - %p : %zu bytes\n",
            lzone->ptr,
            (void *)((char *)lzone->ptr + lzone->size),
            lzone->size);
        total += lzone->size;
        lzone  = lzone->next;
    }

    ft_printf("Total : %zu bytes\n", total);
}