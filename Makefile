# --------------------Paths----------------------------#

OBJS_PATH = objs/

#--------------------Files-----------------------------#

SRCS =	config_main.cpp\
		Server.cpp\
		Client.cpp\
		Http_req.cpp\
		Config.cpp\
		utils.cpp

NAME = webserv

#--------------------Commands---------------------------#

CLANG = clang++

FLAGS = -Wall -Werror -Wextra -std=c++98

MKDIROBJS = @mkdir -p ${OBJS_PATH}

OBJ = 	${SRCS:.cpp=.o}

OBJS = 	${addprefix ${OBJS_PATH}, ${OBJ}}

RM = rm -rf

#----------------------Rules----------------------------#

${OBJS_PATH}%.o : %.cpp
			${MKDIROBJS}
			${CLANG} $(FLAGS) -c $< -o $@

$(NAME):	${OBJS}
			${CLANG} ${FLAGS} ${OBJS} -o ${NAME}

all:		${NAME}

clean:
			${RM} ${OBJS_PATH}

fclean:		clean
			${RM} ${NAME}

re:			fclean all

.phony:		clean fclean re all