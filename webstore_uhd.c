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

static inline void save_ip(rai_t *rc, char *ip, int found)
{
	redisReply *reply;
	if(found == 0) {
		reply = redisCommand(rc->c, "SET IPS:%s 1 EX %d", ip, REQPERIOD);
	} else {
		reply = redisCommand(rc->c, "INCR IPS:%s", ip);
	}
	if(!reply) { handle_redis_error(rc->c); return; }
	freeReplyObject(reply);
}

#if 0
static int check_ip(rai_t *rc, char *ip)
{
	int count, retval = 1;
	redisReply *reply;

	reply = redisCommand(rc->c, "GET IPS:%s", ip);
	if(!reply) { handle_redis_error(rc->c); return 0; }
	if(reply->type == REDIS_REPLY_NIL) {
		// KEY DOES NOT EXIST
		log_add(WSLOG_INFO, "%s new connection allowed (count: 1)", ip);
		save_ip(rc, ip, 0);
	} else if(reply->type == REDIS_REPLY_STRING) {
		count = atoi(reply->str);
		if(count < REQCOUNT) {
			log_add(WSLOG_INFO, "%s new connection allowed (count: %d)", ip, count+1);
			save_ip(rc, ip, 1);
		} else {
			log_add(WSLOG_INFO, "%s new connection denied (count: %d)", ip, count+1);
			retval = 0;
		}
	} else { retval = 0; }	// THIS SHOULD NEVER HAPPEN

	freeReplyObject(reply);
	return retval;
}

static int allow_ip(wsrt_t *lrt, char *ip)
{
	int retval;
	rai_t *rc = &lrt->rc;

	//LOCK RAI if redis calls are in their own thread, using a shared context
	if(lrt->multithreaded) { rai_lock(rc); }
	retval = check_ip(rc, ip);
	if(lrt->multithreaded) { rai_unlock(rc); }

	return retval;
}

// return SR_IP_DENY to DENY a new incoming connection based on IP address
// return SR_IP_ACCEPT to ACCEPT a new incoming connection based on IP address
static int ws_addr_check(char *inc_ip, void *user_data)
{
	int z;
	wsrt_t *lrt = (wsrt_t *)user_data;

	// Blindly accept w/o logging localhost
	if(strcmp(inc_ip, "127.0.0.1") == 0) { return SR_IP_ACCEPT; }

	z = allow_ip(lrt, inc_ip);
	if(z == 0) { return SR_IP_DENY; }

	return SR_IP_ACCEPT;
}
#endif

static int ws_addr_check(char *inc_ip, void *user_data)
{
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

	g_srv = searest_new(32+11, 128+11, so->max_post_data_size);
	searest_node_add(g_srv, "/store/128/",	&node128, NULL);
	searest_node_add(g_srv, "/store/160/",	&node160, NULL);
	searest_node_add(g_srv, "/store/224/",	&node224, NULL);
	searest_node_add(g_srv, "/store/256/",	&node256, NULL);
	searest_node_add(g_srv, "/store/384/",	&node384, NULL);
	searest_node_add(g_srv, "/store/512/",	&node512, NULL);

	if(so->certfile && so->keyfile) { activate_https(so); }

	searest_set_addr_cb(g_srv, &ws_addr_check);
	if(so->use_threads == 0) searest_set_internal_select(g_srv);
	z = searest_start(g_srv, so->http_ip, so->http_port, &g_rt);
	if(z) {
		if(!so->http_ip) { so->http_ip="*"; }
		fprintf(stderr, "searest_start() failed to bind to %s:%u!\n", so->http_ip, so->http_port);
		exit(EXIT_FAILURE);
	}

	log_add(WSLOG_INFO, "webstore started on port %u", so->http_port);
	log_flush();
}

void webstore_stop(void)
{
	if(g_srv) {
		log_add(WSLOG_INFO, "webstore shutdown");
		searest_stop(g_srv);
		searest_del(g_srv);
		rai_disconnect(&g_rt.rc);
	}
}
