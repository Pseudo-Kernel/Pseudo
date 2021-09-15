
# Require a minimum version of CMake.
cmake_minimum_required(VERSION 3.0.0)

# Set our project name and version.
project(
    Kernel
    VERSION 0.1.0
    LANGUAGES C)

# Our output file name.
set(TARGET_NAME core)           # Output file name
set(TARGET_EXTENSION .sys)      # Output file extension

# Kernel macro definitions.
set(EMULATOR_BUILD 0)   # 0 or 1. 0 for real hardware, 1 for emulator.

# Kernel image base.
set(KERNEL_IMAGE_BASE 0xffff8f0000000000)

# List our source files.
set(SRC_LIST
    # arch
    core/arch/arch.h
    core/arch/x64/intrin64.h
    core/arch/x64/types.h
    # base
    core/base/base.h
    core/base/compiler.h
    core/base/firmware.h
    core/base/macros.h
    core/base/osloader.h
    core/base/stdarg.h
    core/base/stddef.h
    core/base/stdio.h
    # dbg
    core/dbg/dbg.h
    core/dbg/dbg.c
    # init
    core/init/bootgfx.h
    core/init/preinit.h
    core/init/zip.h
    core/init/init.h
    core/init/bootgfx.c
    core/init/preinit.c
    core/init/zipread.c
    core/init/init.c
    # ke
    core/ke/ke.h
    core/ke/lock.h
    core/ke/lock.c
    # misc
    core/misc/common.h
    core/misc/list.h
    core/misc/misc.h
    core/misc/bintree.h
    core/misc/common.c
    core/misc/list.c
    core/misc/bintree.c
    # mm
    core/mm/mminit.h
    core/mm/pool.h
    core/mm/xadtree.h
    core/mm/mm.h
    core/mm/paging.h
    core/mm/mminit.c
    core/mm/pool.c
    core/mm/xadtree.c
    core/mm/mm.c
    core/mm/paging.c
    # root
    core/main.c
    core/stdio.c
)

# Add include directory. (./core/)
include_directories(${CMAKE_CURRENT_LIST_DIR}/core)

# Our executable.
add_executable(${PROJECT_NAME} ${SRC_LIST})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${TARGET_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${TARGET_EXTENSION})

# Compile options.
target_compile_options(
    ${PROJECT_NAME} BEFORE
    PRIVATE -O3 -save-temps -DKERNEL_BUILD_TARGET_EMULATOR=${EMULATOR_BUILD} -Wall -masm=intel -fstack-check=no -nostdinc -nodefaultlibs -nostdlib -ffreestanding -lgcc -lmsvcrt -g -pedantic -std=c11)
#    PRIVATE -O0 -save-temps -Wall -masm=intel -fstack-check=no -nostdinc -nodefaultlibs -nostdlib -ffreestanding -lgcc -lmsvcrt -g -pedantic -std=c11)

# Link options.
# Native application (Base=0x400000, Subsystem=Native, EntryPoint=KiKernelStart)
target_link_options(
    ${PROJECT_NAME}
    PRIVATE -Wl,--dll,--shared,--exclude-all-symbols,--image-base,${KERNEL_IMAGE_BASE},--dynamicbase,--subsystem,native,-eKiKernelStart -fstack-check=no -nodefaultlibs -nostdlib)

# Generate assembly listings.
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND objdump -S ${TARGET_NAME}${TARGET_EXTENSION} -M x86-64,intel > ${TARGET_NAME}${TARGET_EXTENSION}_dump.s
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating assembly listing for ${TARGET_NAME}${TARGET_EXTENSION} ..."
    VERBATIM)

