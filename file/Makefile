NAME	:= gomoku
# CFLAGS	:= -Wextra -Wall -Werror -Ofast -g -DDEBUG=1
CFLAGS	:= -Ofast -g -DDEBUG=1

LIBMLX	:= ./MLX42
INCDIR = includes
LGLFW_PATH := /usr/lib/x86_64-linux-gnu/libglfw.so
# LGLFW_PATH := $(shell brew --prefix glfw)
HEADERS	:= -I ./include -I $(LIBMLX)/include
LIBS	:= $(LIBMLX)/build/libmlx42.a -ldl -lglfw -pthread -lm -L $(LGLFW_PATH)/lib/

SRCS    := src/main.c \
           src/hook.c \
           src/graphicsUtils.c \
           src/utils.c \
           src/timer.c \
           src/information.c \
           src/victory.c \
           src/captures.c \
           src/heuristics.c \
           src/ai.c

OBJS	= ${SRCS:.c=.o}

all: libmlx $(NAME)


mlx:
	@git clone https://github.com/codam-coding-college/MLX42.git $(LIBMLX)

libmlx:
	@cmake $(LIBMLX) -B $(LIBMLX)/build && make -C $(LIBMLX)/build -j4

%.o: %.c
	@$(CC) $(CFLAGS) $(HEADERS) -o $@ -c $< && printf "Compiling: $(notdir $<)\n"

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