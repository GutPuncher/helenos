/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_FIBRIL_H_
#define LIBC_FIBRIL_H_

#include <libarch/fibril.h>
#include <libadt/list.h>
#include <libarch/tls.h>

#ifndef context_set
#define context_set(c, _pc, stack, size, ptls) 			\
	(c)->pc = (sysarg_t) (_pc);				\
	(c)->sp = ((sysarg_t) (stack)) + (size) - SP_DELTA; 	\
        (c)->tls = (sysarg_t) (ptls);
#endif /* context_set */

#define FIBRIL_SERIALIZED   1

typedef enum {
	FIBRIL_PREEMPT,
	FIBRIL_TO_MANAGER,
	FIBRIL_FROM_MANAGER,
	FIBRIL_FROM_DEAD
} fibril_switch_type_t;

typedef sysarg_t fid_t;

struct fibril {
	link_t link;
	context_t ctx;
	void *stack;
	void *arg;
	int (*func)(void *);
	tcb_t *tcb;

	struct fibril *clean_after_me;
	int retval;
	int flags;
};
typedef struct fibril fibril_t;

extern int context_save(context_t *c);
extern void context_restore(context_t *c) __attribute__ ((noreturn));

extern fid_t fibril_create(int (*func)(void *), void *arg);
extern fibril_t *fibril_setup(void);
extern void fibril_teardown(fibril_t *f);
extern int fibril_switch(fibril_switch_type_t stype);
extern void fibril_add_ready(fid_t fid);
extern void fibril_add_manager(fid_t fid);
extern void fibril_remove_manager(void);
extern fid_t fibril_get_id(void);
extern void fibril_inc_sercount(void);
extern void fibril_dec_sercount(void);

static inline int fibril_yield(void) {
	return fibril_switch(FIBRIL_PREEMPT);
}

#endif

/** @}
 */
