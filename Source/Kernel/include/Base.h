#pragma once

#include <Arch/Arch.h>
#include <Compiler.h>
#include <Macros.h>


//#include <x86intrin.h> // Part of GCC


//
// Base types.
//


typedef	U64					PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef	U64					VIRTUAL_ADDRESS, *PVIRTUAL_ADDRESS;

typedef	struct _GUID {
	U32 v1;
	U16 v2;
	U16 v3;
	U8 v4[8];
} GUID, *PGUID;


#include <Firmware.h>
#include <OsLoader.h>
#include <stdio.h>
#include <init/preinit.h>

#include <Misc/misc.h>
#include <dbg/dbg.h>
#include <Ke/ke.h>


