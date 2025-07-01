NAME =  MattDeamon

CPP = c++

CPPFLAGS = #-Wall -Werror -Wextra

SRC = main.cpp\
		MattDeamon.cpp\
		utils.cpp

HEADERS = MattDeamon.hpp
			

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