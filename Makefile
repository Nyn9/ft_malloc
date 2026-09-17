.SILENT:

CC 		= cc
CFLAGS 	= -Wall -Wextra -Werror -Iincludes -Ilib

LIBFT = libft.a
LIBPATH = lib

PROG		= cub3D

SRCS_DIR	= srcs/
SRCS		= malloc.c

OBJS_DIR	= objs/
OBJS		= $(patsubst $(SRCS_DIR)%.c,$(OBJS_DIR)%.o,$(SRC))

BONUS		= 0

$(OBJS_DIR)%.o: $(SRCS_DIR)%.c
					@echo "\033[34mCompiling $<..."
					@mkdir -p $(dir $@)
					$(CC) $(CFLAGS) -c $< -o $@

all: 		${PROG}

${PROG}:	${OBJS}
					@echo "\033[33m----Compiling lib----"
					@if [ -f $(MLXPATH)/Makefile ]; then make -s -C $(MLXPATH); fi
					@if [ -f $(LIBPATH)/Makefile ]; then make -s -C $(LIBPATH); fi
					@echo "\n"
					$(CC) $(CFLAGS) ${OBJS} -l:$(LIBPATH)/$(LIBFT) -o ${PROG} 
					@echo "\033[32m Cub3d Compiled! ᕦ(\033[31m♥\033[32m_\033[31m♥\033[32m)ᕤ\n"

clean:
					@if [ -d ./lib ]; then make -s clean -C ./lib; fi
					@ ${RM} $(OBJS_DIR)/*.o
					@ ${RM} -rf $(OBJS_DIR)
					rm -f ${OBJS}

fclean: 	clean
					@if [ -f ./lib/Makefile ]; then make -s fclean -C ./lib; fi
					rm -f ${PROG}
					@echo "\n\033[31mDeleting EVERYTHING! ⌐(ಠ۾ಠ)¬\n"

re:			fclean all


.PHONY: all clean fclean re 