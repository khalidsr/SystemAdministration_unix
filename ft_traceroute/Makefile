NAME = ft_traceroute

LIB = ft_traceroute.h

SRCS =	main.c\
		utils.c\
		ft_traceroute.c


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