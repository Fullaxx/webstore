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
#include "z85.h"
#include "webstore.h"
#include "curl_ops.h"

static void parse_args(int argc, char **argv);

char *g_host = NULL;
unsigned short g_port = 0;
char *g_token = NULL;
char *g_filename = NULL;

// Take the server response and save it to file or print to stdout
static void decode_page(respdata_t *rdata)
{
	char *dec;
	size_t bufsize, decbytes;

	bufsize = Z85_decode_with_padding_bound(rdata->page, rdata->bytecount) + 1;
	dec = malloc(bufsize);
	decbytes = Z85_decode_with_padding(rdata->page, dec, rdata->bytecount);
	if(decbytes == 0) { fprintf(stderr, "nothing to decode!\n"); goto bail; }
	dec[decbytes] = 0;	//if STRING and we have enough space to add the NULL

	if(g_filename) {
		FILE *f = NULL;
		f = fopen(g_filename, "w");
		if(f) {
			fwrite(dec, decbytes, 1, f);
			fclose(f);
		} else { fprintf(stderr, "fopen(%s) failed!!\n", g_filename); goto bail; }
	} else {
		printf("%s", dec);
	}

bail:
	free(dec);
}

// Create the URL and autodetect which REST NODE to request based on token
static char* create_url(char *host, unsigned short port, char *token)
{
	char *hashtag = NULL;
	char url[2048];

	switch(strlen(token)) {
		case HASHMD5LEN:    hashtag="md5";    break;
		case HASHSHA1LEN:   hashtag="sha1";   break;
		case HASHSHA224LEN: hashtag="sha224"; break;
		case HASHSHA256LEN: hashtag="sha256"; break;
		case HASHSHA384LEN: hashtag="sha384"; break;
		case HASHSHA512LEN: hashtag="sha512"; break;
		default: fprintf(stderr, "Token is incorrect!\n"); break;
	}

	if(hashtag) {
		snprintf(&url[0], sizeof(url), "http://%s:%u/store/%s/%s", host, port, hashtag, token);
		return strdup(url);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	int z, retval = 0;
	respdata_t rdata;
	char *url = NULL;

	memset(&rdata, 0, sizeof(respdata_t));
	parse_args(argc, argv);

	url = create_url(g_host, g_port, g_token);
	if(!url) {
		fprintf(stderr, "create_url() failed!\n");
		return 1;
	}

	z = ws_curl_get(url, &rdata);
	if(z) {
		fprintf(stderr, "ws_curl_get() failed!\n");
		retval = 1;
	} else {
		if(rdata.http_code != 200) {
			fprintf(stderr, "HTTP Error %ld: %s\n", rdata.http_code, rdata.page);
			retval = 1;
		} else {
			decode_page(&rdata);
		}
	}

	if(url) free(url);
	if(rdata.page) free(rdata.page);
	if(g_host) free(g_host);
	if(g_token) free(g_token);
	if(g_filename) free(g_filename);
	return retval;
}

struct options opts[] = 
{
	{ 1, "host",	"Host to Connect to",		"H", 1 },
	{ 2, "port",	"Port to Connect to",		"P", 1 },
	{ 3, "token",	"Hash Token to request",	"t", 1 },
	{ 4, "file",	"Save data to file",		"f", 1 },
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
				g_token = strdup(args);
				break;
			case 4:
				g_filename = strdup(args);
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

	if(!g_token) {
		fprintf(stderr, "I need a token to retrieve! (Fix with -t)\n");
		exit(EXIT_FAILURE);
	}
}
