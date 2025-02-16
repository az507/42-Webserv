# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: xzhang <marvin@42.fr>                      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/11/11 17:52:33 by xzhang            #+#    #+#              #
#    Updated: 2025/02/16 14:26:57 by achak            ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
SRCS = main.cpp src/ConfigFile.cpp src/Server.cpp src/Client.cpp src/ClientParseHttp.cpp \
	   src/ClientPerformRequest.cpp  src/ClientRunCgi.cpp src/ClientUtils.cpp src/Cgi.cpp
CPP = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP -Iinclude -O2 -DNDEBUG#-g -fsanitize=address
OBJS = $(SRCS:.cpp=.o)
DEP	= $(OBJS:.o=.d)
REMOVE = rm -f

#------------------------------------------------------------------------

# colours
GREEN = \033[0;32m
B_GREEN = \033[1;32m
BROWN = \033[0;33m
B_BROWN = \033[1;33m
B_RED    = \033[1;31m
B_BLUE   = \033[1;34m
CYAN   = \033[0;36m
B_CYAN   = \033[1;36m
END = \033[0m  #rest the Reset special formatting (such as colour)

#------------------------------------------------------------------------

all: $(NAME)

-include $(DEP)

$(NAME): $(OBJS)
	@$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)
	@echo "$(B_CYAN)$(NAME) compiled$(END)"

%.o:%.cpp
	@$(CPP) -c $(CPPFLAGS) $< -o $@
	@echo "$(B_CYAN)$< compiled.$(END)"

clean:
	@$(REMOVE) $(OBJS) $(DEP)
	@echo "$(B_CYAN)clean done$(END)"

fclean: clean
	@$(REMOVE) $(NAME) $(OBJS) $(DEP)
	@echo "$(B_CYAN)fclean done$(END)"

re: fclean $(NAME)

.PHONY: all clean fclean re





















#/* ····························································· */
#/* :██╗----██╗███████╗██████╗-███████╗███████╗██████╗-██╗---██╗: */
#/* :██║----██║██╔════╝██╔══██╗██╔════╝██╔════╝██╔══██╗██║---██║: */
#/* :██║-█╗-██║█████╗--██████╔╝███████╗█████╗--██████╔╝██║---██║: */
#/* :██║███╗██║██╔══╝--██╔══██╗╚════██║██╔══╝--██╔══██╗╚██╗-██╔╝: */
#/* :╚███╔███╔╝███████╗██████╔╝███████║███████╗██║--██║-╚████╔╝-: */
#/* :-╚══╝╚══╝-╚══════╝╚═════╝-╚══════╝╚══════╝╚═╝--╚═╝--╚═══╝--: */
#/* ····························································· */



