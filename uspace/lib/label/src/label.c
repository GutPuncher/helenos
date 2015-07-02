/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup liblabel
 * @{
 */
/**
 * @file Disk label library.
 */

#include <adt/list.h>
#include <errno.h>
#include <label.h>
#include <mem.h>
#include <stdlib.h>

#include "gpt.h"

static label_ops_t *probe_list[] = {
	&gpt_label_ops,
	NULL
};

int label_open(service_id_t sid, label_t **rlabel)
{
	label_ops_t **ops;
	int rc;

	ops = &probe_list[0];
	while (ops[0] != NULL) {
		rc = ops[0]->open(sid, rlabel);
		if (rc == EOK)
			return EOK;
		++ops;
	}

	return ENOTSUP;
}

int label_create(service_id_t sid, label_type_t ltype, label_t **rlabel)
{
	label_ops_t *ops = NULL;

	switch (ltype) {
	case lt_gpt:
		ops = &gpt_label_ops;
		break;
	case lt_mbr:
		ops = NULL;
		break;
	}

	if (ops == NULL)
		return ENOTSUP;

	return ops->create(sid, rlabel);
}

void label_close(label_t *label)
{
	if (label == NULL)
		return;

	label->ops->close(label);
}

int label_destroy(label_t *label)
{
	return label->ops->destroy(label);
}

int label_get_info(label_t *label, label_info_t *linfo)
{
	return label->ops->get_info(label, linfo);
}

label_part_t *label_part_first(label_t *label)
{
	return label->ops->part_first(label);
}

label_part_t *label_part_next(label_part_t *part)
{
	return part->label->ops->part_next(part);
}

void label_part_get_info(label_part_t *part, label_part_info_t *pinfo)
{
	return part->label->ops->part_get_info(part, pinfo);
}

int label_part_create(label_t *label, label_part_spec_t *pspec,
    label_part_t **rpart)
{
	return label->ops->part_create(label, pspec, rpart);
}

int label_part_destroy(label_part_t *part)
{
	return part->label->ops->part_destroy(part);
}

void label_pspec_init(label_part_spec_t *pspec)
{
	memset(pspec, 0, sizeof(label_part_spec_t));
}

/** @}
 */
