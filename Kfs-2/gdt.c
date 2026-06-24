#include "gdt.h"

static gdt_entry_t *gdt = (gdt_entry_t*)GDT_ADDRESS;
static gdt_ptr_t    gdt_ptr;

void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[index].base_low    = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;

    gdt[index].limit_low   = (limit & 0xFFFF);
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].granularity |= gran & 0xF0;

    gdt[index].access      = access;
}

void gdt_init(void)
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = GDT_ADDRESS;

    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0x00000000, 0x00A00000, GDT_KERNEL_CODE, 0xCF);
    gdt_set_entry(2, 0x00000000, 0x00A00000, GDT_KERNEL_DATA, 0xCF);
    gdt_set_entry(3, 0x00000000, 0x00A00000, 0x96, 0xCF);
    gdt_set_entry(4, 0x00000000, 0x00A00000, GDT_USER_CODE, 0xCF);
    gdt_set_entry(5, 0x00000000, 0x00A00000, GDT_USER_DATA, 0xCF);
    gdt_set_entry(6, 0x00000000, 0x00A00000, 0xF6, 0xCF);

    // Load GDT
    __asm__ volatile("lgdt %0" : : "m"(gdt_ptr));

    // Reload data segment registers with double %%
    __asm__ volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : : "ax"
    );

    // Reload code segment with far jump using push/ret trick
    __asm__ volatile(
        "push $0x08\n"
        "push $1f\n"
        "retf\n"
        "1:\n"
    );

    printk("GDT initialized at 0x%x\n", GDT_ADDRESS);
}

void print_stack(void)
{
    uint32_t esp;
    uint32_t i;

    // get current stack pointer
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));

    printk("=== Kernel Stack ===\n");
    printk("Stack pointer (ESP): 0x%x\n", esp);
    printk("\n");
    printk("Address      Value\n");
    printk("--------     ---------\n");

    // print 16 stack entries
    for (i = 0; i < 16; i++)
    {
        uint32_t addr  = esp + (i * 4);
        uint32_t value = *(uint32_t*)addr;
        printk("0x%x  :  0x%x\n", addr, value);
    }
    printk("====================\n");
}