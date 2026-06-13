ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME   = libft_malloc_$(HOSTTYPE).so
LINK   = libft_malloc.so

SRCS   = malloc.c free.c realloc.c utils.c visualize.c
OBJS   = $(SRCS:.c=.o)

CC     = gcc
CFLAGS = -Wall -Wextra -Werror -fPIC -I.

all: $(NAME) $(LINK)

$(NAME): $(OBJS)
	$(CC) -shared -o $@ $^ -lc

$(LINK): $(NAME)
	ln -sf $(NAME) $(LINK)

%.o: %.c malloc.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME) $(LINK)

re: fclean all

.PHONY: all clean fclean re