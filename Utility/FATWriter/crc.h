
#ifndef _CRC_H_
#define	_CRC_H_


ULONG
APIENTRY
CrcCalculateUpdate(
	IN ULONG Crc, 
	IN PUCHAR Buffer, 
	IN ULONG Length);

ULONG
APIENTRY
CrcMessage(
	IN ULONG Crc, 
	IN PUCHAR Buffer, 
	IN ULONG Length);


#endif

