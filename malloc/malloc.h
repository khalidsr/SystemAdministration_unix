#ifndef MALLOC_H
# define MALLOC_H

#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdarg.h>

#define ALIGNMENT    _Alignof(max_align_t)
#define ALIGN(size)  (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define PAGE_SIZE       sysconf(_SC_PAGESIZE)
#define TINY_MAX        128
#define TINY_ZONE_SIZE  (8 * PAGE_SIZE)
#define SMALL_MAX       1024
#define SMALL_ZONE_SIZE (32 * PAGE_SIZE)

typedef struct s_block {
    struct s_block *next;
    size_t          size;
    size_t          free;
} __attribute__((aligned(_Alignof(max_align_t)))) t_block;

typedef struct s_zone {
    size_t          total_size;
    size_t          used_size;
    struct s_zone  *next;
    t_block        *blocks;
} t_zone;

typedef struct s_large_zone {
    size_t               size;
    void                *ptr;
    struct s_large_zone *next;
} t_large_zone;

typedef struct s_memory {
    t_zone       *tiny_zones;
    t_zone       *small_zones;
    t_large_zone *large_zones;
} t_memory;

extern t_memory g_memory;

void  *malloc(size_t size);
void  *realloc(void *ptr, size_t new_size);
void   free(void *ptr);
void   show_alloc_mem(void);
size_t get_page_size(void);
size_t ft_strlen(char *str);
void  *ft_memcpy(void *dest, const void *src, size_t n);
void  *allocate_in_zone(t_zone **zone, size_t size, size_t zone_size);
void  *large_malloc(size_t size);
void  *find_free_block(t_block *blocks, size_t size);
void   split_block(t_block *block, size_t size);
int    ft_printf(const char *format, ...);

#endif

