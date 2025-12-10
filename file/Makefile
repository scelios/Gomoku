NAME        := gomoku

# ==========================================
# FLAGS D'OPTIMISATION & COMPILATION
# ==========================================
# -Ofast         : Optimisation max (plus fort que -O3)
# -march=native  : Utilise les instructions CPU de ton ordi (AVX, etc.)
# -flto          : Optimisation globale entre les fichiers
# -pthread       : Nécessaire pour le multithreading si utilisé
# On retire -g et -DDEBUG pour la performance pure
CFLAGS      := -Wall -Wextra -Werror -Ofast -march=native -flto

# Si tu veux debug, décommente la ligne ci-dessous et commente celle du dessus :
# CFLAGS    := -Wall -Wextra -Ofast -g -DDEBUG=1

# ==========================================
# LIBRAIRIES & PATHS
# ==========================================
LIBMLX      := ./MLX42
INCDIR      := includes
# Détection automatique du chemin GLFW selon l'OS (Linux vs Mac)
UNAME_S     := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
    LGLFW_PATH := /usr/lib/x86_64-linux-gnu/libglfw.so
    LIBS       := $(LIBMLX)/build/libmlx42.a -ldl -lglfw -pthread -lm
else
    # Configuration MacOS (au cas où tu changes de machine ou pour correcteur)
    LGLFW_PATH := $(shell brew --prefix glfw)
    LIBS       := $(LIBMLX)/build/libmlx42.a -ldl -lglfw -pthread -lm -L$(LGLFW_PATH)/lib
endif

HEADERS     := -I ../include -I $(LIBMLX)/include

SRCS        := $(shell find ./src -iname "*.c")
OBJS        := ${SRCS:.c=.o}
CC          := cc

# ==========================================
# RULES
# ==========================================

all: libmlx $(NAME)

mlx:
	@if [ ! -d "$(LIBMLX)" ]; then \
		git clone https://github.com/codam-coding-college/MLX42.git $(LIBMLX); \
	fi

libmlx: mlx
	@cmake $(LIBMLX) -B $(LIBMLX)/build && make -C $(LIBMLX)/build -j4

# Compilation des objets
%.o: %.c
	@$(CC) $(CFLAGS) -o $@ -c $< -I$(INCDIR) && printf "Compiling: $(notdir $<)\n"

# Linkage final (ajout de CFLAGS ici aussi pour que -flto fonctionne !)
$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) $(LIBS) $(HEADERS) -o $(NAME) && printf "Linking: $(NAME)\n"

clean:
	@rm -f $(OBJS)
	@printf "Cleaned object files.\n"

fclean: clean
	@rm -f $(NAME)
	@rm -rf $(LIBMLX)/build
	@printf "Removed executable and libmlx build.\n"

re: fclean all

# ==========================================
# UTILS
# ==========================================
git: fclean
	git add *
	git commit -m "auto commit"
	git push

brew:
	brew install glfw

.PHONY: all clean fclean re libmlx git brew