#pragma once

#include <arch/arch.h>
#include <base/compiler.h>
#include <base/macros.h>


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


#include <base/firmware.h>
#include <base/osloader.h>
#include <base/stdio.h>
#include <base/gerror.h>

#include <init/preinit.h>

#include <misc/misc.h>
#include <dbg/dbg.h>



