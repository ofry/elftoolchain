/*-
 * Copyright (c) 2009 Kai Wang
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

#include <assert.h>
#include <stdlib.h>
#include "_libdwarf.h"

void
nametbl_cleanup(Dwarf_NameSec ns)
{
	Dwarf_NameTbl nt, tnt;
	Dwarf_NamePair np, tnp;

	assert(ns != NULL);

	STAILQ_FOREACH_SAFE(nt, &ns->ns_ntlist, nt_next, tnt) {
		STAILQ_FOREACH_SAFE(np, &nt->nt_nplist, np_next, tnp) {
			STAILQ_REMOVE(&nt->nt_nplist, np, _Dwarf_NamePair,
			    np_next);
			free(np);
		}
		STAILQ_REMOVE(&ns->ns_ntlist, nt, _Dwarf_NameTbl, nt_next);
		free(nt);
	}

	if (ns->ns_array)
		free(ns->ns_array);

	free(ns);
}

int
nametbl_init(Dwarf_Debug dbg, Dwarf_NameSec *namesec, Elf_Data *d,
    Dwarf_Error *error)
{
	Dwarf_CU cu;
	Dwarf_NameSec ns;
	Dwarf_NameTbl nt;
	Dwarf_NamePair np;
	uint64_t offset, dwarf_size, length, cuoff;
	char *p;
	int i, ret;

	assert(*namesec == NULL);

	if ((ns = malloc(sizeof(struct _Dwarf_NameSec))) == NULL) {
		DWARF_SET_ERROR(error, DWARF_E_MEMORY);
		return (DWARF_E_MEMORY);
	}
	STAILQ_INIT(&ns->ns_ntlist);
	ns->ns_array = NULL;
	ns->ns_len = 0;

	offset = 0;
	while (offset < d->d_size) {

		/* Allocate a new name table. */
		if ((nt = malloc(sizeof(struct _Dwarf_NameTbl))) == NULL) {
			ret = DWARF_E_MEMORY;
			DWARF_SET_ERROR(error, ret);
			goto fail_cleanup;
		}
		STAILQ_INIT(&nt->nt_nplist);
		STAILQ_INSERT_TAIL(&ns->ns_ntlist, nt, nt_next);

		/* Read in the table header. */
		length = dbg->read(&d, &offset, 4);
		if (length == 0xffffffff) {
			dwarf_size = 8;
			length = dbg->read(&d, &offset, 8);
		} else
			dwarf_size = 4;

		nt->nt_length = length;
		/* FIXME: verify version */
		nt->nt_version = dbg->read(&d, &offset, 2);
		nt->nt_cu_offset = dbg->read(&d, &offset, dwarf_size);
		nt->nt_cu_length = dbg->read(&d, &offset, dwarf_size);

		/* Find the referenced CU. */
		STAILQ_FOREACH(cu, &dbg->dbg_cu, cu_next) {
			if (cu->cu_offset == nt->nt_cu_offset)
				break;
		}
		nt->nt_cu = cu;	/* FIXME: Check if NULL here */

		/* Add name pairs. */
		while (offset < d->d_size) {
			cuoff = dbg->read(&d, &offset, dwarf_size);
			if (cuoff == 0)
				break;
			if ((np = malloc(sizeof(struct _Dwarf_NamePair))) ==
			    NULL) {
				ret = DWARF_E_MEMORY;
				DWARF_SET_ERROR(error, ret);
				goto fail_cleanup;
			}
			np->np_nt = nt;
			np->np_offset = cuoff;
			p = (char *) d->d_buf;
			np->np_name = &p[offset];
			while (p[offset++] != '\0')
				;
			STAILQ_INSERT_TAIL(&nt->nt_nplist, np, np_next);
			ns->ns_len++;
		}
	}

	/* Build array of name pairs from all tables. */
	if ((ns->ns_array = malloc(sizeof(Dwarf_NamePair) * ns->ns_len)) ==
	    NULL) {
		ret = DWARF_E_MEMORY;
		DWARF_SET_ERROR(error, ret);
		goto fail_cleanup;
	}

	i = 0;
	STAILQ_FOREACH(nt, &ns->ns_ntlist, nt_next) {
		STAILQ_FOREACH(np, &nt->nt_nplist, np_next)
			ns->ns_array[i++] = np;
	}
	
	assert((Dwarf_Unsigned)i == ns->ns_len);

	*namesec = ns;

	return (DWARF_E_NONE);

fail_cleanup:

	nametbl_cleanup(ns);

	return (ret);
}
