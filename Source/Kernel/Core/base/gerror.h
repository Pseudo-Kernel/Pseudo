
#pragma once

typedef enum _ESTATUS
{
	E_SUCCESS = 0,

	E_FAILED = (int)0x80000000l,

	E_INVALID_PARAMETER,
	E_LOOKUP_FAILED,
    E_BUFFER_TOO_SMALL,
	E_NOT_ENOUGH_MEMORY,
} ESTATUS;

#define	E_IS_SUCCESS(_err)		((_err) == E_SUCCESS)

