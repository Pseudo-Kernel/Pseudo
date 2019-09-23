
# This is a makefile for total build

# NOTE: Filenames must be lowercase because of case-sensitive evil...

POWERSHELL := powershell.exe
ZIPIMAGE := zipimage.exe
BOOTIMGCREATE := boot_image.ps1

# Our OS filesystem image name
TARGET := vm.vhd
# Our kernel binary name (same as $(BUILD_RELATIVE_PATH_KERNEL)/makefile)
TARGET_KERNEL := core.sys
# Our efi bootloader binary name (same as $(BUILD_RELATIVE_PATH_OSLOADER)/makefile)
TARGET_OSLOADER := BOOTx64.efi

# Relative path of makefile for kernel build
BUILD_RELATIVE_PATH_KERNEL := ./kernel/core
# Relative path of makefile for efi osloader build
BUILD_RELATIVE_PATH_OSLOADER := ./osloader/efibootlite

# Relative path of filesystem content to copy.
FILESYSTEM_RELATIVE_PATH := ./vm/filesystem

CURDIR := $(shell cd)


# QEMU := ./vm/qemu/qemu-system-x86_64.exe
# QEMU_ARGS := -S -gdb tcp:127.0.0.1:7000 -debugcon stdio -L . -smp cpus=4,cores=4 -m size=512m,maxmem=8g --bios "./vm/OVMF/ovmf-x64/OVMF-pure-efi.fd" -net none -hda "./VM/boot_image.vhd"


.PHONY: clean all

all: clean run

clean:
	@echo cleaning...
	@cd $(BUILD_RELATIVE_PATH_OSLOADER) && $(MAKE) clean
	@cd $(BUILD_RELATIVE_PATH_KERNEL) && $(MAKE) clean

# Starts a virtual machine.
run: image

image: $(TARGET)


# FIXME: Normalize path separator '\' '/'
$(TARGET): $(TARGET_OSLOADER) $(TARGET_KERNEL)
	@copy "$(BUILD_RELATIVE_PATH_OSLOADER)\$(TARGET_OSLOADER)" "$(FILESYSTEM_RELATIVE_PATH)/EFI/BOOT/" /y
	@copy "$(BUILD_RELATIVE_PATH_KERNEL)\$(TARGET_KERNEL)" "$(FILESYSTEM_RELATIVE_PATH)/EFI/BOOT/" /y
	$(ZIPIMAGE) "$(FILESYSTEM_RELATIVE_PATH)/EFI/BOOT/init.bin" "$(FILESYSTEM_RELATIVE_PATH)/EFI/BOOT/init"
	$(POWERSHELL) -ExecutionPolicy RemoteSigned -File $(BOOTIMGCREATE) -BootImagePath "$(CURDIR)\$(TARGET)" -FilesystemContentsPath "$(FILESYSTEM_RELATIVE_PATH)"

# Loader build.
$(TARGET_OSLOADER):
	@cd $(BUILD_RELATIVE_PATH_OSLOADER) && $(MAKE) $(TARGET_OSLOADER)

# Kernel build.
$(TARGET_KERNEL):
	@cd $(BUILD_RELATIVE_PATH_KERNEL) && $(MAKE) $(TARGET_KERNEL)


