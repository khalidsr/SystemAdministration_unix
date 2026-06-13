#include "malloc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int tests_passed = 0;
int tests_failed = 0;

void print_test_result(const char *test_name, int passed) {
    if (passed) {
        printf(ANSI_COLOR_GREEN "✓ %s" ANSI_COLOR_RESET "\n", test_name);
        tests_passed++;
    } else {
        printf(ANSI_COLOR_RED "✗ %s" ANSI_COLOR_RESET "\n", test_name);
        tests_failed++;
    }
}

void print_header(const char *title) {
    printf("\n" ANSI_COLOR_YELLOW "=== %s ===" ANSI_COLOR_RESET "\n", title);
}

// Test 1: Basic malloc and free
void test_basic_allocation() {
    print_header("Test 1: Basic Allocation");
    
    void *ptr = malloc(100);
    print_test_result("malloc(100) returns non-NULL", ptr != NULL);
    
    if (ptr) {
        memset(ptr, 0xAA, 100);
        free(ptr);
        print_test_result("free(100) completed without crash", 1);
    }
}

// Test 2: Multiple allocations
void test_multiple_allocations() {
    print_header("Test 2: Multiple Allocations");
    
    void *ptrs[10];
    int success = 1;
    
    for (int i = 0; i < 10; i++) {
        ptrs[i] = malloc(50);
        if (!ptrs[i]) {
            success = 0;
            break;
        }
        sprintf((char*)ptrs[i], "Block %d", i);
    }
    
    print_test_result("10 allocations of 50 bytes each", success);
    
    for (int i = 0; i < 10; i++) {
        if (ptrs[i])
            free(ptrs[i]);
    }
    print_test_result("All 10 blocks freed", 1);
}

// Test 3: Memory reuse after free
void test_memory_reuse() {
    print_header("Test 3: Memory Reuse");
    
    void *ptr1 = malloc(100);
    void *ptr2 = malloc(100);
    void *ptr3 = malloc(100);
    
    printf("  ptr1 = %p\n", ptr1);
    printf("  ptr2 = %p\n", ptr2);
    printf("  ptr3 = %p\n", ptr3);
    
    free(ptr2);
    void *ptr4 = malloc(100);
    printf("  ptr4 = %p (should be near ptr2)\n", ptr4);
    
    print_test_result("Memory reused after free", ptr4 == ptr2);
    
    free(ptr1);
    free(ptr3);
    free(ptr4);
}

// Test 4: Different size categories
void test_size_categories() {
    print_header("Test 4: Size Categories");
    
    void *tiny = malloc(32);    // TINY
    void *small = malloc(500);  // SMALL
    void *large = malloc(2000); // LARGE
    
    print_test_result("TINY allocation (32 bytes)", tiny != NULL);
    print_test_result("SMALL allocation (500 bytes)", small != NULL);
    print_test_result("LARGE allocation (2000 bytes)", large != NULL);
    
    printf("  TINY address: %p\n", tiny);
    printf("  SMALL address: %p\n", small);
    printf("  LARGE address: %p\n", large);
    
    free(tiny);
    free(small);
    free(large);
}

// Test 5: Freeing NULL
void test_free_null() {
    print_header("Test 5: Freeing NULL");
    
    free(NULL);
    print_test_result("free(NULL) doesn't crash", 1);
}

// Test 6: malloc(0)
void test_malloc_zero() {
    print_header("Test 6: malloc(0)");
    
    void *ptr = malloc(0);
    print_test_result("malloc(0) returns NULL or valid pointer", 1);
    
    if (ptr) {
        free(ptr);
        print_test_result("free of malloc(0) works", 1);
    }
}

// Test 7: Realloc tests
void test_realloc() {
    print_header("Test 7: Realloc Tests");
    
    // Realloc NULL
    void *ptr1 = realloc(NULL, 100);
    print_test_result("realloc(NULL, 100) works like malloc", ptr1 != NULL);
    
    // Realloc to smaller size
    void *ptr2 = malloc(100);
    strcpy((char*)ptr2, "Hello World");
    void *ptr3 = realloc(ptr2, 50);
    print_test_result("realloc to smaller size preserves data", 
                      strcmp((char*)ptr3, "Hello World") == 0);
    
    // Realloc to larger size
    void *ptr4 = realloc(ptr3, 200);
    print_test_result("realloc to larger size preserves data",
                      strcmp((char*)ptr4, "Hello World") == 0);
    
    // Realloc with size 0
    void *ptr5 = realloc(ptr4, 0);
    print_test_result("realloc with size 0 works like free", 1);
    
    free(ptr1);
}

// Test 8: Stress test
void test_stress() {
    print_header("Test 8: Stress Test (100 allocations)");
    
    void *ptrs[100];
    int success = 1;
    
    for (int i = 0; i < 100; i++) {
        size_t size = (i % 10 + 1) * 8; // 8 to 80 bytes
        ptrs[i] = malloc(size);
        if (!ptrs[i]) {
            success = 0;
            break;
        }
        memset(ptrs[i], i, size);
    }
    
    print_test_result("100 allocations succeeded", success);
    
    // Free every other allocation
    for (int i = 0; i < 100; i += 2) {
        if (ptrs[i])
            free(ptrs[i]);
    }
    print_test_result("Freed 50 blocks", 1);
    
    // Allocate again to test reuse
    for (int i = 0; i < 50; i++) {
        ptrs[i] = malloc(64);
        if (!ptrs[i]) {
            success = 0;
            break;
        }
    }
    print_test_result("Reallocated 50 blocks (should reuse freed space)", success);
    
    // Clean up
    for (int i = 0; i < 100; i++) {
        if (ptrs[i])
            free(ptrs[i]);
    }
}

// Test 9: Large allocations
void test_large_allocations() {
    print_header("Test 9: Large Allocations");
    
    void *large1 = malloc(10000);
    void *large2 = malloc(20000);
    void *large3 = malloc(30000);
    
    print_test_result("10KB allocation", large1 != NULL);
    print_test_result("20KB allocation", large2 != NULL);
    print_test_result("30KB allocation", large3 != NULL);
    
    // Verify they don't overlap
    if (large1 && large2 && large3) {
        int no_overlap = (large1 != large2) && (large2 != large3) && (large1 != large3);
        print_test_result("Large allocations don't overlap", no_overlap);
    }
    
    free(large1);
    free(large2);
    free(large3);
}

// Test 10: Fragmentation test
void test_fragmentation() {
    print_header("Test 10: Fragmentation Test");
    
    void *ptrs[20];
    
    // Allocate alternating sizes
    for (int i = 0; i < 20; i++) {
        size_t size = (i % 2 == 0) ? 32 : 128;
        ptrs[i] = malloc(size);
        if (!ptrs[i]) {
            print_test_result("Alternating allocations", 0);
            return;
        }
        sprintf((char*)ptrs[i], "Block %d", i);
    }
    print_test_result("20 alternating allocations succeeded", 1);
    
    // Free the small ones
    for (int i = 0; i < 20; i += 2) {
        free(ptrs[i]);
    }
    print_test_result("Freed the small blocks", 1);
    
    // Try to allocate medium blocks
    int success = 1;
    for (int i = 0; i < 5; i++) {
        void *ptr = malloc(64);
        if (!ptr) {
            success = 0;
            break;
        }
        free(ptr);
    }
    print_test_result("Fragmentation doesn't prevent 64-byte allocations", success);
    
    // Clean up
    for (int i = 1; i < 20; i += 2) {
        free(ptrs[i]);
    }
}

// Test 11: Show alloc memory
void test_show_alloc_mem() {
    print_header("Test 11: show_alloc_mem()");
    
    printf("\n  Before allocations:\n");
    show_alloc_mem();
    
    void *a = malloc(32);
    void *b = malloc(64);
    void *c = malloc(128);
    void *d = malloc(2048); // Large allocation
    
    printf("\n  After allocations:\n");
    show_alloc_mem();
    
    free(b);
    free(c);
    
    printf("\n  After freeing b and c:\n");
    show_alloc_mem();
    
    free(a);
    free(d);
    
    printf("\n  After cleanup:\n");
    show_alloc_mem();
    
    print_test_result("show_alloc_mem() ran without errors", 1);
}

// Test 12: Boundary test (maximum TINY)
void test_boundary_sizes() {
    print_header("Test 12: Boundary Sizes");
    
    void *tiny_max = malloc(128);  // Max TINY
    void *small_min = malloc(129); // First SMALL
    void *small_max = malloc(1024); // Max SMALL
    void *large_min = malloc(1025); // First LARGE
    
    print_test_result("TINY_MAX (128 bytes)", tiny_max != NULL);
    print_test_result("SMALL_MIN (129 bytes)", small_min != NULL);
    print_test_result("SMALL_MAX (1024 bytes)", small_max != NULL);
    print_test_result("LARGE_MIN (1025 bytes)", large_min != NULL);
    
    free(tiny_max);
    free(small_min);
    free(small_max);
    free(large_min);
}

// Test 13: Realloc in place
void test_realloc_in_place() {
    print_header("Test 13: Realloc In-Place");
    
    void *ptr = malloc(100);
    void *original = ptr;
    
    // Try to shrink (should stay in place)
    ptr = realloc(ptr, 50);
    print_test_result("realloc shrink keeps same address", ptr == original);
    
    // Try to expand (may move)
    ptr = realloc(ptr, 80);
    if (ptr == original) {
        printf(ANSI_COLOR_GREEN "  Expanded in place!" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_YELLOW "  Moved to new address" ANSI_COLOR_RESET "\n");
    }
    print_test_result("realloc expand works", ptr != NULL);
    
    free(ptr);
}

// Test 14: Double free detection (should not crash)
void test_double_free() {
    print_header("Test 14: Double Free");
    
    void *ptr = malloc(100);
    free(ptr);
    free(ptr); // Double free
    print_test_result("Double free doesn't crash (undefined behavior but shouldn't crash)", 1);
}

// Test 15: Memory alignment
void test_alignment() {
    print_header("Test 15: Memory Alignment");
    
    void *ptrs[10];
    int aligned = 1;
    
    for (int i = 0; i < 10; i++) {
        ptrs[i] = malloc(1); // Small allocation
        if ((unsigned long)ptrs[i] % 8 != 0) {
            aligned = 0;
        }
    }
    
    print_test_result("Allocations are 8-byte aligned", aligned);
    
    for (int i = 0; i < 10; i++) {
        free(ptrs[i]);
    }
}

int main(int ac, char **av) {
    printf(ANSI_COLOR_YELLOW "\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     MALLOC TESTER - 42 Project         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf(ANSI_COLOR_RESET);
    
    if (ac > 1) {
        printf("\nRunning specific tests...\n");
        // Add specific test selection if needed
    }
    
    // Run all tests
    test_basic_allocation();
    test_multiple_allocations();
    test_memory_reuse();
    test_size_categories();
    test_free_null();
    test_malloc_zero();
    test_realloc();
    test_stress();
    test_large_allocations();
    test_fragmentation();
    test_show_alloc_mem();
    test_boundary_sizes();
    test_realloc_in_place();
    test_double_free();
    test_alignment();
    
    // Summary
    printf("\n" ANSI_COLOR_YELLOW "=== TEST SUMMARY ===\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_GREEN "Passed: %d\n" ANSI_COLOR_RESET, tests_passed);
    printf(ANSI_COLOR_RED "Failed: %d\n" ANSI_COLOR_RESET, tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf(ANSI_COLOR_GREEN "\n✓ All tests passed! Your malloc is ready for submission!\n" ANSI_COLOR_RESET);
        return 0;
    } else {
        printf(ANSI_COLOR_RED "\n✗ Some tests failed. Please check your implementation.\n" ANSI_COLOR_RESET);
        return 1;
    }
}


// ➜  ft_malloc git:(main) ✗ make fclean && make
// gcc main.c -L. -lft_malloc_x86_64_Linux -Wl,-rpath,'$ORIGIN' -o test_malloc
// ./test_malloc