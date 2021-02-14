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

#include "getopts.h"
#include "curl_ops.h"

static void parse_args(int argc, char **argv);

char *g_host = NULL;
unsigned short g_port = 0;
int g_secure = 0;

// This must be free()'d
static char* create_url(char *host, unsigned short port, int secure)
{
	char *proto;
	char url[2048];

	if(secure) { proto = "https"; }
	else { proto = "http"; }
	snprintf(&url[0], sizeof(url), "%s://%s:%u/config/all", proto, host, port);
	return strdup(url);
}

int main(int argc, char *argv[])
{
	int z, retval = 0;
	curlresp_t resp;
	char *url = NULL;

	memset(&resp, 0, sizeof(curlresp_t));
	parse_args(argc, argv);

	url = create_url(g_host, g_port, g_secure);
	if(!url) {
		fprintf(stderr, "create_url() failed!\n");
		return 1;
	}

	z = ws_curl_get(url, &resp);
	if(z) {
		fprintf(stderr, "ws_curl_get() failed!\n");
		retval = 1;
	} else {
		if(resp.http_code != 200) {
			fprintf(stderr, "HTTP Error %ld: %s\n", resp.http_code, resp.page);
			retval = 1;
		} else {
			//handle_page(&resp);
			printf("%s", resp.page);
		}
	}

	if(url) { free(url); }
	if(resp.page) { free(resp.page); }
	if(g_host) { free(g_host); }
	return retval;
}

struct options opts[] = 
{
	{ 1, "host",	"Host to Connect to",		"H", 1 },
	{ 2, "port",	"Port to Connect to",		"P", 1 },
	{ 3, "https",	"Use HTTPS",				"s", 0 },
	{ 0, NULL,		NULL,						NULL, 0 }
};

static void parse_args(int argc, char **argv)
{
	char *args;
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
				g_host = strdup(args);
				break;
			case 2:
				g_port = atoi(args);
				break;
			case 3:
				g_secure = 1;
				break;
			default:
				fprintf(stderr, "Unexpected getopts Error! (%d)\n", c);
				break;
		}

		//This free() is required since getopts() automagically allocates space for "args" everytime it's called.
		free(args);
	}

	if(!g_host) {
		fprintf(stderr, "I need a hostname to connect to! (Fix with -H)\n");
		exit(EXIT_FAILURE);
	}

	if(!g_port) {
		fprintf(stderr, "I need a port to connect to! (Fix with -P)\n");
		exit(EXIT_FAILURE);
	}
}
