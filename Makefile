#───────────────────────────  PROJECT BASICS  ────────────────────────────────#
NAME = Famine
CC   = cc
RM   = rm -rf
DEBUG ?= 0
FLAGS = -Werror -Wextra -Wall -g -Iinclude -DDEBUG=$(DEBUG) #-fsanitize=address
MAKE := make --no-print-directory

#--------------------------------------SOURCES---------------------------------#
SRC_DIR = src/
SRC = main.c elf.c inject.c infect.c stub_data.c
SRCS = $(addprefix $(SRC_DIR), $(SRC))

#--------------------------------------OBJECTS----------------------------------#
OBJ_DIR  = Objects/
OBJECTS  = $(patsubst %.c,$(OBJ_DIR)%.o,$(SRC))

#--------------------------------------STUB (asm)-------------------------------#
NASM = nasm
NASMFLAGS = -f bin -dDEBUG=$(DEBUG)
STUB_ASM = $(SRC_DIR)stub.asm
STUB_BIN = stub.bin
STUB_HDR = include/stub.h

#────────────────────────────  ANIMATION CONFIG  ─────────────────────────────#
ANIMATION_FRAMES = ⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏
ANIMATION_COLOR  = '\033[1;36m'
TOTAL_FILES := $(words $(OBJECTS))

#───────────────────────────────  COLOR CODES  ───────────────────────────────#
NONE='\033[0m'
GREEN='\033[32m'
YELLOW='\033[33m'
GRAY='\033[2;37m'
CURSIVE='\033[3m'
BLUE='\033[34m'
MAGENTA='\033[35m'
CYAN='\033[36m'
WHITE='\033[37m'
BOLD='\033[1m'

#────────────────────────────────  RULES  ─────────────────────────────────────#
all: $(STUB_HDR) reset_counter $(OBJ_DIR) $(NAME)

$(STUB_HDR): $(STUB_ASM)
	@$(NASM) $(NASMFLAGS) $(STUB_ASM) -o $(STUB_BIN)
	@xxd -i $(STUB_BIN) > $(STUB_HDR)
	@printf $(BOLD)$(MAGENTA)"Stub assembled -> $(STUB_HDR)\n"$(NONE)

reset_counter:
	@rm -f .counter
	@printf "0" > .counter

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@printf $(BOLD)$(MAGENTA)"Objects directory created\n"$(NONE)

$(NAME): $(OBJECTS)
	@$(CC) $(FLAGS) $(OBJECTS) -o $(NAME)
	@printf "\033[1;32m\n✅ $(NAME) successfully compiled!\n\033[0m"
	@rm .counter

#────────────────────────────  COMPILATION RULE  ─────────────────────────────#
$(OBJ_DIR)stub_data.o: $(STUB_HDR)

$(OBJ_DIR)%.o: $(SRC_DIR)%.c
	@mkdir -p $(OBJ_DIR)
	@if [ ! -f .counter ]; then printf "0" > .counter; fi
	@file_count=$$(cat .counter); \
	file_count=$$((file_count + 1)); \
	printf $$file_count > .counter; \
	frames="⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏"; \
	frame_index=$$((file_count % 10)); \
	frame=$$(printf $$frames | cut -d ' ' -f $$((frame_index + 1))); \
	percent=$$((100 * file_count / $(TOTAL_FILES))); \
	barlen=30; \
	done=$$((barlen * percent / 100)); \
	todo=$$((barlen - done)); \
	bar=$$(printf "█%.0s" $$(seq 1 $$done)); \
	space=$$(printf "░%.0s" $$(seq 1 $$todo)); \
	printf "\r\033[1;36m%s \033[1mCompiling\033[0m [%-*s] %3d%% \033[36m%-40.40s\033[0m" "$$frame" "$$barlen" "$$bar$$space" "$$percent" "$(SRC_DIR)$*.c"; \
	$(CC) $(FLAGS) -c $(SRC_DIR)$*.c -o $@

clean:
	@printf $(CURSIVE)$(GRAY)" -> Cleaning object files..\n"$(NONE)
	@$(RM) $(OBJ_DIR)
	@$(RM) .counter

fclean: clean
	@printf $(CURSIVE)$(GRAY)" -> Removing $(NAME)\n"$(NONE)
	@$(RM) $(NAME) $(STUB_BIN) $(STUB_HDR)

re: fclean all

.PHONY: all clean fclean re reset_counter
