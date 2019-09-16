
CC = gcc

BASE_ADDRESS := 0x200000

# NOTE: mingw64-gcc uses ms_abi in default so we don't have to do it manually...
CFLAGS = -masm=intel -fstack-check=no -nodefaultlibs -nostdlib -ffreestanding -lgcc -lmsvcrt 

# We'll create UEFI application image
LDFLAGS = -Wl,--dll,--shared,--exclude-all-symbols,$\
		--image-base,$(BASE_ADDRESS),--dynamicbase,--subsystem,10 $\
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
	include						\
	include/guid				\
	include/industrystandard	\
	include/ipf					\
	include/library				\
	include/pi					\
	include/ppi					\
	include/protocol			\
	include/uefi				\
	include/x64

# Our source path
SRCS := \
	oskernel.c \
	osmemory.c \
	peimage.c \
	sformat.c \
	uefimain.c

# Our dependency, object, preprocessed, assembly file names will be derived by source file names...
DEPS := $(SRCS:.c=.d)
OBJS := $(SRCS:.c=.o)
PREPS := $(notdir $(SRCS:.c=.i))
ASMS := $(notdir $(SRCS:.c=.s))

# Our include directory options
INC_PARAMS := $(INC:%=-I%)

# Our UEFI application image name
TARGET := BOOTx64.efi

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

	
