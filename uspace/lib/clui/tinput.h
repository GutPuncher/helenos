/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup libclui
 * @{
 */
/**
 * @file
 */

#ifndef LIBCLUI_TINPUT_H_
#define LIBCLUI_TINPUT_H_

#include <ipc/ipc.h>

#define HISTORY_LEN     10
#define INPUT_MAX_SIZE  1024

/** Text input field (command line).
 *
 * Applications should treat this structure as opaque.
 */
typedef struct {
	/** Buffer holding text currently being edited */
	wchar_t buffer[INPUT_MAX_SIZE + 1];
	
	/** Screen coordinates of the top-left corner of the text field */
	sysarg_t col0;
	sysarg_t row0;
	
	/** Screen dimensions */
	sysarg_t con_cols;
	sysarg_t con_rows;
	
	/** Number of characters in @c buffer */
	size_t nc;
	
	/** Caret position within buffer */
	size_t pos;
	
	/** Selection mark position within buffer */
	size_t sel_start;
	
	/** History (dynamically allocated strings) */
	char *history[HISTORY_LEN + 1];
	
	/** Number of entries in @c history, not counting [0] */
	size_t hnum;
	
	/** Current position in history */
	size_t hpos;
	
	/** @c true if finished with this line (return to caller) */
	bool done;
	
	/** @c true if user requested to abort interactive loop */
	bool exit_clui;
} tinput_t;

extern tinput_t *tinput_new(void);
extern void tinput_destroy(tinput_t *);
extern int tinput_read(tinput_t *, char **);

#endif

/** @}
 */
