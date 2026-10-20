.SILENT:

CC          = cc
CFLAGS      = -Wall -Wextra -Werror -fPIC -Iincludes -Ilib

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME        = libft_malloc_$(HOSTTYPE).so
LINK        = libft_malloc.so

LIBPATH     = lib
LIBFT       = $(LIBPATH)/libft.a

SRCS_DIR    = srcs/
SRCS        = free.c utils.c malloc.c realloc.c
OBJS_DIR    = objs/
OBJS        = $(addprefix $(OBJS_DIR),$(SRCS:.c=.o))
HEADERS     = $(wildcard includes/*.h)

TEST_SRC    = tests/test.c
TEST_BIN    = test_malloc
TEST_FLAGS  = -Wall -Wextra -Werror -Wno-free-nonheap-object -Iincludes

all: $(NAME)

$(OBJS_DIR)%.o: $(SRCS_DIR)%.c $(HEADERS)
	@echo "\033[34mCompiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBFT):
	@echo "\033[33m----Compiling libft----"
	@make -s -C $(LIBPATH)

$(NAME): $(LIBFT) $(OBJS)
	$(CC) -shared -o $(NAME) $(OBJS) -L$(LIBPATH) -lft
	@ln -sf $(NAME) $(LINK)
	@echo "\033[32m$(NAME) compiled, $(LINK) -> $(NAME)\033[0m"

test: $(NAME) $(TEST_SRC)
	$(CC) $(TEST_FLAGS) $(TEST_SRC) -o $(TEST_BIN) -L. -lft_malloc -Wl,-rpath,.
	@echo "\033[32mRun: ./$(TEST_BIN)\033[0m"

clean:
	@make -s clean -C $(LIBPATH)
	@$(RM) -r $(OBJS_DIR)

fclean: clean
	@make -s fclean -C $(LIBPATH)
	@$(RM) $(NAME) $(LINK) $(TEST_BIN)
	@echo "\033[31mDeleting EVERYTHING!\033[0m"

re: fclean all

.PHONY: all test clean fclean re