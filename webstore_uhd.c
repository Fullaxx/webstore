/*
	webstore is a web-based arbitrary data storage service that accepts z85 encoded data 
	Copyright (C) 2021 Brett Kuskie <fullaxx@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webstore.h"
#include "webstore_ops.h"
#include "webstore_log.h"
#include "futils.h"

sri_t *g_srv = NULL;
wsrt_t g_rt;
	
#ifdef SRNODECHRONOMETRY
void print_avg_nodecb_time(void)
{
	long avg;

	avg = searest_node_get_avg_duration(ws, "/store/128/");
	if(avg > 0) printf("%6s: %ld\n", "128", avg);

	avg = searest_node_get_avg_duration(ws, "/store/160/");
	if(avg > 0) printf("%6s: %ld\n", "160", avg);

	avg = searest_node_get_avg_duration(ws, "/store/224/");
	if(avg > 0) printf("%6s: %ld\n", "224", avg);

	avg = searest_node_get_avg_duration(ws, "/store/256/");
	if(avg > 0) printf("%6s: %ld\n", "256", avg);

	avg = searest_node_get_avg_duration(ws, "/store/384/");
	if(avg > 0) printf("%6s: %ld\n", "384", avg);

	avg = searest_node_get_avg_duration(ws, "/store/512/");
	if(avg > 0) printf("%6s: %ld\n", "512", avg);
}
#endif

// return SR_IP_DENY to DENY a new incoming connection based on IP address
// return SR_IP_ACCEPT to ACCEPT a new incoming connection based on IP address
static int ws_addr_check(char *inc_ip, void *sri_user_data)
{
	int z;
	wsrt_t *lrt = (wsrt_t *)sri_user_data;

	// Blindly accept w/o logging localhost
	if(strcmp(inc_ip, "127.0.0.1") == 0) { return SR_IP_ACCEPT; }

	z = allow_ip(lrt, inc_ip);
	if(z == 0) { return SR_IP_DENY; }

	return SR_IP_ACCEPT;
}

static void activate_https(srv_opts_t *so)
{
	char *cert, *key;

	cert = get_file(so->certfile);
	if(!cert) { exit(EXIT_FAILURE); }
	key = get_file(so->keyfile);
	if(!key) { exit(EXIT_FAILURE); }
	searest_set_https_cert(g_srv, cert);
	searest_set_https_key(g_srv, key);
	free(cert);
	free(key);
}

void webstore_start(srv_opts_t *so)
{
	int z;

	memset(&g_rt, 0, sizeof(wsrt_t));
	g_rt.multithreaded = so->use_threads;
	z = rai_connect(&g_rt.rc, so->rdest, so->rport);
	if(z) {
		if(so->rport) { fprintf(stderr, "Failed to connect to %s:%u!\n", so->rdest, so->rport); }
		else { fprintf(stderr, "Failed to connect to %s!\n", so->rdest); }
		exit(EXIT_FAILURE);
	}

	// Initialize the server
	g_srv = searest_new(32+11, 128+11, so->max_post_data_size);
	searest_node_add(g_srv, "/store/128/",	&node128, NULL);
	searest_node_add(g_srv, "/store/160/",	&node160, NULL);
	searest_node_add(g_srv, "/store/224/",	&node224, NULL);
	searest_node_add(g_srv, "/store/256/",	&node256, NULL);
	searest_node_add(g_srv, "/store/384/",	&node384, NULL);
	searest_node_add(g_srv, "/store/512/",	&node512, NULL);

	// Configure Multithread
	if(so->use_threads == 0) { searest_set_internal_select(g_srv); }

	// Configure Connection Limiting
	if(getenv("REQPERIOD")) { g_rt.reqperiod = atoi(getenv("REQPERIOD")); }
	if(getenv("REQCOUNT")) { g_rt.reqcount = atol(getenv("REQCOUNT")); }
	if((g_rt.reqperiod > 0) && (g_rt.reqcount > 0)) {
		searest_set_addr_cb(g_srv, &ws_addr_check);
	}

	// Configure Redis Key Expiration
	if(getenv("EXPIRATION")) { g_rt.expiration = atol(getenv("EXPIRATION")); }
	if(g_rt.expiration < 0) { g_rt.expiration = 0; }

	// Configure HTTPS
	if(so->certfile && so->keyfile) { activate_https(so); }

	// Start the server
	z = searest_start(g_srv, so->http_ip, so->http_port, &g_rt);
	if(z) {
		if(!so->http_ip) { so->http_ip="*"; }
		fprintf(stderr, "searest_start() failed to bind to %s:%u!\n", so->http_ip, so->http_port);
		exit(EXIT_FAILURE);
	}

	// Log the success
	log_add(WSLOG_INFO, "webstore started on port %u", so->http_port);
	log_flush();
}

void webstore_stop(void)
{
	if(g_srv) {
		searest_stop(g_srv);
		searest_del(g_srv);
		log_add(WSLOG_INFO, "webstore shutdown");
		rai_disconnect(&g_rt.rc);
	}
}
