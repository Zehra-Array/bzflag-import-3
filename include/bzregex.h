/*	$NetBSD: regex.h,v 1.4 1994/10/26 00:56:14 cgd Exp $	*/

/*-
 * Copyright (c) 1992 Henry Spencer.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Henry Spencer of the University of Toronto.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)regex.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _REGEX_H_
#define	_REGEX_H_

/* From #include <sys/cdefs.h> */
#ifndef __BEGIN_DECLS
#  ifdef __cplusplus
#    define __BEGIN_DECLS   extern "C" {
#    define __END_DECLS     }
#  else
#    define __BEGIN_DECLS
#    define __END_DECLS
#  endif
#endif

#include <sys/types.h>

/* types */

/* ugly hack to make non-MS windows compilers work */
#if defined(_WIN32) && !defined(_MSC_VER) && !defined(_OFF_T_)
#define _OFF_T_
typedef long off_t;
#endif

typedef off_t regoff_t;

struct regex_t {
	int re_magic;
	size_t re_nsub;		/* number of parenthesized subexpressions */
	const char *re_endp;	/* end pointer for REG_PEND */
	struct re_guts *re_g;	/* none of your business :-) */
};
typedef struct regex_t regex_t;

struct regmatch_t {
	regoff_t rm_so;		/* start of match */
	regoff_t rm_eo;		/* end of match */
};
typedef struct regmatch_t regmatch_t;

/* regcomp() flags */
#define	REG_BASIC	0000
#define	REG_EXTENDED	0001
#define	REG_ICASE	0002
#define	REG_NOSUB	0004
#define	REG_NEWLINE	0010
#define	REG_NOSPEC	0020
#define	REG_PEND	0040
#define	REG_DUMP	0200

/* regerror() flags */
#define	REG_NOMATCH	 1
#define	REG_BADPAT	 2
#define	REG_ECOLLATE	 3
#define	REG_ECTYPE	 4
#define	REG_EESCAPE	 5
#define	REG_ESUBREG	 6
#define	REG_EBRACK	 7
#define	REG_EPAREN	 8
#define	REG_EBRACE	 9
#define	REG_BADBR	10
#define	REG_ERANGE	11
#define	REG_ESPACE	12
#define	REG_BADRPT	13
#define	REG_EMPTY	14
#define	REG_ASSERT	15
#define	REG_INVARG	16
#define	REG_ATOI	255	/* convert name to number (!) */
#define	REG_ITOA	0400	/* convert number to name (!) */

/* regexec() flags */
#define	REG_NOTBOL	00001
#define	REG_NOTEOL	00002
#define	REG_STARTEND	00004
#define	REG_TRACE	00400	/* tracing of execution */
#define	REG_LARGE	01000	/* force large representation */
#define	REG_BACKR	02000	/* force use of backref code */

__BEGIN_DECLS
int	regcomp (regex_t *, const char *, int);
size_t	regerror (int, const regex_t *, char *, size_t);
int	regexec (const regex_t *, const char *, size_t, regmatch_t [], int);
void	regfree (regex_t *);
__END_DECLS

#endif /* !_REGEX_H_ */
