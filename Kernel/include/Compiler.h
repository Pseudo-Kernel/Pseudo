#pragma once

//
// Macros for compiler keywords.
//

#define	STATIC							static
#define	VOLATILE						volatile
#define	CONST							const
#define	INLINE							__inline

#define	CDECL							__cdecl
#define	STDCALL							__stdcall
#define	VECTORCALL						__vectorcall

#define	EXPORT							__declspec(dllexport)
#define	IMPORT							__declspec(dllimport)

//
// Alignments.
//

#define	COMPILER_PACK(_x)				__declspec(align(_x))
#define	STRUCT_PACK						COMPILER_PACK
#define	UNION_PACK						COMPILER_PACK


