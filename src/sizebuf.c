/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * This file is largely based on (and named after) a set of common reading/
 * writing functions used in Quake engines.  Its purpose is to allow extraction
 * of bytes, words, and dwords in a safe, endian adjusted environment and should
 * probably be used in any situation where checking for buffer overflows
 * manually makes the code a godawful mess.
 *
 * Currently this is only used by the animation decoding.
 *
 * This file is written with the intention of being easily converted into a
 * class capable of throwing exceptions if data is out of range.
 *
 * If an operation fails, subsequent operations will also fail.  The sizebuf
 * is assumed to be in an invalid state.  This COULD be changed pretty easily
 * and in normal Quake IIRC it is.  But our MO is to bail on failure, not
 * figure out what went wrong (making throws perfect).
 */
#include "sizebuf.h"

#include "SDL_endian.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Construct buffer with the passed array and size */
void SZ_Init(sizebuf_t * sz, Uint8 * buf, unsigned int size)
{
	sz->data = buf;
	sz->bufferLen = size;
	sz->bufferPos = 0;
	sz->error = false;
}

/* Check error flags */
bool SZ_Error(sizebuf_t * sz)
{
	return sz->error;
}

/* mimic memset */
void SZ_Memset(sizebuf_t * sz, int value, size_t count)
{
	/* Do bounds checking before writing */
	if (sz->error || sz->bufferPos + count > sz->bufferLen)
	{
		sz->error = true;
		return;
	}

	/* Memset and increment pointer */
	memset(sz->data + sz->bufferPos, value, count);
	sz->bufferPos += count;
}

/* Mimic memcpy. */
void SZ_Memcpy2(sizebuf_t * sz, sizebuf_t * bf, size_t count)
{
	/* State checking */
	if (sz->error || sz->bufferPos + count > sz->bufferLen)
	{
		sz->error = true;
		return;
	}
	if (bf->error || bf->bufferPos + count > bf->bufferLen)
	{
		bf->error = true;
		return;
	}

	/* Memcpy & increment */
	memcpy(sz->data + sz->bufferPos, bf->data + bf->bufferPos, count);
	sz->bufferPos += count;
	bf->bufferPos += count;
}

/* Reposition buffer pointer */
void SZ_Seek(sizebuf_t * sz, long count, int mode)
{
	if (sz->error)
		return;

	unsigned int new_pos;

	switch (mode)
	{
		case SEEK_SET:
			new_pos = (unsigned int)count;
			break;
		case SEEK_CUR:
			new_pos = sz->bufferPos + count;
			break;
		case SEEK_END:
			new_pos = sz->bufferLen - count;
			break;
		default:
			assert(false);
			return;
	}

	/* Check errors */
	if (new_pos > sz->bufferLen)
		sz->error = true;
	else
	{
		sz->bufferPos = new_pos;
		sz->error = false;
	}
}

/* Helper to read a byte from a buffer using word-aligned access to avoid traps */
static inline uint8_t internal_read_byte(const uint8_t *data, uint32_t pos)
{
	uint32_t addr = (uint32_t)(data + pos);
	uint32_t aligned_addr = addr & ~3;
	uint32_t val = *(const volatile uint32_t *)aligned_addr;
	int shift = (addr & 3) * 8;
	return (uint8_t)(val >> shift);
}

/* The code below makes use of trap-free word-aligned loads to improve performance. */
unsigned int MSG_ReadByte(sizebuf_t * sz)
{
	unsigned int ret;

	if (sz->error || sz->bufferPos + 1 > sz->bufferLen)
	{
		sz->error = true;
		return 0;
	}

	ret = internal_read_byte(sz->data, sz->bufferPos);
	sz->bufferPos += 1;

	return ret;
}

unsigned int MSG_ReadWord(sizebuf_t * sz)
{
	unsigned int ret;

	if (sz->error || sz->bufferPos + 2 > sz->bufferLen)
	{
		sz->error = true;
		return 0;
	}

	/* Read two bytes individually using trap-free helper to avoid misaligned or byte traps */
	uint8_t b0 = internal_read_byte(sz->data, sz->bufferPos);
	uint8_t b1 = internal_read_byte(sz->data, sz->bufferPos + 1);
	
	ret = b0 | (b1 << 8);
	sz->bufferPos += 2;

	return ret;
}
