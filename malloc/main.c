#include "malloc.h"
#include <stdio.h>

int main(void)
{
    g_memory.tiny_zones  = NULL;
    g_memory.small_zones = NULL;
    g_memory.large_zones = NULL;

    char *a = malloc(32);
    char *b = malloc(64);
    char *c = malloc(129);
    char *d = malloc(1500);

    a[0] = 'a'; a[1] = '\0';
    b[0] = 'b'; b[1] = '\0';
    c[0] = 'c'; c[1] = '\0';
    d[0] = 'd'; d[1] = '\0';

    show_alloc_mem();

    free(b);
    free(c);

    write(1, "\nAfter freeing b and c:\n", 24);
    
    show_alloc_mem();

    free(a);
    free(d);
    return 0;
}