OUTPUT_FORMAT(elf32-i386)
ENTRY(start)
SECTIONS
{
    . = 0x00000800;
    .gdt : { *(.gdt) }    /* GDT at 0x00000800 as required */

    . = 1M;
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
