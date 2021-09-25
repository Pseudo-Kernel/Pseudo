
/**
 * @file osdebug.c
 * @author Pseudo-Kernel (sandbox.isolated@gmail.com)
 * @brief Implements debugging related routines.
 * @version 0.1
 * @date 2021-09-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "OsLoader.h"
#include "osmisc.h"
#include "osdebug.h"


VOID
TracePortE9(
    IN CHAR16 *Format, 
    ...)
{
    CHAR16 Buffer[PRINT_MAX_BUFFER + 1];
    VA_LIST List;
    UINTN Length;

    VA_START(List, Format);
    Length = StrFormatV(Buffer, ARRAY_SIZE(Buffer) - 1, Format, List);
    VA_END(List);

    StrTerminate(Buffer, ARRAY_SIZE(Buffer), Length);

    CHAR16 *s = Buffer;
    while (*s)
    {
        CHAR8 c = *s;
	    __asm__ __volatile__ (
	        "out %0, %1\n\t"
	        :
	        : "n"(0xe9), "a"(c)
    	    : /*"memory"*/
    	);

        s++;
    }
}

/**
 * @brief Prints the formatted string.
 * 
 * @param [in] Format   Printf-like format string.
 * @param [in] ...      Arguments.
 * 
 * @return None.
 */
VOID
Trace(
    IN CHAR16 *Format,
    ...)
{
    CHAR16 Buffer[PRINT_MAX_BUFFER + 1];
    VA_LIST List;
    UINTN Length;

    VA_START(List, Format);
    Length = StrFormatV(Buffer, ARRAY_SIZE(Buffer) - 1, Format, List);
    VA_END(List);

    StrTerminate(Buffer, ARRAY_SIZE(Buffer), Length);
    gConOut->OutputString(gConOut, Buffer);
}


/**
 * @brief Waits for enter key input.
 * 
 * @param [in] LoaderBlock  Loader block.
 * @param [in] Message      Message to be printed.
 * 
 * @return None.
 */
VOID
EFIAPI
OslDbgWaitEnterKey(
    IN OS_LOADER_BLOCK *LoaderBlock, 
    IN CHAR16 *Message)
{
    if(IS_DEBUG_MODE(LoaderBlock))
    {
        if(!Message)
            TRACE(L"DBG: Press Enter Key to Continue\r");
        else
            TRACE(Message);

        OslWaitForKeyInput(0, 0x0d, 0x00ffffffffffffff);

        // 79 chars
        TRACE(L"                                        "
              L"                                       \r");
    }
}

/**
 * @brief Dumps low 1M area to disk.
 * 
 * @param [in] LoaderBlock  Loader block.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslDbgDumpLowMemoryToDisk(
    IN OS_LOADER_BLOCK *LoaderBlock)
{
    EFI_FILE *RootDirectory = LoaderBlock->Base.RootDirectory;
    EFI_FILE *DumpImage = NULL;
    EFI_STATUS Status = EFI_SUCCESS;

    UINTN BufferSize = 0;

    do
    {
        Status = RootDirectory->Open(
            RootDirectory,
            &DumpImage,
            L"Efi\\Boot\\Dump_00000000_000FFFFF.bin",
            EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
            0
        );

        if (Status != EFI_SUCCESS)
        {
            TRACEF(L"Cannot create the dump file (0x%lx)\r\n", (UINT64)Status);
            break;
        }

        //
        // Dump Area : 0x00000000 - 0x000fffff.
        //

        BufferSize = 0x100000;

        Status = DumpImage->Write(DumpImage, &BufferSize, (void *)LoaderBlock->LoaderData.ShadowBase);
        if (Status != EFI_SUCCESS || !BufferSize)
        {
            TRACEF(L"Cannot write to the dump file (0x%lx)\r\n", (UINT64)Status);
            break;
        }

        if (BufferSize != 0x100000)
        {
            TRACEF(L"Dump requested 0x%08x but only written 0x%08x - 0x%08x\r\n",
                0x100000, 0, BufferSize - 1);
        }

        //
        // Flush and close.
        //

        DumpImage->Flush(DumpImage);
        DumpImage->Close(DumpImage);

        return TRUE;

    } while (FALSE);

    return FALSE;
}

/**
 * @brief Fills the screen to color.
 * 
 * @param LoaderBlock       Loader block.
 * @param Color             Color to fill.
 * 
 * @return TRUE if succeeds, FALSE otherwise.
 */
BOOLEAN
EFIAPI
OslDbgFillScreen(
    IN OS_LOADER_BLOCK *LoaderBlock,
    IN UINT32 Color)
{
    volatile UINT32 *Framebuffer = (volatile UINT32 *)LoaderBlock->LoaderData.VideoFramebufferBase;
    UINT32 Count = LoaderBlock->LoaderData.VideoFramebufferSize / sizeof(UINT32);

    if (!Framebuffer)
    {
        return FALSE;
    }

    for (UINT32 i = 0; i < Count; i++)
    {
        Framebuffer[i] = Color;
    }

    return TRUE;
}

