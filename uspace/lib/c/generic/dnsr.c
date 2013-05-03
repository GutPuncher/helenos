/*
 * Copyright (c) 2013 Jiri Svoboda
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

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <inet/dnsr.h>
#include <ipc/dnsr.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static FIBRIL_MUTEX_INITIALIZE(dnsr_sess_mutex);

static async_sess_t *dnsr_sess = NULL;

static async_exch_t *dnsr_exchange_begin(void)
{
	async_sess_t *sess;
	service_id_t dnsr_svc;

	fibril_mutex_lock(&dnsr_sess_mutex);

	if (dnsr_sess == NULL) {
		(void) loc_service_get_id(SERVICE_NAME_DNSR, &dnsr_svc,
		    IPC_FLAG_BLOCKING);

		dnsr_sess = loc_service_connect(EXCHANGE_SERIALIZE, dnsr_svc,
		    IPC_FLAG_BLOCKING);
	}

	sess = dnsr_sess;
	fibril_mutex_unlock(&dnsr_sess_mutex);

	return async_exchange_begin(sess);
}

static void dnsr_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

int dnsr_name2host(const char *name, dnsr_hostinfo_t **rinfo)
{
	async_exch_t *exch = dnsr_exchange_begin();
	dnsr_hostinfo_t *info;

	ipc_call_t answer;
	aid_t req = async_send_0(exch, DNSR_NAME2HOST, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));

	dnsr_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		return EIO;

	info = calloc(1, sizeof(dnsr_hostinfo_t));
	if (info == NULL)
		return ENOMEM;

	info->name = str_dup(name);
	info->addr.ipv4 = IPC_GET_ARG1(answer);

	*rinfo = info;
	return EOK;
}

void dnsr_hostinfo_destroy(dnsr_hostinfo_t *info)
{
	if (info == NULL)
		return;

	free(info->name);
	free(info);
}

int dnsr_get_srvaddr(inet_addr_t *srvaddr)
{
	sysarg_t addr;
	async_exch_t *exch = dnsr_exchange_begin();

	int rc = async_req_0_1(exch, DNSR_GET_SRVADDR, &addr);
	dnsr_exchange_end(exch);

	if (rc != EOK)
		return rc;

	srvaddr->ipv4 = addr;
	return EOK;
}

int dnsr_set_srvaddr(inet_addr_t *srvaddr)
{
	async_exch_t *exch = dnsr_exchange_begin();

	int rc = async_req_1_0(exch, DNSR_SET_SRVADDR, srvaddr->ipv4);
	dnsr_exchange_end(exch);

	if (rc != EOK)
		return rc;

	return EOK;
}

/** @}
 */