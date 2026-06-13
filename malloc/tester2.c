#include "malloc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

static int passed = 0;
static int failed = 0;

static void ok(const char *name, int cond) {
    if (cond) { printf(GREEN "  [PASS] " RESET "%s\n", name); passed++; }
    else       { printf(RED   "  [FAIL] " RESET "%s\n", name); failed++; }
}

static void header(const char *title) {
    printf(YELLOW "\n── %s\n" RESET, title);
}

/* ------------------------------------------------------------------ */
/* 1. NULL / zero edge cases                                            */
/* ------------------------------------------------------------------ */
static void t_null_zero(void) {
    header("1. NULL / zero edge cases");

    free(NULL);
    ok("free(NULL) does not crash", 1);

    void *p = malloc(0);
    ok("malloc(0) returns NULL or unique pointer", 1);
    if (p) { free(p); ok("free of malloc(0) does not crash", 1); }

    void *r = realloc(NULL, 0);
    ok("realloc(NULL, 0) does not crash", 1);
    if (r) free(r);
}

/* ------------------------------------------------------------------ */
/* 2. Alignment                                                         */
/* ------------------------------------------------------------------ */
static void t_alignment(void) {
    header("2. Alignment (16-byte requirement on most 64-bit systems)");

    /* malloc */
    int ok8 = 1, ok16 = 1;
    for (int i = 1; i <= 32; i++) {
        void *p = malloc((size_t)i);
        if (!p) { ok8 = ok16 = 0; break; }
        uintptr_t addr = (uintptr_t)p;
        if (addr % 8  != 0) ok8  = 0;
        if (addr % 16 != 0) ok16 = 0;
        free(p);
    }
    ok("malloc: all 32 small sizes 8-byte aligned",  ok8);
    ok("malloc: all 32 small sizes 16-byte aligned", ok16);

    /* realloc */
    void *p = malloc(64);
    void *r = realloc(p, 128);
    ok("realloc result 16-byte aligned", r && (uintptr_t)r % 16 == 0);
    if (r) free(r); else if (p) free(p);
}

/* ------------------------------------------------------------------ */
/* 3. Basic read/write correctness                                      */
/* ------------------------------------------------------------------ */
static void t_readwrite(void) {
    header("3. Read/write correctness");

    unsigned char *p = malloc(256);
    ok("malloc(256) non-NULL", p != NULL);
    if (!p) return;

    for (int i = 0; i < 256; i++) p[i] = (unsigned char)i;
    int corrupt = 0;
    for (int i = 0; i < 256; i++) if (p[i] != (unsigned char)i) corrupt = 1;
    ok("256-byte write/read round-trip", !corrupt);
    free(p);
}

/* ------------------------------------------------------------------ */
/* 4. Size categories (TINY / SMALL / LARGE)                           */
/* ------------------------------------------------------------------ */
static void t_categories(void) {
    header("4. Size categories");

    /* one per category + boundary values */
    size_t sizes[] = { 1, 16, 128, 129, 512, 1024, 1025, 4096, 65536 };
    int n = (int)(sizeof(sizes) / sizeof(sizes[0]));

    for (int i = 0; i < n; i++) {
        void *p = malloc(sizes[i]);
        char label[64];
        snprintf(label, sizeof(label), "malloc(%zu) non-NULL", sizes[i]);
        ok(label, p != NULL);
        if (p) {
            /* write a byte and read it back */
            *(char *)p = 0x42;
            ok("writable", *(char *)p == 0x42);
            free(p);
        }
    }
}

/* ------------------------------------------------------------------ */
/* 5. Memory reuse after free                                           */
/* ------------------------------------------------------------------ */
static void t_reuse(void) {
    header("5. Memory reuse after free");

    /* Allocate three same-size blocks, free the middle, reallocate */
    void *a = malloc(128);
    void *b = malloc(128);
    void *c = malloc(128);

    printf("  a=%p  b=%p  c=%p\n", a, b, c);
    free(b);
    void *d = malloc(128);
    printf("  d=%p  (expected near b)\n", d);
    ok("freed block reused", d == b);

    free(a); free(c); free(d);
}

/* ------------------------------------------------------------------ */
/* 6. Realloc                                                           */
/* ------------------------------------------------------------------ */
static void t_realloc(void) {
    header("6. Realloc");

    /* realloc(NULL, n) == malloc(n) */
    char *p = realloc(NULL, 64);
    ok("realloc(NULL, 64) non-NULL", p != NULL);
    if (p) { strcpy(p, "hello"); }

    /* grow: data preserved */
    char *q = realloc(p, 512);
    ok("realloc grow non-NULL", q != NULL);
    ok("realloc grow: data preserved", q && strcmp(q, "hello") == 0);

    /* shrink: data preserved */
    if (q) strcpy(q, "world");
    char *r = realloc(q, 8);
    ok("realloc shrink non-NULL", r != NULL);
    ok("realloc shrink: data preserved", r && strcmp(r, "world") == 0);

    /* realloc to 0 == free */
    void *s = realloc(r, 0);
    ok("realloc(ptr, 0) does not crash", 1);
    if (s) free(s);

    /* realloc(NULL, 0) */
    void *t2 = realloc(NULL, 0);
    ok("realloc(NULL, 0) does not crash", 1);
    if (t2) free(t2);
}

/* ------------------------------------------------------------------ */
/* 7. No overlap between independent allocations                        */
/* ------------------------------------------------------------------ */
static void t_no_overlap(void) {
    header("7. No overlap between allocations");

    #define N 50
    void *ptrs[N];
    size_t szs[N];

    for (int i = 0; i < N; i++) {
        szs[i] = (size_t)(((i * 97 + 13) % 200) + 1);
        ptrs[i] = malloc(szs[i]);
        if (ptrs[i]) memset(ptrs[i], i & 0xFF, szs[i]);
    }

    int overlap = 0;
    for (int i = 0; i < N && !overlap; i++) {
        if (!ptrs[i]) continue;
        char *si = (char *)ptrs[i];
        for (int j = i + 1; j < N && !overlap; j++) {
            if (!ptrs[j]) continue;
            char *sj = (char *)ptrs[j];
            if (si < sj + (ptrdiff_t)szs[j] && sj < si + (ptrdiff_t)szs[i])
                overlap = 1;
        }
    }
    ok("50 varied allocations do not overlap", !overlap);

    /* verify bytes weren't clobbered */
    int corrupt = 0;
    for (int i = 0; i < N; i++) {
        if (!ptrs[i]) continue;
        unsigned char *b = (unsigned char *)ptrs[i];
        for (size_t k = 0; k < szs[i]; k++)
            if (b[k] != (unsigned char)(i & 0xFF)) { corrupt = 1; break; }
    }
    ok("50 allocations: bytes not clobbered by each other", !corrupt);

    for (int i = 0; i < N; i++) if (ptrs[i]) free(ptrs[i]);
    #undef N
}

/* ------------------------------------------------------------------ */
/* 8. Interleaved alloc / free                                          */
/* ------------------------------------------------------------------ */
static void t_interleaved(void) {
    header("8. Interleaved alloc/free (fragmentation stress)");

    #define M 200
    void *p[M];
    memset(p, 0, sizeof(p));

    /* allocate all */
    for (int i = 0; i < M; i++)
        p[i] = malloc((size_t)((i % 8 + 1) * 16));

    /* free even indices */
    for (int i = 0; i < M; i += 2) { free(p[i]); p[i] = NULL; }

    /* re-allocate into the freed slots */
    int refail = 0;
    for (int i = 0; i < M; i += 2) {
        p[i] = malloc(64);
        if (!p[i]) refail = 1;
    }
    ok("re-alloc after freeing even slots", !refail);

    /* free odd indices */
    for (int i = 1; i < M; i += 2) { free(p[i]); p[i] = NULL; }

    /* allocate larger blocks (coalescing benefit) */
    int bigfail = 0;
    for (int i = 1; i < M; i += 2) {
        p[i] = malloc(128);
        if (!p[i]) bigfail = 1;
    }
    ok("larger alloc after freeing odd slots", !bigfail);

    for (int i = 0; i < M; i++) if (p[i]) free(p[i]);
    #undef M
}

/* ------------------------------------------------------------------ */
/* 9. Large allocations                                                 */
/* ------------------------------------------------------------------ */
static void t_large(void) {
    header("9. Large allocations");

    size_t sizes[] = { 4096, 16384, 65536, 1 << 20 };
    int n = (int)(sizeof(sizes) / sizeof(sizes[0]));

    for (int i = 0; i < n; i++) {
        void *p = malloc(sizes[i]);
        char label[64];
        snprintf(label, sizeof(label), "malloc(%zu) non-NULL", sizes[i]);
        ok(label, p != NULL);
        if (p) {
            /* touch first and last byte */
            ((char *)p)[0]           = 1;
            ((char *)p)[sizes[i] - 1] = 2;
            ok("large: first+last byte writable",
               ((char *)p)[0] == 1 && ((char *)p)[sizes[i] - 1] == 2);
            free(p);
        }
    }
}

/* ------------------------------------------------------------------ */
/* 10. Many tiny allocations and frees                                  */
/* ------------------------------------------------------------------ */
static void t_many_tiny(void) {
    header("10. Many tiny allocations (1-byte)");

    #define T 500
    char *p[T];
    int fail = 0;

    for (int i = 0; i < T; i++) {
        p[i] = malloc(1);
        if (!p[i]) { fail = 1; break; }
        *p[i] = (char)(i & 0x7F);
    }
    ok("500 × malloc(1) all succeed", !fail);

    int corrupt = 0;
    for (int i = 0; i < T && !fail; i++)
        if (*p[i] != (char)(i & 0x7F)) corrupt = 1;
    ok("500 × malloc(1) data not clobbered", !corrupt);

    for (int i = 0; i < T; i++) if (p[i]) free(p[i]);
    #undef T
}

/* ------------------------------------------------------------------ */
/* 11. Double free (should not crash — UB, but common expectation)     */
/* ------------------------------------------------------------------ */
static void t_double_free(void) {
    header("11. Double free (should not crash)");
    /*
     * Double-free is undefined behaviour.  A robust allocator either
     * silently ignores it or aborts cleanly — it must not segfault
     * and corrupt other live allocations.
     *
     * We test by allocating a canary *after* the soon-to-be-double-freed
     * pointer and checking it is untouched.
     */
    void *victim = malloc(64);
    void *canary = malloc(64);
    memset(canary, 0xCC, 64);

    free(victim);
    free(victim);   /* double free */

    int canary_ok = 1;
    unsigned char *c = (unsigned char *)canary;
    for (int i = 0; i < 64; i++) if (c[i] != 0xCC) { canary_ok = 0; break; }
    ok("canary block untouched after double free", canary_ok);
    free(canary);
}

/* ------------------------------------------------------------------ */
/* 12. Realloc in-place shrink                                          */
/* ------------------------------------------------------------------ */
static void t_realloc_inplace(void) {
    header("12. Realloc in-place shrink keeps same address");

    void *p = malloc(256);
    void *orig = p;
    p = realloc(p, 64);
    ok("realloc shrink: same address", p == orig);
    if (p && p != orig) free(orig); /* in case it moved */
    if (p) free(p);
}

/* ------------------------------------------------------------------ */
/* 13. Realloc preserves full content up to min(old,new)               */
/* ------------------------------------------------------------------ */
static void t_realloc_content(void) {
    header("13. Realloc preserves content across size changes");

    unsigned char *p = malloc(512);
    ok("malloc(512) non-NULL", p != NULL);
    if (!p) return;
    for (int i = 0; i < 512; i++) p[i] = (unsigned char)(i & 0xFF);

    /* grow */
    unsigned char *q = realloc(p, 1024);
    ok("realloc 512→1024 non-NULL", q != NULL);
    if (!q) { free(p); return; }
    int ok_grow = 1;
    for (int i = 0; i < 512; i++) if (q[i] != (unsigned char)(i & 0xFF)) { ok_grow = 0; break; }
    ok("first 512 bytes intact after grow", ok_grow);

    /* shrink */
    unsigned char *r = realloc(q, 128);
    ok("realloc 1024→128 non-NULL", r != NULL);
    if (!r) { free(q); return; }
    int ok_shrink = 1;
    for (int i = 0; i < 128; i++) if (r[i] != (unsigned char)(i & 0xFF)) { ok_shrink = 0; break; }
    ok("first 128 bytes intact after shrink", ok_shrink);

    free(r);
}

/* ------------------------------------------------------------------ */
/* 14. show_alloc_mem smoke test                                        */
/* ------------------------------------------------------------------ */
static void t_show_alloc_mem(void) {
    header("14. show_alloc_mem()");

    void *a = malloc(32);
    void *b = malloc(512);
    void *c = malloc(8192);
    void *d = malloc(1);

    printf(CYAN);
    show_alloc_mem();
    printf(RESET);

    free(b);
    free(d);

    printf(CYAN);
    show_alloc_mem();
    printf(RESET);

    free(a);
    free(c);

    ok("show_alloc_mem() ran without crash", 1);
}

/* ------------------------------------------------------------------ */
/* 15. Consecutive frees of distinct blocks do not corrupt heap        */
/* ------------------------------------------------------------------ */
static void t_heap_integrity(void) {
    header("15. Heap integrity after mixed free order");

    #define K 30
    void *p[K];
    for (int i = 0; i < K; i++) {
        p[i] = malloc((size_t)((i + 1) * 8));
        memset(p[i], i, (size_t)((i + 1) * 8));
    }

    /* free in reverse order */
    for (int i = K - 1; i >= 0; i--) free(p[i]);

    /* now reallocate — heap should be fully reusable */
    int ok_realloc = 1;
    for (int i = 0; i < K; i++) {
        p[i] = malloc(64);
        if (!p[i]) { ok_realloc = 0; break; }
    }
    ok("K allocs after full reverse-order free", ok_realloc);
    for (int i = 0; i < K; i++) if (p[i]) free(p[i]);
    #undef K
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void) {
    printf(YELLOW
        "\n╔══════════════════════════════════════════╗\n"
        "║     MALLOC TESTER v2 — 42 Project        ║\n"
        "╚══════════════════════════════════════════╝\n"
        RESET);

    t_null_zero();
    t_alignment();
    t_readwrite();
    t_categories();
    t_reuse();
    t_realloc();
    t_no_overlap();
    t_interleaved();
    t_large();
    t_many_tiny();
    t_double_free();
    t_realloc_inplace();
    t_realloc_content();
    t_show_alloc_mem();
    t_heap_integrity();

    printf(YELLOW "\n── Summary ──\n" RESET);
    printf(GREEN  "  Passed : %d\n" RESET, passed);
    printf(RED    "  Failed : %d\n" RESET, failed);
    printf("  Total  : %d\n", passed + failed);

    if (failed == 0)
        printf(GREEN "\n✓ All tests passed!\n\n" RESET);
    else
        printf(RED "\n✗ %d test(s) failed — check your implementation.\n\n" RESET, failed);

    return failed ? 1 : 0;
}

// gcc -o test_malloc tester2.c -L. -lft_malloc
// LD_LIBRARY_PATH=. ./test