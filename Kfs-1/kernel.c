#include "kernel.h"


unsigned short *terminal_buffer;
unsigned int vga_index;
static int cursor_x = 0;
static int cursor_y = 0;

static bool shift_pressed = false;


static char scroll_buffer[SCROLL_BUFFER_SIZE][VGA_WIDTH * 2];
static int scroll_offset = 0;
static int total_lines = 0;


static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

void redraw_from_buffer(char *vidptr)
{
    int screen_line, buffer_line;
    int i;
    
    for (screen_line = 0; screen_line < VGA_HEIGHT; screen_line++)
    {
        buffer_line = scroll_offset + screen_line;
        
        if (buffer_line >= SCROLL_BUFFER_SIZE)
            buffer_line -= SCROLL_BUFFER_SIZE;
        
        for (i = 0; i < VGA_WIDTH * 2; i++)
        {
            vidptr[screen_line * VGA_WIDTH * 2 + i] = scroll_buffer[buffer_line][i];
        }
    }
}

void scroll_up(char *vidptr)
{
    if (scroll_offset > 0)
    {
        scroll_offset--;
        redraw_from_buffer(vidptr);
    }
}

uint8_t keyboard_get_scancode(void)
{
    while (!keyboard_has_input());
    return inb(KEYBOARD_DATA_PORT);
}

void scroll_down(char *vidptr)
{
    int max_offset = total_lines - VGA_HEIGHT;
    if (max_offset < 0)
        max_offset = 0;
    
    if (scroll_offset < max_offset)
    {
        scroll_offset++;
        redraw_from_buffer(vidptr);
    }
}

void clear_scroll_buffer(void)
{
    int i, j;
    for (i = 0; i < SCROLL_BUFFER_SIZE; i++)
    {
        for (j = 0; j < VGA_WIDTH * 2; j += 2)
        {
            scroll_buffer[i][j] = ' ';
            scroll_buffer[i][j + 1] = GRAY;
        }
    }
    total_lines = 0;
    scroll_offset = 0;
}

void clear_screen(char* vidptr)
{
    int j = 0;

    while(j < 80*25*2)
    {
        vidptr[j] = ' ';
        vidptr[j+1] = GRAY;
        j = j+2;
    }
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void update_cursor(int x, int y)
{
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    
    cursor_x = x;
    cursor_y = y;
}


void scroll_screen(char *vidptr)
{
    int i;
    
    for (i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i++)
    {
        vidptr[i] = vidptr[i + VGA_WIDTH * 2];
    }
    
  
    for (i = (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i < VGA_HEIGHT * VGA_WIDTH * 2; i += 2)
    {
        vidptr[i] = ' ';
        vidptr[i + 1] = GRAY;
    }
}

void keyboard_init(void)
{
    while (inb(KEYBOARD_STATUS_PORT) & 0x01) 
    {
        inb(KEYBOARD_DATA_PORT);
    }
}

bool keyboard_has_input(void)
{
    return (inb(KEYBOARD_STATUS_PORT) & 0x01) != 0;
}


int main(void)
{
    char *vidptr = (char*) VGA_ADDRESS;
    char c = 0;
    
    clear_screen(vidptr);
    clear_scroll_buffer();
    enable_cursor(0, 15);
    
    cursor_x = 0;
    cursor_y = 0;
    
    set_print_color(GREEN);
    printf("#    #     ######\n");
    printf("#    #          #\n");
    printf("#    #          #\n");
    printf("#    #     ######\n");
    printf("######     #     \n");
    printf("     #     #     \n");
    printf("     #     ######\n");
    

    set_print_color(YELLOW);
    printf("=== Welcome to 42 Kernel ===\n\n");

    printk("Initializing keyboard...\n");
    keyboard_init();
    printk("Keyboard ready\n");
    
    printf("\n");
    set_print_color(LIGHT_CYAN);
    printf("Type something (ESC to quit):\n");
    
    set_print_color(WHITE_COLOR);
    printf("> ");
    
    while (1) 
    {
        uint8_t scancode = keyboard_get_scancode();
        
       
        if (scancode == 0x2A || scancode == 0x36)  // Handle shift keys
        {
            shift_pressed = true;
            continue;
        }
        else if (scancode == 0xAA || scancode == 0xB6)
        {
            shift_pressed = false;
            continue;
        }
        
        
        if (scancode & 0x80)     // Ignore key releases (high bit set)
            continue;

        if (scancode < sizeof(scancode_to_ascii))
        {
            if (shift_pressed)
                c = scancode_to_ascii_shift[scancode];
            else
                c = scancode_to_ascii[scancode];
        }
        if (scancode == 0x01)
        {
            printf("\n");
            printk("ESC pressed, shutting down...\n");
            break;
        }

        if (c != 0)
            printf("%c", c);
    }
    
    printf("\n");
    set_print_color(RED);
    printf("Goodbye!\n"); 
    printk("Kernel halted\n");

 while(1) {
        __asm__("hlt");
    }
return 0;
}