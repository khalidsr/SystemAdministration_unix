NAME = ft_nmap

LIB = include/ft_nmap.h

SRCS =	src/main.c\
		src/args.c\
		src/utils.c\
		src/scanner.c


CC = cc #-fsanitize=address

CFLAGS = -Wall -Wextra -Werror

OBJCS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(SRCS) $(LIB) 
		$(CC)  $(CFLAGS) -o $(NAME) $(SRCS)

clean:
	@/bin/rm -f $(OBJCS)

fclean: clean
	@/bin/rm -f $(NAME)

re : fclean all

.PHONY: all clean fclean re