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
#include <unistd.h>
#include <signal.h>

#include "getopts.h"
#include "webstore_ops.h"
#include "webstore_log.h"

static void parse_args(int argc, char **argv);

int g_redis_error = 0;
int g_shutdown = 0;

char *g_rsock = NULL;
char *g_rip = NULL;
unsigned short g_rport = 0;

srv_opts_t g_so;
char *g_logfile = NULL;

#ifdef SRNODECHRONOMETRY
int alarm_stats = 0;

void alarm_handler(int signum)
{
	print_avg_nodecb_time();

	(void) alarm(1);
}
#endif

int shutting_down(void) { return g_shutdown; }

#include <errno.h>
void handle_redis_error(redisContext *c)
{
	char *etype = NULL;
	switch(c->err) {
		case REDIS_ERR_IO:
			fprintf(stderr, "REDIS_ERR_IO: %s\n", strerror(errno));
			log_add(WSLOG_ERR, "REDIS_ERR_IO: %s", strerror(errno));
			break;
		case REDIS_ERR_EOF:
			etype = "REDIS_ERR_EOF";
			break;
		case REDIS_ERR_PROTOCOL:
			etype = "REDIS_ERR_PROTOCOL";
			break;
		case REDIS_ERR_OOM:
			etype = "REDIS_ERR_OOM";
			break;
		case REDIS_ERR_OTHER:
			etype = "REDIS_ERR_OTHER";
			break;
	}
	if(etype) {
		fprintf(stderr, "%s: %s\n", etype, c->errstr);
		log_add(WSLOG_ERR, "%s: %s", etype, c->errstr);
	}
	g_redis_error = 1;
	g_shutdown = 1;
}

void sig_handler(int signum)
{
	switch(signum) {
		case SIGPIPE:
			fprintf(stderr, "SIGPIPE\n");
			log_add(WSLOG_ERR, "SIGPIPE");
			break;
		case SIGHUP:
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			g_shutdown = 1;
			break;
	}
}

int main(int argc, char *argv[])
{
	int z;

	memset(&g_so, 0, sizeof(srv_opts_t));
	g_so.max_post_data_size = (20*1024*1024);
	parse_args(argc, argv);

	if(g_logfile) {
		z = log_open(g_logfile);
		if(z) {
			fprintf(stderr, "log_open(%s) failed!\n", g_logfile);
			exit(EXIT_FAILURE);
		}
	}

	if(g_rsock) {
		g_so.rdest = g_rsock;
		webstore_start(&g_so);
	} else if(g_rip && (g_rport > 0)) {
		g_so.rdest = g_rip;
		g_so.rport = g_rport;
		webstore_start(&g_so);
	} else {
		fprintf(stderr, "webstore_start() failed!\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT,	sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGQUIT,	sig_handler);
	signal(SIGHUP,	sig_handler);
	signal(SIGPIPE,	sig_handler);
#ifdef SRNODECHRONOMETRY
	if(alarm_stats) {
		signal(SIGALRM, alarm_handler);
		(void) alarm(1);
	}
#endif

	z=0;	// Wait for the sweet release of death
	while(!g_shutdown && !g_redis_error) {
		if(z>999) { z=0; }
		usleep(1000);
		z++;
	}
	usleep(1000);

	// Stop Services
	webstore_stop();
	log_close();

	// Unnecessary Clean Up
	if(g_logfile) { free(g_logfile); }
	if(g_rsock) { free(g_rsock); }
	if(g_rip) { free(g_rip); }
	if(g_so.http_ip) { free(g_so.http_ip); }
	if(g_so.certfile) { free(g_so.certfile); }
	if(g_so.keyfile) { free(g_so.keyfile); }
	return 0;
}

struct options opts[] = 
{
	{ 1, "ip",		"HTTP IP to bind to",			"I",  1 },
	{ 2, "port",	"HTTP Port to bind to",			"P",  1 },
	{ 3, "tpc",		"UHD Multithread",				"t",  0 },
	{ 4, "log",		"Log events to this file",		"l",  1 },
	{ 5, "rsock",	"Connect to Redis file socket",	NULL, 1 },
	{ 6, "rtcp",	"Connect to Redis over tcp",	NULL, 1 },
	{ 7, "cert",	"Use this HTTPS cert",			NULL, 1 },
	{ 8, "key",		"Use this HTTPS key",			NULL, 1 },
	{ 9, "dsize",	"Set max POST data size",		NULL, 1 },
#ifdef SRNODECHRONOMETRY
	{ 10, "stats",	"Show stats every second",		NULL, 0 },
#endif
	{ 0, NULL,		NULL,							NULL, 0 }
};

static void parse_args(int argc, char **argv)
{
	char *args, *colon;
	int c;

	while ((c = getopts(argc, argv, opts, &args)) != 0) {
		switch(c) {
			case -2:
				// Special Case: Recognize options that we didn't set above.
				fprintf(stderr, "Unknown Getopts Option: %s\n", args);
				break;
			case -1:
				// Special Case: getopts() can't allocate memory.
				fprintf(stderr, "Unable to allocate memory for getopts().\n");
				exit(EXIT_FAILURE);
				break;
			case 1:
				g_so.http_ip = strdup(args);
				break;
			case 2:
				g_so.http_port = atoi(args);
				break;
			case 3:
				g_so.use_threads = 1;
				break;
			case 4:
				g_logfile = strdup(args);
				break;
			case 5:
				g_rsock = strdup(args);
				break;
			case 6:
				colon = strchr(args, ':');
				if(colon) {
					*colon = 0;
					g_rip = strdup(args);
					g_rport = atoi(colon+1);
				}
				break;
			case 7:
				g_so.certfile = strdup(args);
				break;
			case 8:
				g_so.keyfile = strdup(args);
				break;
			case 9:
				g_so.max_post_data_size = atol(args);
				break;
#ifdef SRNODECHRONOMETRY
			case 10:
				alarm_stats = 1;
				break;
#endif
			default:
				fprintf(stderr, "Unexpected getopts Error! (%d)\n", c);
				break;
		}

		//This free() is required since getopts() automagically allocates space for "args" everytime it's called.
		free(args);
	}

	if(!g_rsock && !g_rip) {
		fprintf(stderr, "I need to connect to redis! (Fix with --rsock/--rtcp)\n");
		exit(EXIT_FAILURE);
	}

	if(g_rip && !g_rport) {
		fprintf(stderr, "Invalid redis tcp port! (Fix with --rsock IP:PORT)\n");
		exit(EXIT_FAILURE);
	}

	if(!g_so.http_port) {
		fprintf(stderr, "I need a port to listen on! (Fix with -P)\n");
		exit(EXIT_FAILURE);
	}

	if(g_so.certfile && !g_so.keyfile) {
		fprintf(stderr, "I need a key to enable HTTPS operations! (Fix with --key)\n");
		exit(EXIT_FAILURE);
	}

	if(!g_so.certfile && g_so.keyfile) {
		fprintf(stderr, "I need a cert to enable HTTPS operations! (Fix with --cert)\n");
		exit(EXIT_FAILURE);
	}

	if(g_so.max_post_data_size < 6) {
		fprintf(stderr, "POST data size limit is too small! (Fix with --dsize)\n");
		exit(EXIT_FAILURE);
	}
}
