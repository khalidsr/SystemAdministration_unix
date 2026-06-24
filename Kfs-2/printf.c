#include "kernel.h"

static char *global_vidptr = (char*)VGA_ADDRESS;
static unsigned int global_position = 0;
static uint8_t printf_color = WHITE_COLOR;
static int cursor_x = 0;
static int cursor_y = 0;


void set_print_color(uint8_t color)
{
    printf_color = color;
}


void init_printf_position(void)
{
    global_position = (cursor_y * VGA_WIDTH + cursor_x) * 2;
}

int ft_putchar(char c)
{
  
    print_char(c, global_vidptr, &global_position, printf_color);
    return (1);
}

unsigned int ft_strlen(const char *s)
{
    unsigned int i = 0;

    while(s[i] != '\0')
        i++;
    return i;
}

void print_char(char c, char *vidptr, unsigned int *position, uint8_t color)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    } 
    else if (c == '\t') 
    {
        int spaces = 4 - (cursor_x % 4);
        int i;
        for (i = 0; i < spaces; i++)
        {
            vidptr[*position] = ' ';
            vidptr[*position + 1] = color;
            *position += 2;
            cursor_x++;
            
            if (cursor_x >= VGA_WIDTH)
            {
                cursor_x = 0;
                cursor_y++;
            }
        }
    } 
    else 
    {
        vidptr[*position] = c;
        vidptr[*position + 1] = color;
        *position += 2;
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) 
    {
        cursor_x = 0;
        cursor_y++;
    }
    
   
    if (cursor_y >= VGA_HEIGHT)
    {
        scroll_screen(vidptr);
        cursor_y = VGA_HEIGHT - 1;  
        cursor_x = 0;
    }
    

    *position = (cursor_y * VGA_WIDTH + cursor_x) * 2;
    
    update_cursor(cursor_x, cursor_y);
}


int ft_putstr(char *str)
{
    int count = 0;
    
    if (!str)
    {
        ft_putstr("(null)");
        return (6);
    }
    
    while (*str)
    {
        ft_putchar(*str);
        str++;
        count++;
    }
    return (count);
}

int ft_putnbr(int n)
{
    int count = 0;
    long num = n;
    
    if (num < 0)
    {
        ft_putchar('-');
        num = -num;
        count++;
    }
    
    if (num >= 10)
        count += ft_putnbr(num / 10);
    
    ft_putchar((num % 10) + '0');
    count++;
    
    return (count);
}


int ft_putnbrunsigned(unsigned int n)
{
    int count = 0;
    
    if (n >= 10)
        count += ft_putnbrunsigned(n / 10);
    
    ft_putchar((n % 10) + '0');
    count++;
    
    return (count);
}


int ft_putnbrlowehex(unsigned int n)
{
    int count = 0;
    const char *hex = "0123456789abcdef";
    
    if (n >= 16)
        count += ft_putnbrlowehex(n / 16);
    
    ft_putchar(hex[n % 16]);
    count++;
    
    return (count);
}


int ft_putnbruperhex(unsigned int n)
{
    int count = 0;
    const char *hex = "0123456789ABCDEF";
    
    if (n >= 16)
        count += ft_putnbruperhex(n / 16);
    
    ft_putchar(hex[n % 16]);
    count++;
    
    return (count);
}

int ft_printaddress(unsigned long addr)
{
    int count = 0;
    
    if (addr == 0)
    {
        ft_putstr("(nil)");
        return (5);
    }
    
    count += ft_putstr("0x");
    count += ft_putnbrlowehex((unsigned int)addr);
    
    return (count);
}


int ft_printfspesific(const char format, va_list args)
{
    if (format == 'c')
        return (ft_putchar(va_arg(args, int)));
    else if (format == 's')
        return (ft_putstr(va_arg(args, char *)));
    else if (format == 'd')
        return (ft_putnbr(va_arg(args, int)));
    else if (format == 'i')
        return (ft_putnbr(va_arg(args, int)));
    else if (format == 'u')
        return (ft_putnbrunsigned(va_arg(args, unsigned int)));
    else if (format == 'x')
        return (ft_putnbrlowehex(va_arg(args, unsigned int)));
    else if (format == 'X')
        return (ft_putnbruperhex(va_arg(args, unsigned int)));
    else if (format == 'p')
        return (ft_printaddress(va_arg(args, unsigned long)));
    else if (format == '%')
        return (ft_putchar('%'));
    return (1);
}

int vprintf(const char *format, va_list args)
{
    int i = 0, r = 0;
    while (format[i])
    {
        if (format[i] == '%')
        {
            i++;
            r += ft_printfspesific(format[i], args);
        }
        else
            r += ft_putchar(format[i]);
        i++;
    }
    return r;
}

int printf(const char *format, ...)
{
    va_list args;
    int r;
    va_start(args, format);
    r = vprintf(format, args);
    va_end(args);
    return r;
}

int printk(const char *format, ...)
{
    va_list args;
    int r;
    uint8_t old_color;

    old_color = printf_color;
    printf_color = LIGHT_CYAN;
    r = ft_putstr("[KERNEL] ");
    printf_color = old_color;

    va_start(args, format);
    r += vprintf(format, args);
    va_end(args);
    return r;
}