#ifndef KERNEL_H
#define KERNEL_H




typedef int bool;
#define true  1
#define false 0

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef __builtin_va_list va_list;
#define va_start(v, l)  __builtin_va_start(v, l)
#define va_arg(v, l)    __builtin_va_arg(v, l)
#define va_end(v)       __builtin_va_end(v)

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25


#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED  4
#define MAGENTA 5
#define BROWN 6
#define LIGHT_GRAY 7
#define GRAY 8
#define LIGHT_BLUE 9
#define LIGHT_GREEN 10
#define LIGHT_CYAN 11
#define LIGHT_RED 12
#define LIGHT_MAGENTA 13
#define YELLOW  14
#define WHITE_COLOR 15

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64


#define SCROLL_BUFFER_SIZE 100





static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


void clear_screen(char* vidptr);
unsigned int ft_strlen(const char *s);
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void disable_cursor(void);
void update_cursor(int x, int y);


void keyboard_init(void);
bool keyboard_has_input(void);
char keyboard_getchar(void);
uint8_t keyboard_get_scancode(void);


void print_char(char c, char *vidptr, unsigned int *position, uint8_t color);


void scroll_up(char *vidptr);
void scroll_down(char *vidptr);
void clear_scroll_buffer(void);
void redraw_from_buffer(char *vidptr);
void scroll_screen(char *vidptr);


void set_print_color(uint8_t color);
void init_printf_position(void);
int printf(const char *format, ...);
int printk(const char *format, ...);
int ft_putchar(char c);
int ft_putstr(char *str);
int ft_putnbr(int n);
int ft_putnbrunsigned(unsigned int n);
int ft_putnbrlowehex(unsigned int n);
int ft_putnbruperhex(unsigned int n);
int ft_printaddress(unsigned long addr);
int ft_printfspesific(const char format, va_list args);
void write_string( int colour, const char *string );
#endif