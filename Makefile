#───────────────────────────  PROJECT BASICS  ────────────────────────────────#
NAME := Famine
DEBUG ?= 0
XXD := xxd
OBJCOPY := objcopy

CFLAGS := -Wall -Wextra -Werror -Iinclude -MP -MMD -DDEBUG=$(DEBUG) $(if $(DEBUG),-g)
LFLAGS :=

LINK_SCRIPT := linker.ld

STUB_CFLAGS := $(CFLAGS) -ffreestanding -flto -Os -DNOSTD=1 -nostdlib -DCREATE_STUB=1 -fno-stack-protector
STUB_LFLAGS := $(LFLAGS) -pie -nostartfiles -Wl,-e,_start -ffreestanding -Wl,-T,$(LINK_SCRIPT) -Wl,--build-id=none

STUB_ELF := stub.elf
STUB_HDR := include/stub.h
STUB_BIN := stub.bin

#--------------------------------------SOURCES--------------------------------#
SRC_DIR := src
SHARED_SRC := main.c elf.c inject.c infect.c std.c walk.c x64-syscall.c
EXE_SRC := $(SHARED_SRC) stub_data.c
STUB_SRC := $(SHARED_SRC)


#--------------------------------------OBJECTS--------------------------------#
OBJ_DIR := Objects
EXE_OBJ_DIR := $(OBJ_DIR)/$(NAME)
STUB_OBJ_DIR := $(OBJ_DIR)/stub
EXE_OBJ := $(patsubst %.c,$(EXE_OBJ_DIR)/%.o,$(EXE_SRC))
STUB_OBJ := $(patsubst %.c,$(STUB_OBJ_DIR)/%.o,$(STUB_SRC))

EXE_DEP := $(EXE_OBJ:%.o=%.d)
STUB_DEP := $(STUB_OBJ:%.o=%.d)


#────────────────────────────  ANIMATION CONFIG  ─────────────────────────────#
ANIMATION_FRAMES := ⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏
ANIMATION_COLOR := '\033[1;36m'
ALL_FILES := $(EXE_OBJ) $(STUB_OBJ)
TOTAL_FILES := $(words $(ALL_FILES))
BAR_LEN := 30
ifeq ($(CALC_UPDATE),1)
NEED_UPDATE := 0
else
NEED_UPDATE := $(foreach f, $(ALL_FILES), $(shell $(MAKE) CALC_UPDATE=1 --no-print-directory -q $(f); echo $$?))
endif
NEED_UPDATE := $(shell echo $(NEED_UPDATE) | tr ' ' '+' | bc)

#───────────────────────────────  COLOR CODES  ───────────────────────────────#
NONE := '\033[0m'
GREEN := '\033[32m'
YELLOW := '\033[33m'
GRAY := '\033[2;37m'
CURSIVE := '\033[3m'
BLUE := '\033[34m'
MAGENTA := '\033[35m'
CYAN := '\033[36m'
WHITE := '\033[37m'
BOLD := '\033[1m'

#────────────────────────────────  RULES  ────────────────────────────────────#
$(NAME): $(EXE_OBJ)
	@$(CC) $(CFLAGS) $(LFLAGS) $(EXE_OBJ) -o $@
	@printf "\033[1;32m\n✅ $(NAME) successfully compiled!\n\033[0m"

.PHONY: all
all: $(NAME)

$(STUB_ELF): $(STUB_OBJ) $(LINK_SCRIPT)
	@$(CC) $(STUB_CFLAGS) $(STUB_LFLAGS) $(STUB_OBJ) -o $@

$(STUB_BIN): $(STUB_ELF)
	@$(OBJCOPY) -O binary $(STUB_ELF) $@ -j .single_section

$(STUB_HDR): $(STUB_BIN)
	@$(XXD) -i $(STUB_BIN) > $@
	@printf "\r\033[K"$(BOLD)$(MAGENTA)"Stub assembled -> $(STUB_HDR)\n"$(NONE)

$(EXE_OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(call comp_unit,$< -> $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(EXE_OBJ_DIR)/stub_data.o: $(STUB_HDR)

$(STUB_OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(call comp_unit,$< -> $@)
	@$(CC) $(STUB_CFLAGS) -c $< -o $@

.PHONY: clean
clean: reset_counter
	@printf $(CURSIVE)$(GRAY)" -> Cleaning object files..\n"$(NONE)
	@$(RM) -r $(OBJ_DIR) $(STUB_ELF) $(STUB_BIN) $(STUB_HDR)

.PHONY: fclean
fclean: clean
	@printf $(CURSIVE)$(GRAY)" -> Removing $(NAME)\n"$(NONE)
	@$(RM) $(NAME)

.PHONY: re
re: fclean reset_counter all

-include $(EXE_DEP) $(STUB_DEP)

.PHONY: reset_counter
reset_counter:
	$(eval NEED_UPDATE := $(TOTAL_FILES))

define comp_unit
	$(eval NEED_UPDATE := $(shell echo $$(($(NEED_UPDATE) - 1))))
	file_count="$$(($(TOTAL_FILES) - $(NEED_UPDATE)))"; \
	frames="⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏"; \
	frame_index=$$((file_count % 10)); \
	frame=$$(printf $$frames | cut -d ' ' -f $$((frame_index + 1))); \
	percent=$$((100 * file_count / $(TOTAL_FILES))); \
	barlen=$(BAR_LEN); \
	done=$$((barlen * percent / 100)); \
	bar=$$(printf "█%.0s" $$(seq 1 $$done)); \
	todo=$$((barlen - done)); \
	space=$$(yes "░" | head -n $$todo | tr -d '\n'); \
	printf "\r\033[1;36m%s \033[1mCompiling\033[0m [%-*s] %3d%% \033[36m%-80.80s\033[0m" "$$frame" "$$barlen" "$$bar$$space" "$$percent" "$(1)"
endef

ifeq ($(CALC_UPDATE),1)
override define comp_unit
endef
endif
