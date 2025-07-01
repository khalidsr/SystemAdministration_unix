NAME = ft_ping

LIB = ft_ping.h

SRCS =	main.c\
		utils.c\
		ft_ping.c


CC = cc 

CFLAGS = -Wall -Wextra -Werror

OBJCS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(SRCS) $(LIB) 
		$(CC)  $(CFLAGS) -o $(NAME) $(SRCS) -lm

clean:
	@/bin/rm -f $(OBJCS)

fclean: clean
	@/bin/rm -f $(NAME)

re : fclean all

.PHONY: all clean fclean re