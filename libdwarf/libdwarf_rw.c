/*-
 * Copyright (c) 2007 John Birrell (jb@freebsd.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "_libdwarf.h"

uint64_t
read_lsb(Elf_Data **dp, uint64_t *offsetp, int bytes_to_read)
{
	uint64_t ret = 0;

	uint8_t *src = (uint8_t *) (*dp)->d_buf + *offsetp;

	switch (bytes_to_read) {
	case 8:
		ret |= ((uint64_t) src[4]) << 32 | ((uint64_t) src[5]) << 40;
		ret |= ((uint64_t) src[6]) << 48 | ((uint64_t) src[7]) << 56;
	case 4:
		ret |= ((uint64_t) src[2]) << 16 | ((uint64_t) src[3]) << 24;
	case 2:
		ret |= ((uint64_t) src[1]) << 8;
	case 1:
		ret |= src[0];
		break;
	default:
		return 0;
		break;
	}

	*offsetp += bytes_to_read;

	return ret;
}

uint64_t
decode_lsb(uint8_t **dp, int bytes_to_read)
{
	uint64_t ret = 0;

	uint8_t *src = *dp;

	switch (bytes_to_read) {
	case 8:
		ret |= ((uint64_t) src[4]) << 32 | ((uint64_t) src[5]) << 40;
		ret |= ((uint64_t) src[6]) << 48 | ((uint64_t) src[7]) << 56;
	case 4:
		ret |= ((uint64_t) src[2]) << 16 | ((uint64_t) src[3]) << 24;
	case 2:
		ret |= ((uint64_t) src[1]) << 8;
	case 1:
		ret |= src[0];
		break;
	default:
		return 0;
		break;
	}

	*dp += bytes_to_read;

	return ret;
}

uint64_t
read_msb(Elf_Data **dp, uint64_t *offsetp, int bytes_to_read)
{
	uint64_t ret = 0;

	uint8_t *src = (uint8_t *) (*dp)->d_buf + *offsetp;

	switch (bytes_to_read) {
	case 1:
		ret = src[0];
		break;
	case 2:
		ret = src[1] | ((uint64_t) src[0]) << 8;
		break;
	case 4:
		ret = src[3] | ((uint64_t) src[2]) << 8;
		ret |= ((uint64_t) src[1]) << 16 | ((uint64_t) src[0]) << 24;
		break;
	case 8:
		ret = src[7] | ((uint64_t) src[6]) << 8;
		ret |= ((uint64_t) src[5]) << 16 | ((uint64_t) src[4]) << 24;
		ret |= ((uint64_t) src[3]) << 32 | ((uint64_t) src[2]) << 40;
		ret |= ((uint64_t) src[1]) << 48 | ((uint64_t) src[0]) << 56;
		break;
	default:
		return 0;
		break;
	}

	*offsetp += bytes_to_read;

	return ret;
}

uint64_t
decode_msb(uint8_t **dp, int bytes_to_read)
{
	uint64_t ret = 0;

	uint8_t *src = *dp;

	switch (bytes_to_read) {
	case 1:
		ret = src[0];
		break;
	case 2:
		ret = src[1] | ((uint64_t) src[0]) << 8;
		break;
	case 4:
		ret = src[3] | ((uint64_t) src[2]) << 8;
		ret |= ((uint64_t) src[1]) << 16 | ((uint64_t) src[0]) << 24;
		break;
	case 8:
		ret = src[7] | ((uint64_t) src[6]) << 8;
		ret |= ((uint64_t) src[5]) << 16 | ((uint64_t) src[4]) << 24;
		ret |= ((uint64_t) src[3]) << 32 | ((uint64_t) src[2]) << 40;
		ret |= ((uint64_t) src[1]) << 48 | ((uint64_t) src[0]) << 56;
		break;
	default:
		return 0;
		break;
	}

	*dp += bytes_to_read;

	return ret;
}

void
write_lsb(Elf_Data **dp, uint64_t *offsetp, uint64_t value, int bytes_to_write)
{
	uint8_t *dst = (uint8_t *) (*dp)->d_buf + *offsetp;

	switch (bytes_to_write) {
	case 8:
		dst[7] = (value >> 56) & 0xff;
		dst[6] = (value >> 48) & 0xff;
		dst[5] = (value >> 40) & 0xff;
		dst[4] = (value >> 32) & 0xff;
	case 4:
		dst[3] = (value >> 24) & 0xff;
		dst[2] = (value >> 16) & 0xff;
	case 2:
		dst[1] = (value >> 8) & 0xff;
	case 1:
		dst[0] = value & 0xff;
		break;
	default:
		return;
		break;
	}

	*offsetp += bytes_to_write;
}

void
write_msb(Elf_Data **dp, uint64_t *offsetp, uint64_t value, int bytes_to_write)
{
	uint8_t *dst = (uint8_t *) (*dp)->d_buf + *offsetp;

	switch (bytes_to_write) {
	case 8:
		dst[7] = value & 0xff;
		dst[6] = (value >> 8) & 0xff;
		dst[5] = (value >> 16) & 0xff;
		dst[4] = (value >> 24) & 0xff;
		value >>= 32;
	case 4:
		dst[3] = value & 0xff;
		dst[2] = (value >> 8) & 0xff;
		value >>= 16;
	case 2:
		dst[1] = value & 0xff;
		value >>= 8;
	case 1:
		dst[0] = value & 0xff;
		break;
	default:
		return;
		break;
	}

	*offsetp += bytes_to_write;
}

int64_t
read_sleb128(Elf_Data **dp, uint64_t *offsetp)
{
	int64_t ret = 0;
	uint8_t b;
	int shift = 0;

	uint8_t *src = (uint8_t *) (*dp)->d_buf + *offsetp;

	do {
		b = *src++;

		ret |= ((b & 0x7f) << shift);

		(*offsetp)++;

		shift += 7;
	} while ((b & 0x80) != 0);

	if (shift < 32 && (b & 0x40) != 0)
		ret |= (-1 << shift);

	return ret;
}

uint64_t
read_uleb128(Elf_Data **dp, uint64_t *offsetp)
{
	uint64_t ret = 0;
	uint8_t b;
	int shift = 0;

	uint8_t *src = (uint8_t *) (*dp)->d_buf + *offsetp;

	do {
		b = *src++;

		ret |= ((b & 0x7f) << shift);

		(*offsetp)++;

		shift += 7;
	} while ((b & 0x80) != 0);

	return ret;
}

int64_t
decode_sleb128(uint8_t **dp)
{
	int64_t ret = 0;
	uint8_t b;
	int shift = 0;

	uint8_t *src = *dp;

	do {
		b = *src++;

		ret |= ((b & 0x7f) << shift);

		shift += 7;
	} while ((b & 0x80) != 0);

	if (shift < 32 && (b & 0x40) != 0)
		ret |= (-1 << shift);

	*dp = src;

	return ret;
}

uint64_t
decode_uleb128(uint8_t **dp)
{
	uint64_t ret = 0;
	uint8_t b;
	int shift = 0;

	uint8_t *src = *dp;

	do {
		b = *src++;

		ret |= ((b & 0x7f) << shift);

		shift += 7;
	} while ((b & 0x80) != 0);

	*dp = src;

	return ret;
}

char *
read_string(Elf_Data **dp, uint64_t *offsetp)
{
	char *ret;

	char *src = (char *) (*dp)->d_buf + *offsetp;

	ret = src;

	while (*src != '\0' && *offsetp < (*dp)->d_size) {
		src++;
		(*offsetp)++;
	}

	if (*src == '\0' && *offsetp < (*dp)->d_size)
		(*offsetp)++;

	return ret;
}

uint8_t *
read_block(Elf_Data **dp, uint64_t *offsetp, uint64_t length)
{
	uint8_t *ret;

	uint8_t *src = (char *) (*dp)->d_buf + *offsetp;

	ret = src;

	(*offsetp) += length;

	return ret;
}
