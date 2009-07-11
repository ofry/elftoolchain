/*-
 * Copyright (c) 2006 Joseph Koshy
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

#include <sys/cdefs.h>
__FBSDID("$Id$");

#include <gelf.h>
#include <libelf.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "tet_api.h"
#include "elfts.h"

IC_REQUIRES_VERSION_INIT();

#define	NO_TESTCASE_FUNCTIONS	/* only want the tables */

#define	TS_XLATESZ	32
#include "xlate_template.c"

#undef	TS_XLATESZ
#define	TS_XLATESZ	64
#include "xlate_template.c"

static int
check_gelf_xlate(Elf_Data *xlator(Elf *e,Elf_Data *d, const Elf_Data *s, unsigned int enc),
    Elf *e, int ed, Elf_Data *dst, Elf_Data *src, struct testdata *td, int ncopies)
{
	Elf_Data *dstret;
	size_t msz;

	msz = td->tsd_msz;

	/* Invoke translator */
	if ((dstret = xlator(e, dst, src, ed)) != dst) {
		tet_printf("fail: \"%s\" " __XSTRING(TC_XLATETOM)
		    ": %s", td->tsd_name, elf_errmsg(-1));
		return (TET_FAIL);
	}

	/* Check return parameters. */
	if (dst->d_type != td->tsd_type || dst->d_size != msz*ncopies) {
		tet_printf("fail: \"%s\" type(ret=%d,expected=%d) "
		    "size (ret=%d,expected=%d).", td->tsd_name,
		    dst->d_type,  td->tsd_type, dst->d_size, msz*ncopies);
		return (TET_FAIL);
	}

	return (TET_PASS);
}

static const char *testfns[] = {
	"xlate.lsb32",
	"xlate.msb32",
	"xlate.lsb64",
	"xlate.msb64",
	NULL
};

static int
tcDriver(int (*tf)(const char *fn, Elf *e))
{
	int fd, result;
	Elf *e;
	const char **fn;

	result = TET_PASS;
	for (fn = testfns; result == TET_PASS && *fn; fn++) {

		_TS_OPEN_FILE(e,*fn,ELF_C_READ,fd,;);

		if (e == NULL) {
			result = TET_UNRESOLVED;
			break;
		}

		result = (*tf)(*fn, e);

		(void) elf_end(e);
		(void) close(fd);
	}

	return (result);
}

/*
 * Check byte conversions:
 */

static int
_tcByte(const char *fn, Elf *e)
{
	Elf_Data dst, src;
	int i, offset, sz, result;
	char *filebuf, *membuf, *t, *ref;

	ref = td_L32_QUAD;
	sz = sizeof(td_L32_QUAD);

	if ((membuf = malloc(sz*NCOPIES)) == NULL ||
	    (filebuf = malloc(sz*NCOPIES+NOFFSET)) == NULL) {
		tet_infoline("unresolved: malloc() failed.");
		return (TET_UNRESOLVED);
	}

	/*
	 * Check memory to file conversions.
	 */
	t = membuf;
	for (i = 0; i < NCOPIES; i++)
		t = memcpy(t,ref,sz) + sz;

	src.d_buf     = membuf;
	src.d_size    = sz*NCOPIES;
	src.d_type    = ELF_T_BYTE;
	src.d_version = EV_CURRENT;

	tet_printf("assertion: \"%s\" Byte TOF() succeeds.", fn);

	for (offset = 0; offset < NOFFSET; offset++) {
		/*
		 * LSB
		 */
		dst.d_buf     = filebuf + offset;
		dst.d_size    = sz*NCOPIES;
		dst.d_version = EV_CURRENT;

		if (gelf_xlatetof(e,&dst,&src,ELFDATA2LSB) != &dst ||
		    dst.d_size != sz*NCOPIES) {
			tet_infoline("fail: LSB TOF() conversion.");
			result = TET_FAIL;
			goto done;
		}

		if (memcmp(membuf, filebuf+offset, sz*NCOPIES)) {
			tet_infoline("fail: LSB TOF() memcmp().");
			result = TET_FAIL;
			goto done;
		}

		/*
		 * MSB
		 */
		dst.d_buf     = filebuf + offset;
		dst.d_size    = sz*NCOPIES;
		dst.d_version = EV_CURRENT;

		if (gelf_xlatetof(e,&dst,&src,ELFDATA2MSB) != &dst ||
		    dst.d_size != sz*NCOPIES) {
			tet_infoline("fail: MSB TOF() conversion.");
			result = TET_FAIL;
			goto done;
		}

		if (memcmp(membuf, filebuf+offset, sz*NCOPIES)) {
			tet_infoline("fail: MSB TOF() memcmp().");
			result = TET_FAIL;
			goto done;
		}
	}

	/*
	 * Check file to memory conversions.
	 */

	tet_printf("assertion: \"%s\" Byte TOM() succeeds.", fn);

	ref = td_M32_QUAD;
	sz = sizeof(td_M32_QUAD);

	for (offset = 0; offset < NOFFSET; offset++) {

		src.d_buf = t = filebuf + offset;
		for (i = 0; i < NCOPIES; i++)
			t = memcpy(t,ref,sz);

		src.d_size    = sz*NCOPIES;
		src.d_type    = ELF_T_BYTE;
		src.d_version = EV_CURRENT;

		/*
		 * LSB
		 */
		dst.d_buf     = membuf;
		dst.d_size    = sz*NCOPIES;
		dst.d_version = EV_CURRENT;

		if (gelf_xlatetom(e,&dst,&src,ELFDATA2LSB) != &dst ||
		    dst.d_size != sz * NCOPIES) {
			tet_infoline("fail: LSB TOM() conversion.");
			result = TET_FAIL;
			goto done;
		}

		if (memcmp(membuf, filebuf+offset, sz*NCOPIES)) {
			tet_infoline("fail: LSB TOM() memcmp().");
			result = TET_FAIL;
			goto done;
		}

		/*
		 * MSB
		 */
		dst.d_buf     = membuf;
		dst.d_size    = sz*NCOPIES;
		dst.d_version = EV_CURRENT;

		if (gelf_xlatetom(e,&dst,&src,ELFDATA2MSB) != &dst ||
		    dst.d_size != sz * NCOPIES) {
			tet_infoline("fail: MSB TOM() conversion.");
			result = TET_FAIL;
			goto done;
		}

		if (memcmp(membuf, filebuf+offset, sz*NCOPIES)) {
			tet_infoline("fail: MSB TOM() memcmp().");
			result = TET_FAIL;
			goto done;
		}
	}

	result = TET_PASS;

 done:
	if (membuf)
		free(membuf);
	if (filebuf)
		free(filebuf);

	return (result);
}

void
tcXlate_tpByte(void)
{
	tet_result(tcDriver(_tcByte));
}

static int
_tpToM(const char *fn, Elf *e)
{
	Elf_Data dst, src;
	struct testdata *td;
	size_t fsz, msz;
	int i, offset, result;
	char *srcbuf, *membuf, *t;

	srcbuf = NULL;	/* file data (bytes) */
	membuf = NULL;	/* memory data (struct) */

	result = TET_PASS;

	td = gelf_getclass(e) == ELFCLASS32 ? tests32  : tests64;

	/* Loop over all types for this ELF class */
	for (; td->tsd_name; td++) {

		fsz = gelf_fsize(e, td->tsd_type, 1, EV_CURRENT);
		msz = td->tsd_msz;

		if (msz == 0 ||
		    fsz != td->tsd_fsz) {
			tet_printf("? \"%s\" %s: msz=%d fsz=%d td->fsz=%d.",
			    fn, td->tsd_name, msz, fsz, td->tsd_fsz);
		}

		assert(fsz == td->tsd_fsz);

		/*
		 * allocate space for NCOPIES of data + offset for file data and
		 * NCOPIES of memory data.
		 */
		if ((srcbuf = malloc(NCOPIES*fsz+NOFFSET)) == NULL ||
		    ((membuf = malloc(NCOPIES*msz))) == NULL) {
			if (srcbuf)
				free(srcbuf);
			tet_infoline("unresolved: malloc() failed.");
			return (TET_UNRESOLVED);
		}


		tet_printf("assertion: \"%s\" gelf_xlatetom(%s) succeeds.", fn,
		    td->tsd_name);

		for (offset = 0; offset < NOFFSET; offset++) {

			src.d_buf     = t = srcbuf + offset;
			src.d_size    = fsz * NCOPIES;
			src.d_type    = td->tsd_type;
			src.d_version = EV_CURRENT;

			dst.d_buf     = membuf;
			dst.d_size    = msz * NCOPIES;
			dst.d_version = EV_CURRENT;


			/*
			 * Check conversion of LSB encoded data.
			 */

			/* copy `NCOPIES*fsz' bytes in `srcbuf+offset' */
			for (i = 0; i < NCOPIES; i++) {
				(void) memcpy(t, td->tsd_lsb, fsz);
				t += fsz;
			}
			(void) memset(membuf, 0, NCOPIES*msz);

			if ((result = check_gelf_xlate(gelf_xlatetom,e,
				 ELFDATA2LSB, &dst,&src,td,NCOPIES)) !=
			    TET_PASS)
				goto done;

			/* compare the retrieved data with the canonical value */
			t = dst.d_buf;
			for (i = 0; i < NCOPIES; i++) {
				if (memcmp(t, td->tsd_mem, msz)) {
					tet_printf("fail: \"%s\" \"%s\" LSB "
					    "memory compare failed.", fn,
					    td->tsd_name);
					result = TET_FAIL;
					goto done;
				}
				t += msz;
			}

			/*
			 * Check conversion of MSB encoded data.
			 */

			t = srcbuf + offset;
			for (i = 0; i < NCOPIES; i++) {
				(void) memcpy(t, td->tsd_msb, fsz);
				t += fsz;
			}
			(void) memset(membuf, 0, NCOPIES*msz);
			if ((result = check_gelf_xlate(gelf_xlatetom,e,
				 ELFDATA2MSB, &dst,&src,td,NCOPIES)) !=
			    TET_PASS)
				goto done;

			/* compare the retrieved data with the canonical value */
			t = dst.d_buf;
			for (i = 0; i < NCOPIES; i++) {
				if (memcmp(t, td->tsd_mem, msz)) {
					tet_printf("fail: \"%s\" \"%s\" MSB "
					    "memory compare failed.", fn,
					    td->tsd_name);
					result = TET_FAIL;
					goto done;
				}
				t += msz;
			}
		}

		free(srcbuf); srcbuf = NULL;
		free(membuf); membuf = NULL;
	}

 done:
	if (srcbuf)
		free(srcbuf);
	if (membuf)
		free(membuf);

	return (result);
}

void
tcXlate_tpToM(void)
{
	tet_result(tcDriver(_tpToM));
}

/*
 * Check non-byte conversions from memory to file.
 */
static int
_tpToF(const char *fn, Elf *e)
{
	Elf_Data dst, src;
	struct testdata *td;
	size_t fsz, msz;
	int i, offset, result;
	char *filebuf, *membuf, *t;

	filebuf = NULL;	/* file data (bytes) */
	membuf = NULL;	/* memory data (struct) */

	td = gelf_getclass(e) == ELFCLASS32 ? tests32 : tests64;

	result = TET_PASS;

	/* Loop over all types */
	for (; td->tsd_name; td++) {

		fsz = gelf_fsize(e, td->tsd_type, 1, EV_CURRENT);
		msz = td->tsd_msz;

		if (msz == 0 ||
		    fsz != td->tsd_fsz) {
			tet_printf("? \"%s\" %s: msz=%d fsz=%d td->fsz=%d.",
			    fn, td->tsd_name, msz, fsz, td->tsd_fsz);
		}

		assert(msz > 0);
		assert(fsz == td->tsd_fsz);

		/*
		 * allocate space for NCOPIES of data + offset for file data and
		 * NCOPIES of memory data.
		 */
		if ((filebuf = malloc(NCOPIES*fsz+NOFFSET)) == NULL ||
		    ((membuf = malloc(NCOPIES*msz))) == NULL) {
			if (filebuf)
				free(filebuf);
			tet_infoline("unresolved: malloc() failed.");
			return (TET_UNRESOLVED);
		}


		tet_printf("assertion: \"%s\" gelf_xlatetof(%s) succeeds.", fn,
		    td->tsd_name);

		for (offset = 0; offset < NOFFSET; offset++) {

			src.d_buf     = membuf;
			src.d_size    = msz * NCOPIES;
			src.d_type    = td->tsd_type;
			src.d_version = EV_CURRENT;

			/*
			 * Check LSB conversion.
			 */

			/* copy `NCOPIES' of canonical memory data to the src buffer */
			t = membuf;
			for (i = 0; i < NCOPIES; i++) {
				(void) memcpy(t, td->tsd_mem, msz);
				t += msz;
			}
			(void) memset(filebuf, 0, NCOPIES*fsz+NOFFSET);

			dst.d_buf     = filebuf + offset;
			dst.d_size    = fsz * NCOPIES;
			dst.d_version = EV_CURRENT;

			if ((result = check_gelf_xlate(gelf_xlatetof, e,
				ELFDATA2LSB, &dst, &src, td, NCOPIES)) !=
			    TET_PASS)
				goto done;

			/* compare converted data to canonical form */
			t = filebuf + offset;
			for (i = 0; i < NCOPIES; i++) {
				if (memcmp(t, td->tsd_lsb, fsz)) {
					tet_printf("fail: \"%s\" \"%s\" LSB memory "
					    "compare.", fn, td->tsd_name);
					result = TET_FAIL;
					goto done;
				}
				t += fsz;
			}

			/*
			 * Check MSB conversion.
			 */
			t = membuf;
			for (i = 0; i < NCOPIES; i++) {
				(void) memcpy(t, td->tsd_mem, msz);
				t += msz;
			}
			(void) memset(filebuf, 0, NCOPIES*fsz+NOFFSET);

			dst.d_buf     = filebuf + offset;
			dst.d_size    = fsz * NCOPIES;
			dst.d_version = EV_CURRENT;

			if ((result = check_gelf_xlate(gelf_xlatetof, e,
				 ELFDATA2MSB, &dst, &src, td, NCOPIES)) ==
			    TET_PASS)
				goto done;

			/* compare converted data to canonical form */
			t = filebuf + offset;
			for (i = 0; i < NCOPIES; i++) {
				if (memcmp(t, td->tsd_msb, fsz)) {
					tet_printf("fail: \"%s\" \"%s\" MSB "
					    "memory compare.", fn, td->tsd_name);
					result = TET_FAIL;
					goto done;
				}
				t += fsz;
			}
		}

		free(filebuf); filebuf = NULL;
		free(membuf); membuf = NULL;
	}

 done:
	if (filebuf)
		free(filebuf);
	if (membuf)
		free(membuf);

	return (result);
}

void
tcXlate_tpToF(void)
{
	tet_result(tcDriver(_tpToF));
}


/*
 * Various checks for invalid arguments.
 */

static int
_tpNullArgs(const char *fn, Elf *e)
{
	Elf_Data ed;
	int result;

	tet_printf("assertion: gelf_xlatetof(%s)/gelf_xlatetom(%s)"
	    " with NULL arguments fails with ELF_E_ARGUMENT.",
	    fn, fn);

	result = TET_PASS;

	if (gelf_xlatetof(NULL, NULL, NULL, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	if (gelf_xlatetof(e, NULL, &ed, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	if (gelf_xlatetof(e, &ed, NULL, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	if (gelf_xlatetom(NULL, NULL, NULL, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	if (gelf_xlatetom(e, NULL, &ed, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	if (gelf_xlatetom(e, &ed, NULL, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	return (result);
}

void
tcArgs_tpNull(void)
{
	tet_result(tcDriver(_tpNullArgs));
}


static int
_tpBadType(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;
	char buf[1024];

	tet_printf("assertion: gelf_xlatetof(%s)/"
	    "gelf_xlatetom(%s) with an out of range type "
	    "fails with ELF_E_DATA.", fn, fn);

	result = TET_PASS;

	es.d_version = ed.d_version = EV_CURRENT;
	es.d_buf     = ed.d_buf = buf;
	es.d_size    = ed.d_size = sizeof(buf);

	es.d_type = (Elf_Type) -1;

	if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	es.d_type = ELF_T_NUM;

	if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	return (result);
}

void
tcArgs_tpBadType(void)
{
	tet_result(tcDriver(_tpBadType));
}

static int
_tpBadEncoding(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;

	tet_infoline("assertion: %s gelf_xlatetof/"
	    "gelf_xlatetom()(*,*,BADENCODING) "
	    "fails with ELF_E_ARGUMENT.");

	result = TET_PASS;

	if (gelf_xlatetof(e, &ed, &es, ELFDATANONE-1) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;
	else if (gelf_xlatetof(e, &ed, &es, ELFDATA2MSB+1) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	if (gelf_xlatetom(e, &ed, &es, ELFDATANONE-1) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;
	else if (gelf_xlatetom(e, &ed, &es, ELFDATA2MSB+1) != NULL ||
	    elf_errno() != ELF_E_ARGUMENT)
		result = TET_FAIL;

	return (result);
}

void
tcArgs_tpBadEncoding(void)
{
	tet_result(tcDriver(_tpBadEncoding));
}

static int
_tpDstSrcVersion(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;
	char buf[sizeof(int)];

	tet_infoline("assertion: gelf_xlateto[fm]() "
	    "with unequal src,dst versions fails with ELF_E_UNIMPL.");

	es.d_buf     = ed.d_buf = buf;
	es.d_type    = ELF_T_BYTE;
	es.d_size    = ed.d_size = sizeof(buf);
	es.d_version = EV_CURRENT;
	ed.d_version = EV_NONE;

	result = TET_PASS;

	if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_UNIMPL)
		result = TET_FAIL;

	if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_UNIMPL)
		result = TET_FAIL;

	return (result);
}

void
tcArgs_tpDstSrcVersion(void)
{
	tet_result(tcDriver(_tpDstSrcVersion));
}

/*
 * Check for an unimplemented type.
 */
static int
_tpUnimplemented(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int i, result;
	char *buf;

	tet_infoline("assertion: gelf_xlateto[fm]() on "
	    "unimplemented types will with ELF_E_UNIMPL.");

	/*
	 * allocate a buffer that is large enough for any potential
	 * ELF data structure.
	 */
	if ((buf = malloc(1024)) == NULL) {
		tet_infoline("unresolved: malloc() failed.");
		return (TET_UNRESOLVED);
	}

	ed.d_buf = es.d_buf = buf;
	ed.d_size = es.d_size = 1024;
	es.d_version = EV_CURRENT;

	result = TET_PASS;

	for (i = 0; i < ELF_T_NUM; i++) {
		switch (i) {
		case ELF_T_CAP:
		case ELF_T_MOVE:
		case ELF_T_MOVEP:
		case ELF_T_NOTE:
		case ELF_T_SYMINFO:
			break;

		case ELF_T_SXWORD:	/* unimplemented for 32 bit ELF */
		case ELF_T_XWORD:
			if (gelf_getclass(e) == ELFCLASS64)
				continue;
			break;
		default:
			continue;
		}

		es.d_type = i;

		if (gelf_xlatetof(e, &ed, &es, ELFDATA2LSB) != NULL ||
		    elf_errno() != ELF_E_UNIMPL) {
			tet_printf("fail: TOF/LSB/type=%d.", i);
			result = TET_FAIL;
		}

		if (gelf_xlatetof(e, &ed, &es, ELFDATA2MSB) != NULL ||
		    elf_errno() != ELF_E_UNIMPL) {
			tet_printf("fail: TOF/MSB/type=%d.", i);
			result = TET_FAIL;
		}

		if (gelf_xlatetom(e, &ed, &es, ELFDATA2LSB) != NULL ||
		    elf_errno() != ELF_E_UNIMPL) {
			tet_printf("fail: TOM/LSB/type=%d.", i);
			result = TET_FAIL;
		}

		if (gelf_xlatetom(e, &ed, &es, ELFDATA2MSB) != NULL ||
		    elf_errno() != ELF_E_UNIMPL) {
			tet_printf("fail: TOM/MSB/type=%d.", i);
			result = TET_FAIL;
		}
	}

	free(buf);
	return (result);
}

void
tcArgs_tpUnimplemented(void)
{
	tet_result(tcDriver(_tpUnimplemented));
}

/*
 * Check for null buffer pointers.
 */
static int
_tpNullDataPtr(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;
	char buf[sizeof(int)];

	tet_printf("assertion: gelf_xlateto[fm](%s) with a null "
	    "src,dst buffer pointer fails with ELF_E_DATA.", fn);

	result = TET_PASS;

	es.d_type    = ELF_T_BYTE;
	es.d_size    = ed.d_size = sizeof(buf);
	es.d_version = EV_CURRENT;
	ed.d_version = EV_CURRENT;

	es.d_buf     = NULL;
	ed.d_buf     = buf;
	if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	es.d_buf     = buf;
	ed.d_buf     = NULL;
	if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA)
		result = TET_FAIL;

	return (result);
}

void
tcBuffer_tpNullDataPtr(void)
{
	tet_result(tcDriver(_tpNullDataPtr));
}

/*
 * Misaligned data.
 */

static int
_tpMisaligned(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;
	size_t fsz, msz;
	char *sb, *db;
	struct testdata *td;

	tet_printf("assertion: \"%s\" misaligned buffers are rejected with "
	    "ELF_E_DATA.", fn);

	sb = db = NULL;
	if ((sb = malloc(1024)) == NULL ||
	    (db = malloc(1024)) == NULL) {
		tet_infoline("unresolved: malloc() failed.");
		if (sb)
			free(sb);
		return (TET_UNRESOLVED);
	}

	result = TET_PASS;

	td = gelf_getclass(e) == ELFCLASS32 ? tests32 : tests64;

	for (; td->tsd_name; td++) {
		fsz = td->tsd_fsz;
		msz = td->tsd_msz;

		es.d_type = td->tsd_type;
		es.d_version = EV_CURRENT;

		/* Misalign the destination for to-memory xfers */
		es.d_size = (1024 / fsz) * fsz;	/* round down */
		es.d_buf  = sb;

		ed.d_buf = db + 1;	/* guaranteed to be misaliged */
		ed.d_version = EV_CURRENT;
		ed.d_size = 1024;

		if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
		    elf_errno() != ELF_E_DATA) {
			tet_printf("fail: \"%s\" TOM alignment.",
			    td->tsd_name);
			result = TET_FAIL;
		}

		/* Misalign the source for to-file xfers */
		es.d_buf = sb + 1;
		es.d_size = (1024/msz) * msz;	/* round down */
		ed.d_buf = db;

		if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
		    elf_errno() != ELF_E_DATA) {
			tet_printf("fail: \"%s\" TOF alignment.",
			    td->tsd_name);
			result = TET_FAIL;
		}
	}

	free(sb);
	free(db);
	return (result);
}

void
tcBuffer_tpMisaligned(void)
{
	tet_result(tcDriver(_tpMisaligned));
}

/*
 * Overlapping buffers.
 */
static int
_tpOverlap(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;
	char buf[sizeof(int)];

	tet_printf("assertion: \"%s\" overlapping buffers are rejected with "
	    "ELF_E_DATA.", fn);

	es.d_buf = buf; 	ed.d_buf = buf+1;
	es.d_version = ed.d_version = EV_CURRENT;
	es.d_size = ed.d_size = sizeof(buf);
	es.d_type = ELF_T_BYTE;

	result = TET_PASS;

	if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA) {
		tet_infoline("fail: gelf_xlatetof().");
		result = TET_FAIL;
	}

	if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
	    elf_errno() != ELF_E_DATA) {
		tet_infoline("fail: gelf_xlatetom().");
		result = TET_FAIL;
	}

	return (result);
}

void
tcBuffer_tpOverlap(void)
{
	tet_result(tcDriver(_tpOverlap));
}

/*
 * Non-integral number of src elements.
 */
static int
_tpSrcExtra(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;
	size_t fsz, msz;
	char *sb, *db;
	struct testdata *td;

	tet_printf("assertion: \"%s\" mis-sized buffers are rejected with "
	    "ELF_E_DATA.", fn);

	sb = db = NULL;
	if ((sb = malloc(1024)) == NULL ||
	    (db = malloc(1024)) == NULL) {
		tet_infoline("unresolved: malloc() failed.");
		if (sb)
			free(sb);
		return (TET_UNRESOLVED);
	}

	result = TET_PASS;

	td = gelf_getclass(e) == ELFCLASS32 ? tests32 : tests64;

	for (; td->tsd_name; td++) {
		fsz = td->tsd_fsz;
		msz = td->tsd_msz;

		es.d_type    = td->tsd_type;
		es.d_version = EV_CURRENT;
		ed.d_version = EV_CURRENT;
		ed.d_buf     = db;
		es.d_buf     = sb;
		ed.d_size    = 1024;

		/* Pad src bytes with extra bytes for to memor */
		es.d_size = fsz+1;

		if (gelf_xlatetom(e, &ed, &es, ELFDATANONE) != NULL ||
		    elf_errno() != ELF_E_DATA) {
			tet_printf("fail: \"%s\" TOM buffer size.",
			    td->tsd_name);
			result = TET_FAIL;
		}

		es.d_size = msz+1;
		if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
		    elf_errno() != ELF_E_DATA) {
			tet_printf("fail: \"%s\" TOF buffer size.",
			    td->tsd_name);
			result = TET_FAIL;
		}
	}

	free(sb);
	free(db);

	return (result);
}

void
tcBuffer_tpSrcExtra(void)
{
	tet_result(tcDriver(_tpSrcExtra));
}

static int
_tpDstTooSmall(const char *fn, Elf *e)
{
	Elf_Data ed, es;
	int result;
	struct testdata *td;
	size_t fsz, msz;
	char buf[1024];

	result = TET_PASS;

	tet_printf("assertion: \"%s\" too small destination buffers are "
	    "rejected with ELF_E_DATA.", fn);

	td = gelf_getclass(e) == ELFCLASS32 ? tests32 : tests64;

	for (; td->tsd_name; td++) {
		msz = td->tsd_msz;
		fsz = td->tsd_fsz;

		es.d_type    = td->tsd_type;
		es.d_version = ed.d_version = EV_CURRENT;
		es.d_buf     = ed.d_buf = buf;

		es.d_size = (sizeof(buf) / msz) * msz;
		ed.d_size  = 1;	/* too small a size */

		if (gelf_xlatetof(e, &ed, &es, ELFDATANONE) != NULL ||
		    elf_errno() != ELF_E_DATA) {
			tet_printf("fail: \"%s\" TOF dst size.",
			    td->tsd_name);
			result = TET_FAIL;
		}

		es.d_size = (sizeof(buf) / fsz) * fsz;
		if (gelf_xlatetom(e, &ed,&es,ELFDATANONE) != NULL ||
		    elf_errno() != ELF_E_DATA) {
			tet_printf("fail: \"%s\" TOF dst size.",
			    td->tsd_name);
			result = TET_FAIL;
		}
	}

	return (result);
}

void
tcBuffer_tpDstTooSmall(void)
{
	tet_result(tcDriver(_tpDstTooSmall));
}

static int
_tpSharedBufferByte(const char *fn, Elf *e)
{
	int i, result;
	size_t sz;
	Elf_Data dst, src;
	char *membuf, *t, *ref;

#define	PREPARE_SHARED(T,SZ)	do {					\
		src.d_buf     = dst.d_buf     = membuf;			\
		src.d_size    = dst.d_size    = (SZ) * NCOPIES;		\
		src.d_type    = dst.d_type    = (T);			\
		src.d_version = dst.d_version = EV_CURRENT;		\
	} while (0)

#define	VERIFY(R,SZ)	do {						\
		t = dst.d_buf;						\
		for (i = 0; i < NCOPIES; i++, t += (SZ))		\
			if (memcmp((R), t, (SZ))) {			\
				tet_infoline("fail: LSB TOF() "		\
				    "memcmp().");			\
				tet_result(TET_FAIL);			\
				goto done;				\
			}						\
	} while (0)

	membuf = NULL;
	ref    = TYPEDEFNAME(L,QUAD);
	sz     = sizeof(TYPEDEFNAME(L,QUAD));

	if ((membuf = malloc(sz * NCOPIES)) == NULL) {
		tet_printf("unresolved: \"%s\" malloc() failed.", fn);
		return (TET_UNRESOLVED);
	}

	result = TET_PASS;

	t = membuf;
	for (i = 0; i < NCOPIES; i++)
		t = memcpy(t, ref, sz) + sz;

	tet_printf("assertion: \"%s\" byte TOF() on a shared dst/src arena "
	    "succeeds.", fn);

	PREPARE_SHARED(ELF_T_BYTE, sz);
	if (gelf_xlatetof(e, &dst, &src, ELFDATA2LSB) != &dst ||
	    dst.d_size != sz * NCOPIES ||
	    dst.d_buf != src.d_buf) {
		tet_printf("fail: \"%s\" LSB TOF() conversion: %s.", fn,
		    elf_errmsg(-1));
		result = TET_FAIL;
		goto done;
	}
	VERIFY(ref,sz);

	PREPARE_SHARED(ELF_T_BYTE, sz);
	if (gelf_xlatetof(e, &dst, &src, ELFDATA2MSB) != &dst ||
	    dst.d_size != sz * NCOPIES ||
	    dst.d_buf != src.d_buf) {
		tet_printf("fail: \"%s\" MSB TOF() conversion: %s.", fn,
		    elf_errmsg(-1));
		result = TET_FAIL;
		goto done;
	}
	VERIFY(ref,sz);

	tet_printf("assertion: \"%s\" byte TOM() on a shared dst/src arena "
	    "succeeds.", fn);

	PREPARE_SHARED(ELF_T_BYTE, sz);
	if (gelf_xlatetom(e, &dst, &src, ELFDATA2LSB) != &dst ||
	    dst.d_size != sz * NCOPIES ||
	    dst.d_buf != src.d_buf) {
		tet_printf("fail: \"%s\" LSB TOM() conversion: %s.", fn,
		    elf_errmsg(-1));
		result = TET_FAIL;
		goto done;
	}
	VERIFY(ref,sz);

	PREPARE_SHARED(ELF_T_BYTE, sz);
	if (gelf_xlatetom(e, &dst, &src, ELFDATA2MSB) != &dst ||
	    dst.d_size != sz * NCOPIES ||
	    dst.d_buf != src.d_buf) {
		tet_printf("fail: \"%s\" MSB TOM() conversion: %s.", fn,
		    elf_errmsg(-1));
		result = TET_FAIL;
		goto done;
	}
	VERIFY(ref,sz);

 done:
	free(membuf);

	return (result);
}


void
tcBuffer_tpSharedBufferByte(void)
{
	tet_result(tcDriver(_tpSharedBufferByte));
}

static int
_tpToMShared(const char *fn, Elf *e)
{
	Elf_Data dst, src;
	struct testdata *td;
	size_t fsz, msz;
	int i, r, result;
	char *membuf, *t;

	membuf = NULL;

	td = gelf_getclass(e) == ELFCLASS32 ? tests32 : tests64;

	r = TET_PASS;

	for (; td->tsd_name; td++) {

		tet_printf("assertion: \"%s\" in-place gelf_xlatetom"
		    "(\"%s\").", fn, td->tsd_name);

		fsz = gelf_fsize(e, td->tsd_type, 1, EV_CURRENT);
		msz = td->tsd_msz;

		assert(msz >= fsz);

		if ((membuf = malloc(fsz * NCOPIES)) == NULL) {
			tet_printf("unresolved: \"%s\" \"%s\" malloc() failed.",
			    fn, td->tsd_name);
			r = TET_UNRESOLVED;
			goto done;
		}

		/*
		 * In-place conversion of LSB data.
		 */

		t = membuf;
		for (i = 0; i < NCOPIES; i++)
			t = memcpy(t, td->tsd_lsb, fsz) + fsz;

		PREPARE_SHARED(td->tsd_type, fsz);
		result = gelf_xlatetom(e, &dst, &src, ELFDATA2LSB) == &dst;

		if (fsz < msz) {
			/* conversion should fail with ELF_E_DATA */
			if (result || elf_errno() != ELF_E_DATA) {
				tet_printf("fail: \"%s\" \"%s\" LSB TOM() succeeded "
				    "with fsz < msz", fn, td->tsd_name);
				r = TET_FAIL;
				goto done;
			}
			free(membuf); membuf = NULL;
			continue;
		}

		/* conversion should have succeeded. */
		if (!result) {
			tet_printf("fail: \"%s\" \"%s\" LSB TOM() failed.",
			    fn, td->tsd_name);
			r = TET_FAIL;
			goto done;
		}

		VERIFY(td->tsd_mem,msz);

		/*
		 * In-place conversion of MSB data.
		 */

		t = membuf;
		for (i = 0; i < NCOPIES; i++)
			t = memcpy(t, td->tsd_msb, fsz) + fsz;

		PREPARE_SHARED(td->tsd_type, fsz);
		result = gelf_xlatetom(e, &dst, &src, ELFDATA2MSB) == &dst;

		if (fsz < msz) {
			/* conversion should fail with ELF_E_DATA */
			if (result || elf_errno() != ELF_E_DATA) {
				tet_printf("fail: \"%s\" \"%s\" MSB TOM() succeeded "
				    "with fsz < msz", fn, td->tsd_name);
				r = TET_FAIL;
				goto done;
			}
			free(membuf); membuf = NULL;
			continue;
		}

		/* conversion should have succeeded. */
		if (!result) {
			tet_printf("fail: \"%s\" \"%s\" MSB TOM() failed.",
			    fn, td->tsd_name);
			r = TET_FAIL;
			goto done;
		}

		VERIFY(td->tsd_mem,msz);

	}

 done:
	if (membuf)
		free(membuf);
	return (r);
}

void
tcXlate_tpToMShared(void)
{
	tet_result(tcDriver(_tpToMShared));
}

static int
_tpToFShared(const char *fn, Elf *e)
{
	Elf_Data dst, src;
	struct testdata *td;
	size_t fsz, msz;
	int i, result;
	char *membuf, *t;

	membuf = NULL;

	td = gelf_getclass(e) == ELFCLASS32 ? tests32 : tests64;
	result = TET_PASS;

	for (; td->tsd_name; td++) {

		tet_printf("assertion: \"%s\" in-place gelf_xlatetof(\"%s\").",
		    fn, td->tsd_name);

		fsz = gelf_fsize(e, td->tsd_type, 1, EV_CURRENT);
		msz = td->tsd_msz;

		assert(msz >= fsz);

		if ((membuf = malloc(msz * NCOPIES)) == NULL) {
			tet_printf("unresolved: \"%s\" \"%s\" malloc() failed.",
			    fn, td->tsd_name);
			result = TET_UNRESOLVED;
			goto done;
		}

		/*
		 * In-place conversion to LSB data.
		 */

		t = membuf;
		for (i = 0; i < NCOPIES; i++)
			t = memcpy(t, td->tsd_mem, msz) + msz;

		PREPARE_SHARED(td->tsd_type, msz);
		if (gelf_xlatetof(e, &dst, &src, ELFDATA2LSB) != &dst) {
			tet_printf("fail: \"%s\" \"%s\" LSB TOF() failed: %s.",
			    fn, td->tsd_name, elf_errmsg(-1));
			result = TET_FAIL;
			goto done;
		}
		VERIFY(td->tsd_lsb,fsz);

		/*
		 * In-place conversion to MSB data.
		 */

		t = membuf;
		for (i = 0; i < NCOPIES; i++)
			t = memcpy(t, td->tsd_mem, msz) + msz;

		PREPARE_SHARED(td->tsd_type, msz);
		if (gelf_xlatetof(e, &dst, &src, ELFDATA2MSB) != &dst) {
			tet_printf("fail: \"%s\" \"%s\" MSB TOF() failed: %s.",
			    fn, td->tsd_name, elf_errmsg(-1));
			result = TET_FAIL;
			goto done;
		}
		VERIFY(td->tsd_msb,fsz);
	}

 done:
	if (membuf)
		free(membuf);
	return (result);
}

void
tcXlate_tpToFShared(void)
{
	tet_result(tcDriver(_tpToFShared));
}
