/*
 * Copyright (c) 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2003-2021 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include <sys/param.h>
#include "../bsd/sys/ioctl.h"
#include "../bsd/sys/socket.h"
#include "../bsd/sys/sockio.h"

#include <stdlib.h>
#include <unistd.h>

#include "../bsd/net/ethernet.h"
#include "../bsd/net/if.h"
#include "../bsd/net/if_var.h"
#include "../bsd/net/if_vlan_var.h"
#include "../bsd/net/route.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include "ifconfig.h"

#include <sys/cdefs.h>

#define	NOTAG	((u_short) -1)

static 	struct vlanreq params = {
	.vlr_tag	= NOTAG,
};

static int
getvlan(int s, struct ifreq *ifr, struct vlanreq *vreq)
{
	bzero((char *)vreq, sizeof(*vreq));
	ifr->ifr_data = (caddr_t)vreq;

	return ioctl(s, SIOCGETVLAN, (caddr_t)ifr);
}

static void
vlan_status(int s)
{
	struct vlanreq		vreq;

	if (getvlan(s, &ifr, &vreq) != -1)
		printf("\tvlan: %d parent interface: %s\n",
		    vreq.vlr_tag, vreq.vlr_parent[0] == '\0' ?
		    "<none>" : vreq.vlr_parent);
}

static void
vlan_create(int s, struct ifreq *ifr)
{
	if (params.vlr_tag != NOTAG || params.vlr_parent[0] != '\0') {
		/*
		 * One or both parameters were specified, make sure both.
		 */
		if (params.vlr_tag == NOTAG)
			errx(1, "must specify a tag for vlan create");
		if (params.vlr_parent[0] == '\0')
			errx(1, "must specify a parent device for vlan create");
		ifr->ifr_data = (caddr_t) &params;
	}
#ifdef SIOCIFCREATE2
	if (ioctl(s, SIOCIFCREATE2, ifr) < 0)
		err(1, "SIOCIFCREATE2");
#else
	if (ioctl(s, SIOCIFCREATE, ifr) < 0)
		err(1, "SIOCIFCREATE");
#endif
}

static void
vlan_cb(int s, void *arg)
{
	if ((params.vlr_tag != NOTAG) ^ (params.vlr_parent[0] != '\0'))
		errx(1, "both vlan and vlandev must be specified");
}

static void
vlan_set(int s, struct ifreq *ifr)
{
	if (params.vlr_tag != NOTAG && params.vlr_parent[0] != '\0') {
		ifr->ifr_data = (caddr_t) &params;
		if (ioctl(s, SIOCSETVLAN, (caddr_t)ifr) == -1)
			err(1, "SIOCSETVLAN");
	}
}

static
DECL_CMD_FUNC(setvlantag, val, d)
{
	struct vlanreq vreq;
	u_long ul;
	char *endp;

	ul = strtoul(val, &endp, 0);
	if (*endp != '\0')
		errx(1, "invalid value for vlan");
	params.vlr_tag = ul;
	/* check if the value can be represented in vlr_tag */
	if (params.vlr_tag != ul)
		errx(1, "value for vlan out of range");

	if (getvlan(s, &ifr, &vreq) != -1)
		vlan_set(s, &ifr);
	else
		clone_setcallback(vlan_create);
}

static
DECL_CMD_FUNC(setvlandev, val, d)
{
	struct vlanreq vreq;

	strlcpy(params.vlr_parent, val, sizeof(params.vlr_parent));

	if (getvlan(s, &ifr, &vreq) != -1)
		vlan_set(s, &ifr);
	else
		clone_setcallback(vlan_create);
}

static
DECL_CMD_FUNC(unsetvlandev, val, d)
{
	struct vlanreq		vreq;

	bzero((char *)&vreq, sizeof(struct vlanreq));
	ifr.ifr_data = (caddr_t)&vreq;

	if (ioctl(s, SIOCGETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCGETVLAN");

	bzero((char *)&vreq.vlr_parent, sizeof(vreq.vlr_parent));
	vreq.vlr_tag = 0;

	if (ioctl(s, SIOCSETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCSETVLAN");
}

static struct cmd vlan_cmds[] = {
	DEF_CLONE_CMD_ARG("vlan",			setvlantag),
	DEF_CLONE_CMD_ARG("vlandev",			setvlandev),
	/* XXX For compatibility.  Should become DEF_CMD() some day. */
	DEF_CMD_OPTARG("-vlandev",			unsetvlandev),
#ifdef IFCAP_VLAN_MTU
	DEF_CMD("vlanmtu",	IFCAP_VLAN_MTU,		setifcap),
	DEF_CMD("-vlanmtu",	-IFCAP_VLAN_MTU,	setifcap),
#endif /* IFCAP_VLAN_MTU */
#ifdef IFCAP_VLAN_HWTAGGING
	DEF_CMD("vlanhwtag",	IFCAP_VLAN_HWTAGGING,	setifcap),
	DEF_CMD("-vlanhwtag",	-IFCAP_VLAN_HWTAGGING,	setifcap),
#endif /* IFCAP_VLAN_HWTAGGING */
};

#define VLAN_CLONE_NAME		"vlan"
#define VLAN_CLONE_NAME_LENGTH	(sizeof(VLAN_CLONE_NAME) - 1)

static struct afswtch af_vlan = {
	.af_name	= "af_vlan",
	.af_clone_name  = VLAN_CLONE_NAME,
	.af_clone_name_length = VLAN_CLONE_NAME_LENGTH,
	.af_af		= AF_UNSPEC,
	.af_other_status = vlan_status,
};

static __constructor void
vlan_ctor(void)
{
#define	N(a)	(sizeof(a) / sizeof(a[0]))
	int i;

	for (i = 0; i < N(vlan_cmds);  i++)
		cmd_register(&vlan_cmds[i]);
	af_register(&af_vlan);
	callback_register(vlan_cb, NULL);
#undef N
}
