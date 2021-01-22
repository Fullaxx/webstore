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

int g_redis_dissappeared = 0;
int g_shutdown = 0;

char *ip = NULL;
unsigned short http_port = 0;
int g_use_threads = 0;

char *g_logfile = NULL;

char *rsock = NULL;
char *rip = NULL;
unsigned short rport = 0;

char *g_certfile = NULL;
char *g_keyfile = NULL;

#ifdef SRNODECHRONOMETRY
int alarm_stats = 0;

void alarm_handler(int signum)
{
	print_avg_nodecb_time();

	(void) alarm(1);
}
#endif

void sig_handler(int signum)
{
	switch(signum) {
		/*case SIGPIPE:
			fprintf(stderr, "SIGPIPE recvd!\n");
			g_shutdown = 1;*/
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

	parse_args(argc, argv);

	if(rsock) {
		webstore_start(ip, http_port, g_use_threads, rsock, 0, g_certfile, g_keyfile);
	} else if(rip && (rport > 0)) {
		webstore_start(ip, http_port, g_use_threads, rip, rport, g_certfile, g_keyfile);
	} else {
		if(!ip) { ip = "*"; }
		if(!rip) { rip = "*"; }
		if(!g_certfile) { g_certfile = "NULL"; }
		if(!g_keyfile) { g_keyfile = "NULL"; }
		fprintf(stderr, "webstore_start(%s, %u, %d, %s, %u, %s, %s) failed!\n", ip, http_port, g_use_threads, rip, rport, g_certfile, g_keyfile);
		exit(EXIT_FAILURE);
	}

	if(g_logfile) {
		z = log_open(g_logfile);
		if(z) {
			fprintf(stderr, "log_open(%s) failed!\n", g_logfile);
			exit(EXIT_FAILURE);
		}
	}

	signal(SIGINT,	sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGQUIT,	sig_handler);
	signal(SIGHUP,	sig_handler);
#ifdef SRNODECHRONOMETRY
	if(alarm_stats) {
		signal(SIGALRM, alarm_handler);
		(void) alarm(1);
	}
#endif

	z=0;
	while(!g_shutdown && !g_redis_dissappeared) {
		if(z>999) { z=0; }
		usleep(1000);
		z++;
	}

	webstore_stop();
	log_close();
	if(g_logfile) { free(g_logfile); }
	if(rsock) { free(rsock); }
	if(rip) { free(rip); }
	if(ip) { free(ip); }
	if(g_certfile) { free(g_certfile); }
	if(g_keyfile) { free(g_keyfile); }
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
#ifdef SRNODECHRONOMETRY
	{ 9, "stats",	"Show stats on the second",		NULL, 0 },
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
				ip = strdup(args);
				break;
			case 2:
				http_port = atoi(args);
				break;
			case 3:
				g_use_threads = 1;
				break;
			case 4:
				g_logfile = strdup(args);
				break;
			case 5:
				rsock = strdup(args);
				break;
			case 6:
				colon = strchr(args, ':');
				if(colon) {
					*colon = 0;
					rip = strdup(args);
					rport = atoi(colon+1);
				}
				break;
			case 7:
				g_certfile = strdup(args);
				break;
			case 8:
				g_keyfile = strdup(args);
				break;
#ifdef SRNODECHRONOMETRY
			case 9:
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

	if(!rsock && !rip) {
		fprintf(stderr, "I need to connect to redis! (Fix with --rsock/--rtcp)\n");
		exit(EXIT_FAILURE);
	}

	if(rip && !rport) {
		fprintf(stderr, "Invalid redis tcp port! (Fix with --rsock IP:PORT)\n");
		exit(EXIT_FAILURE);
	}

	if(!http_port) {
		fprintf(stderr, "I need a port to listen on! (Fix with -P)\n");
		exit(EXIT_FAILURE);
	}

	if(g_certfile && !g_keyfile) {
		fprintf(stderr, "I need a key to enable HTTPS operations! (Fix with --key)\n");
		exit(EXIT_FAILURE);
	}

	if(!g_certfile && g_keyfile) {
		fprintf(stderr, "I need a cert to enable HTTPS operations! (Fix with --cert)\n");
		exit(EXIT_FAILURE);
	}
}
