NAME := test

CC := clang

SRC := syscall.c test.c x64-syscall.c
SRC := $(addprefix src/, $(SRC))

LINK_SCRIPT := linker.ld

CFLAGS := -Wall -Wextra \
		  -Os -fpie -fpic -fPIE -fPIC -pie \
		  -nostdlib -flto -nostartfiles -ffreestanding \
		  -Wl,-e,_start -Wl,-T,$(LINK_SCRIPT)

$(NAME): $(SRC) $(LINK_SCRIPT)
	$(CC) $(CFLAGS) $(LFLAGS) $(SRC) -o $(NAME)

test.bin: $(NAME)
	objcopy -O binary $(NAME) test.bin -j .single_section

.PHONY: dump_bin
dump_bin: test.bin
	objdump -b binary --architecture=i386:x86-64 -D test.bin

.PHONY: all
all: $(NAME)

.PHONY: fclean
fclean:
	$(RM) $(NAME)

.PHONY: re
re: fclean all
