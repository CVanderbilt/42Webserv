# --------------------Paths----------------------------#

OBJS_PATH = /objs/

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

FLAGS = -Wall -Werror -Wextra #-std=c++98

MKDIROBJS = @mkdir -p ${OBJS_PATH}

OBJS = ${SRCS:.cpp=.o}

RM = rm -rf

DEBUG_FLAG = -D DEBUG_MODE

#----------------------Rules----------------------------#

.cpp.o:
			${MKDIROBJS}
			${CLANG} ${FLAGS} -c $< -o ${addprefix ${OBJS_PATH}, ${notdir ${<:.cpp=.o}}}

$(NAME):	${OBJS}
			${CLANG} ${FLAGS} ${addprefix ${OBJS_PATH}, ${notdir ${OBJS}}} -o ${NAME}

all:		$(NAME)

re:			fclean all

clean:
			${RM} ${TMP_PATH}

fclean:
			${RM} ${NAME}
.phony:		clean fclean re all