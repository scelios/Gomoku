NAME        := gomoku

# --- CONFIGURATION OPTIMISÉE ---

# Choix du compilateur (clang est souvent légèrement plus rapide que gcc pour l'optimisation)
CC          := gcc 

# Flags de base (Sécurité)
BASE_FLAGS  := -Wall -Wextra -Werror

# Flags pour le DEBUG (make re MODE=debug)
DEBUG_FLAGS := -g3 -DDEBUG=1 -fsanitize=address

# Flags pour la PERFORMANCE (make re)
# -Ofast : Optimisation agressive (plus fort que -O3)
# -march=native : Utilise les instructions CPU modernes (AVX/SSE) -> GAIN ÉNORME
# -flto : Optimisation lors de l'édition des liens (inlining inter-fichiers)
# -funroll-loops : Déroule les petites boucles (utile pour tes directions[8])
# -DNDEBUG : Désactive les asserts et macros de debug
PERF_FLAGS  := -Ofast -march=native -flto -funroll-loops -DNDEBUG

# Gestion automatique du mode
ifdef MODE
    ifeq ($(MODE), debug)
        CFLAGS := $(BASE_FLAGS) $(DEBUG_FLAGS)
    else
        CFLAGS := $(BASE_FLAGS) $(PERF_FLAGS)
    endif
else
    CFLAGS := $(BASE_FLAGS) $(PERF_FLAGS)
endif

# --- LIBRAIRIES ---
LIBMLX      := ./MLX42
INCDIR      := includes
LGLFW_PATH  := /usr/lib/x86_64-linux-gnu/libglfw.so
HEADERS     := -I ./include -I $(LIBMLX)/include
LIBS        := $(LIBMLX)/build/libmlx42.a -ldl -lglfw -pthread -lm -L $(LGLFW_PATH)/lib/

SRCS        := src/main.c \
               src/hook.c \
               src/graphicsUtils.c \
               src/utils.c \
               src/timer.c \
               src/information.c \
               src/victory.c \
               src/captures.c \
               src/heuristics.c \
               src/ai.c

OBJS        = ${SRCS:.c=.o}

all: libmlx $(NAME)

mlx:
	@git clone https://github.com/codam-coding-college/MLX42.git $(LIBMLX)

libmlx:
	@cmake $(LIBMLX) -B $(LIBMLX)/build && make -C $(LIBMLX)/build -j4

%.o: %.c
	@$(CC) $(CFLAGS) $(HEADERS) -o $@ -c $< && printf "Compiling: $(notdir $<)\n"

$(NAME): $(OBJS)
	# Note: On ajoute CFLAGS ici aussi pour que le Linker (ld) utilise -flto
	@$(CC) $(CFLAGS) $(OBJS) $(LIBS) $(HEADERS) -o $(NAME) && printf "Linking: $(NAME) with flags: $(CFLAGS)\n"

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