NAME =  MattDaemon

CPP = c++

CPPFLAGS = -Wall -Werror -Wextra -I. -Iutils

SRC = main.cpp\
		MattDaemon.cpp\
		Tintin_reporter.cpp\
		./utils/auth.cpp\
		./utils/remote_shell.cpp\
		./utils/email.cpp\


HEADERS = MattDaemon.hpp
			

OBJ = $(SRC:.cpp=.o)

all : $(NAME)
	

$(NAME): $(OBJ) $(HEADERS)
		$(CPP) $(CPPFLAGS) $(SRC)  -o $(NAME)

clean: 
	@/bin/rm -f $(OBJ)

fclean: clean
	@/bin/rm -f $(NAME)

re : fclean all

.PHONY : all clean fclean re