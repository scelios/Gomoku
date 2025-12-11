NAME	:= gomoku
# CFLAGS	:= -Wextra -Wall -Werror -Ofast -g -DDEBUG=1
CFLAGS	:= -O3 -march=native
LIBMLX	:= ./file/MLX42
INCDIR = includes
LGLFW_PATH := /usr/lib/x86_64-linux-gnu/libglfw.so
# LGLFW_PATH := $(shell brew --prefix glfw)
HEADERS	:= -I ../include -I $(LIBMLX)/include
LIBS	:= $(LIBMLX)/build/libmlx42.a -ldl -lglfw -pthread -lm -L $(LGLFW_PATH)/lib/

SRCS	= $(shell find ./file/src -iname "*.c")
# SRCS	:= ./file/src/test.c

OBJS	= ${SRCS:.c=.o}

all: libmlx $(NAME)

mlx:
	@git clone https://github.com/codam-coding-college/MLX42.git $(LIBMLX)

docker: all
	@cp gomoku ./file/gomoku
	docker compose up --build -d

libmlx:
	@cmake $(LIBMLX) -B $(LIBMLX)/build && make -C $(LIBMLX)/build -j4

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ -c $< -I$(INCDIR) && printf "Compiling: $(notdir $<)\n"

$(NAME): $(OBJS)
	@$(CC) $(OBJS) $(LIBS) $(HEADERS) -o $(NAME) && printf "Linking: $(NAME)\n"

clean:
	@rm -f $(OBJS)
	@printf "Cleaned object files.\n"

fclean: clean
	@rm -f $(NAME)
	@cmake --build $(LIBMLX)/build --target clean
	@printf "Removed executable: $(NAME).\n"

re: fclean all

git: fclean
	git add *
	git commit -m "auto commit"
	git push

brew:
	brew install glfw

.PHONY: all clean fclean re libmlx git brew