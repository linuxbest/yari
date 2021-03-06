/* src/vm/jit/sparc64/emit.h - sparc64 code emitter functions

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   $Id: emitfuncs.c 4398 2006-01-31 23:43:08Z twisti $

*/


#ifndef _EMIT_H
#define _EMIT_H

#include "config.h"
#include "vm/types.h"

/* SPARC branch options */

#define BRANCH_OPT_XCC      0x1
#define BRANCH_OPT_PREDICT  0x2
#define BRANCH_OPT_ANNULL   0x4

#define BRANCH_CHECKS_XCC(options) \
	((options) & BRANCH_OPT_XCC)
	
#define BRANCH_WITH_PREDICT(options) \
	((options) & BRANCH_OPT_PREDICT)
	
#define BRANCH_WITH_ANNULL(options) \
	((options) & BRANCH_OPT_ANNULL)



/* wrappers for branches on 64-bit condition codes */

void emit_beq_xcc(codegendata *cd, basicblock *target);
void emit_bne_xcc(codegendata *cd, basicblock *target);
void emit_blt_xcc(codegendata *cd, basicblock *target);
void emit_bge_xcc(codegendata *cd, basicblock *target);
void emit_bgt_xcc(codegendata *cd, basicblock *target);
void emit_ble_xcc(codegendata *cd, basicblock *target);


#endif /* _EMIT_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
