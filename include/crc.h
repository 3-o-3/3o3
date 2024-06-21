/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *                             check sums
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/

/*
 * https://github.com/aeldidi/crc32/blob/master/src/crc32.c
 * https://github.com/novatel/checksum/blob/master/C/checksumDemo.c
 */

#ifndef CRC_H_
#define CRC_H_

#include "os3.h"

/**
 * compute the CRC32 of a buffer
 * \param data buffer
 * \param len size of buffer
 * \returns the computed CRC32 value
 */
static os_uint32 crypt__crc32(const os_utf8 *data, os_size len)
{
	os_uint32 crc = 0;
	os_uint32 v;
	os_intn i, j;
	for (i = 0; i < len; i++)
	{
		v = (crc ^ data[i]) & 0xff;
		for (j = 0; j < 8; j++)
		{
			if (v & 0x1)
			{
				v = (v >> 1) ^ 0xEDB88320; /* CRC32 POLYNOMINAL */
			}
			else
			{
				v = v >> 1;
			}
		}
		crc = ((crc >> 8) & 0xFFFFFF) ^ v;
	}
	return crc;
}
#endif