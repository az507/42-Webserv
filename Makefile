NAME	= a.out
CC		= c++
CFLAGS	= -Wall -Werror -Wextra -std=c++98 -MMD -MP -Iincludes -g#-fsanitize=address
SRCS	= $(shell find srcs -name "*.cpp")
OBJ_DIR	= objs
OBJS	= $(addprefix $(OBJ_DIR)/,$(notdir $(SRCS:.cpp=.o)))
DEP		= $(OBJS:.o=.d)
VPATH	= $(sort $(dir $(SRCS)))

all:	$(NAME)

abc:
	$(CC) $(CFLAGS) abc.cpp -o $(NAME)

test:
	$(CC) $(CFLAGS) test.cpp -o $(NAME)

$(NAME):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME) $(LFLAGS)

-include $(DEP)

$(OBJ_DIR)/%.o:%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean:	clean
	rm -f $(NAME)

re:	fclean all

.PHONY:	all clean fclean re
