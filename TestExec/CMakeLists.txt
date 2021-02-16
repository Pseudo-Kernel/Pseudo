
# Require a minimum version of CMake.
cmake_minimum_required(VERSION 3.0.0)

# Set our project name and version.
project(
    Pseudo_VMImage
    VERSION 0.1.0)

# Set current directory.
set(CURDIR ${CMAKE_CURRENT_LIST_DIR})

# Set our build tools.
set(POWERSHELL powershell.exe)
set(ZIPIMAGE zipimage.exe)
set(BOOTIMGCREATE boot_image.ps1)
set(QEMU qemu-system-x86_64.exe)

# Set our target.
set(TARGET_IMAGE vm.vhd)                  # Our OS filesystem image name
set(TARGET_KERNEL core.sys)         # Our kernel binary name (same as $(BUILD_RELATIVE_PATH_KERNEL)/)
set(TARGET_OSLOADER BOOTx64.efi)    # Our efi bootloader binary name (same as $(BUILD_RELATIVE_PATH_OSLOADER)/makefile)

# Path of CMakeLists.txt
set(LIST_PATH_KERNEL ${CURDIR}/../source/kernel)        # CMakeLists.txt path for Kernel project
set(LIST_PATH_OSLOADER ${CURDIR}/../source/osloader)    # CMakeLists.txt path for OsLoader project

# kernel binary path.
set(TARGET_PATH_KERNEL ${LIST_PATH_KERNEL}/build/${TARGET_KERNEL})
# osloader binary path.
set(TARGET_PATH_OSLOADER ${LIST_PATH_OSLOADER}/build/${TARGET_OSLOADER})

# Filesystem image work path.
set(FILESYSTEM_PATH ${CURDIR}/filesystem)

# Include Osloader and Kernel.
add_subdirectory(${LIST_PATH_OSLOADER} ${LIST_PATH_OSLOADER}/build)
add_subdirectory(${LIST_PATH_KERNEL} ${LIST_PATH_KERNEL}/build)

# Diskpart only accepts backslash as path separator.
set(DISKPART_ACCEPTABLE_TARGET_IMAGE_PATH ${CURDIR}\\${TARGET_IMAGE})
string(REPLACE "/" "\\" DISKPART_ACCEPTABLE_TARGET_IMAGE_PATH "${DISKPART_ACCEPTABLE_TARGET_IMAGE_PATH}")


add_custom_command(
    OUTPUT
        ${TARGET_IMAGE}                                     # Our target VM image
        "${FILESYSTEM_PATH}/efi/boot/${TARGET_OSLOADER}"    # Our target Osloader binary
        "${FILESYSTEM_PATH}/efi/boot/${TARGET_KERNEL}"      # Our target Kernel binary

    # Copy binaries to filesystem path.
    COMMAND ${CMAKE_COMMAND} -E copy "${TARGET_PATH_OSLOADER}" "${FILESYSTEM_PATH}/efi/boot/"
    COMMAND ${CMAKE_COMMAND} -E copy "${TARGET_PATH_KERNEL}" "${FILESYSTEM_PATH}/efi/boot/"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FILESYSTEM_PATH}/efi/boot/init"

    # Create zip image for our kernel.
    COMMAND ${ZIPIMAGE} "${FILESYSTEM_PATH}/efi/boot/init.bin" "${FILESYSTEM_PATH}/efi/boot/init"

    # Finally, create the FAT32 harddisk image (with GPT partition).
    # NOTE : BootImagePath - Full path for our harddisk image to create. Only backslash is accepted for path separator (because windows wants it).
    COMMAND ${CMAKE_COMMAND} -E rm -f "${DISKPART_ACCEPTABLE_TARGET_IMAGE_PATH}"
    COMMAND ${POWERSHELL} -ExecutionPolicy RemoteSigned -File ${BOOTIMGCREATE} -BootImagePath "${DISKPART_ACCEPTABLE_TARGET_IMAGE_PATH}" -FilesystemContentsPath "${FILESYSTEM_PATH}"
    WORKING_DIRECTORY "${CURDIR}"
    DEPENDS "${TARGET_PATH_OSLOADER}" "${TARGET_PATH_KERNEL}"
    COMMENT "Creating a VM image..."
)

add_custom_target(Pseudo_VMImage ALL DEPENDS ${TARGET_IMAGE})

# Start our VM.
#add_custom_command(
#    TARGET Pseudo_VMImage
#    POST_BUILD
#    COMMAND "${CURDIR}/QEMU/${QEMU}" -S -gdb tcp:127.0.0.1:7000 -debugcon stdio -L . -smp cpus=4,cores=4 -m size=512m,maxmem=8g --bios "./OVMF/ovmf-x64/OVMF-pure-efi.fd" -net none -hda ${TARGET_IMAGE}
#    WORKING_DIRECTORY "${CURDIR}"
#    COMMENT "Starting VM..."
#    VERBATIM)
