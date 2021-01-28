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

#include <sys/types.h>	//off_t
#include <sys/stat.h>	//stat() S_ISREG()

#include "getopts.h"
#include "futils.h"
#include "z85.h"
#include "webstore.h"
#include "webstore_url.h"
#include "curl_ops.h"
#include "gchash.h"

static void parse_args(int argc, char **argv);

char *g_host = NULL;
unsigned short g_port = 0;
int g_alg = 0;
char *g_filename = NULL;
char *g_message = NULL;
char *g_token = NULL;
int g_verbosity = 1;
long g_msglen = 0;
int g_secure = 0;

static int post_msg(char *url, char *z85block)
{
	int curlerr, httperr = 0;
	curlresp_t resp;
	curlpost_t post;

	// Prepare our data structures
	memset(&resp, 0, sizeof(curlresp_t));
	post.data = (unsigned char *)z85block;
	post.size = strlen(z85block);

	if(g_verbosity >= 2) { printf("Uploading: %ld bytes\n", post.size); }

	// Submit our z85 message to webstore
	curlerr = ws_curl_post(url, &resp, &post);
	if(curlerr) {
		fprintf(stderr, "ws_curl_get() failed!\n");
	} else {
		if(resp.http_code != 200) {
			httperr = 1;
			if((resp.http_code == 304) && (!resp.page)) {
				// I'm not sure why this doesnt come through properly
				resp.page = strdup("object immutable - not modified");
			}
			fprintf(stderr, "HTTP Error %ld: %s\n", resp.http_code, resp.page);
		}
	}

	// Clean up and check for error conditions
	if(resp.page) { free(resp.page); }
	if(curlerr || httperr) { return 1; }

	return 0;
}

// This must be free()'d
static char* encode_msg(char *msg, size_t len)
{
	char *z85block;
	size_t bufsize, encbytes;

	// Encode our data into a z85 message block
	bufsize = Z85_encode_with_padding_bound(len) + 1;
	z85block = malloc(bufsize);
	encbytes = Z85_encode_with_padding(msg, z85block, len);
	if(!encbytes) { free(z85block); return NULL; }
	z85block[encbytes] = 0;
	return z85block;
}

// This must be free()'d
static char* encode_msg_and_post(char *host, unsigned short port, int alg, char *msg, size_t len)
{
	int err;
	char *z85block;
	char *token;
	char *url;

	z85block = encode_msg(msg, len);
	if(!z85block) { fprintf(stderr, "encode_msg() failed!"); return NULL; }

	// Create the Token and use it to create the URL
	if(g_token) { token = strdup(g_token); }
	else { token = create_token(alg, msg, len); }
	if(!token) { fprintf(stderr, "create_token() failed!"); return NULL; }

	url = create_url(host, port, token, g_secure);
	if(!url) {
		fprintf(stderr, "create_url(%s, %u, %s) failed!\n", host, port, token);
		free(token); token = NULL;
	}

	if(url) {
		err = post_msg(url, z85block);
		if(err) { free(token); token = NULL; }
	}

	if(z85block) { free(z85block); }
	return token;
}

int main(int argc, char *argv[])
{
	int z, retval = 0;
	char *token;

	z = ws_gcinit(WEBSTORE_GCRYPT_MINVERS);
	if(z != 0) { return 1; }
	parse_args(argc, argv);

	token = encode_msg_and_post(g_host, g_port, g_alg, g_message, g_msglen);
	if(token) {
		if(g_verbosity >= 1) { printf("Token: %s\n", token); }
		free(token);
	} else {
		retval = 1;
	}

	if(g_host)		{ free(g_host); }
	if(g_filename)	{ free(g_filename); }
	if(g_message)	{ free(g_message); }
	if(g_token)		{ free(g_token); }
	return retval;
}

static int valid_token(char *token)
{
	switch(strlen(token)) {
		case HASHLEN128: return 1; break;
		case HASHLEN160: return 1; break;
		case HASHLEN224: return 1; break;
		case HASHLEN256: return 1; break;
		case HASHLEN384: return 1; break;
		case HASHLEN512: return 1; break;
	}

	return 0;
}

struct options opts[] = 
{
	{ 1, "host",	"Host to Connect to",			"H", 1 },
	{ 2, "port",	"Port to Connect to",			"P", 1 },
	{ 3, "alg",		"Algorithm to use (1 - 6)",		"a", 1 },
	{ 4, "file",	"File to encode and post",		"f", 1 },
	{ 5, "msg",		"Message to encode and post",	"m", 1 },
	{ 6, "token",	"Use this token when posting",	"t", 1 },
	{ 7, "https",	"Use HTTPS",					"s", 0 },
	{ 8, "quiet",	"Decrease verbosity",			"q", 0 },
	{ 9, "verbose",	"Increase verbosity",			"v", 0 },
	{ 0, NULL,		NULL,							NULL, 0 }
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
				g_alg = atoi(args);
				break;
			case 4:
				g_filename = strdup(args);
				break;
			case 5:
				g_message = strdup(args);
				break;
			case 6:
				g_token = strdup(args);
				break;
			case 7:
				g_secure = 1;
				break;
			case 8:
				g_verbosity = 0;
				break;
			case 9:
				g_verbosity = 2;
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

	if(g_token) {
		if(!valid_token(g_token)) {
			fprintf(stderr, "Invalid Token!\n");
			exit(EXIT_FAILURE);
		}
		if(g_alg) {
			fprintf(stderr, "Choose 1 of (-a/-t)!\n");
			exit(EXIT_FAILURE);
		}
	} else {
		if((g_alg < HASHALG128) || (g_alg > HASHALG512)) {
			fprintf(stderr, "Please choose to an algorithm (Fix with -a)\n");
			exit(EXIT_FAILURE);
		}
	}

	if(g_filename && g_message) {
		fprintf(stderr, "Please choose to post a file or message, not both (Fix with -f or -m)\n");
		exit(EXIT_FAILURE);
	}

	if(!g_filename && !g_message) {
		fprintf(stderr, "I need a file or message to post! (Fix with -f/-m)\n");
		exit(EXIT_FAILURE);
	}

	if(g_filename) {
		g_msglen = file_size(g_filename, 1);
		if(g_msglen == 0) { fprintf(stderr, "%s is empty!\n", g_filename); }
		if(g_msglen <= 0) { exit(EXIT_FAILURE); }
		g_message = get_file(g_filename);
		if(!g_message) { exit(EXIT_FAILURE); }
	}

	if(g_msglen == 0) { g_msglen = strlen(g_message); }
	if(g_msglen < 1) {
		fprintf(stderr, "Content must not be empty!\n");
		exit(EXIT_FAILURE);
	}
}
