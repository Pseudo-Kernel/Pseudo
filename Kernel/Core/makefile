
# This is a makefile for kernel build

CC = gcc

KERNEL_FAKE_BASE_ADDRESS := 0x400000

# NOTE: mingw64-gcc uses ms_abi in default so we don't have to do it manually...
CFLAGS = -masm=intel -fstack-check=no -nodefaultlibs -nostdlib -ffreestanding -lgcc -lmsvcrt 

# We'll create UEFI application image
LDFLAGS = -Wl,--dll,--shared,--exclude-all-symbols,$\
		--image-base,$(KERNEL_FAKE_BASE_ADDRESS),--dynamicbase,--subsystem,native $\
		-fstack-check=no

# Invoking GCC
# https://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html#Invoking-GCC

# GetFileList (filter)
# GetFileListRelative = $(shell powershell "Get-ChildItem -Filter $1 -Recurse -Force | foreach { Resolve-Path -Path $$_.FullName -Relative }")


#
# NOTE: Filenames must be lowercase because of case-sensitive evil...
#

# Our include path
INC := \
	../include		\
	.

# Our source path
SRCS := \
	main.c				\
	stdio.c             \
	dbg\dbg.c           \
	init\bootgfx.c      \
	init\preinit.c      \
	init\zipread.c      \
	ke\lock.c           \
	misc\common.c       \
	misc\list.c         \
	mm\mminit.c         \
	mm\pool.c

# Our dependency, object, preprocessed, assembly file names will be derived by source file names...
DEPS := $(SRCS:.c=.d)
OBJS := $(SRCS:.c=.o)
PREPS := $(notdir $(SRCS:.c=.i))
ASMS := $(notdir $(SRCS:.c=.s))

# Our include directory options
INC_PARAMS := $(INC:%=-I%)

# Our UEFI application image name
TARGET := core.sys

# The absolute path of the makefile's parent directory
#SRCDIR := $(CURDIR)

# Our linker script
#LINKER_SCRIPT := linker.ld


.PHONY: clean all
#.SUFFIXES: .c .o .d

all: clean run

clean:
	@echo cleaning...
	$(shell cmd /C "del $(TARGET) $(OBJS) $(PREPS) $(ASMS)")

run: $(TARGET)
	@echo Running '$(TARGET)' ...
	-@$(TARGET)

$(TARGET): $(OBJS)
	@echo SRCS: $(SRCS)
	@echo DEPS: $(DEPS)
	@echo OBJS: $(OBJS)
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $^

#i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc
	
	
# NOTE: DO NOT put $(DEPS) : $(SRCS) here because %.d: %.c does all work.
# NOTE: Make sure that source and destination is in the same directory when you're writing rules like %.a: %.b.

-include $(DEPS)

%.d: %.c
	@echo Creating dependency for '$<' ...
	@$(CC) $(INC_PARAMS) -E -MD -MP $< -MF $@ >nul

%.o: %.c
	@echo Compiling '$<' ...
	@$(CC) $(INC_PARAMS) -save-temps $(CFLAGS) -c $< -o $@

	
