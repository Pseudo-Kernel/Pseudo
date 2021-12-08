
#pragma once

#include <ke/lock.h>

typedef struct _KTHREAD             KTHREAD;
typedef struct _MMXAD_TREE          MMXAD_TREE;

#define PROCESS_NAME_MAX_LENGTH     128

typedef struct _KPROCESS
{
    KSPIN_LOCK Lock;

    DLIST_ENTRY ProcessList;
    DLIST_ENTRY ThreadList;

    U64 ReferenceCount;

    //
    // Address space.
    //

    MMXAD_TREE *KernelVads;                 // VAD tree for shared kernel addresses.
    MMXAD_TREE *UserVads;                   // VAD tree for user addresses.
    PVOID TopLevelPageTablePhysicalBase;    // Physical address of PML4T base.
    PVOID TopLevelPageTableVirtualBase;     // Virtual address of PML4T base.

    //
    // Etc.
    //

    U64 ProcessId;
    CHAR Name[PROCESS_NAME_MAX_LENGTH];
} KPROCESS;


extern DLIST_ENTRY KiProcessListHead;
extern KSPIN_LOCK KiProcessListLock;

extern KPROCESS KiIdleProcess;
extern KPROCESS KiSystemProcess;



VOID
KiCreateInitialProcessThreads(
    VOID);

KPROCESS *
KeGetCurrentProcess(
    VOID);

VOID
KiInitializeProcess(
    IN KPROCESS *Process,
    IN U64 ProcessId,
    IN CHAR *ProcessName);

KPROCESS *
KiAllocateProcess(
    IN U64 ProcessId,
    IN CHAR *ProcessName);

ESTATUS
KiInsertProcess(
    IN KPROCESS *Process);

KPROCESS *
KiCreateProcess(
    IN CHAR *ProcessName);

