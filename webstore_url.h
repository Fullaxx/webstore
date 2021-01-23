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

#ifndef __WEBSTORE_URL_H__
#define __WEBSTORE_URL_H__

#include "webstore.h"

// Create the URL and autodetect which REST NODE to request based on token
static char* create_url(char *host, unsigned short port, char *token, int secure)
{
	char *hashtag;
	char url[2048];

	switch(strlen(token)) {
		case HASHLEN128: hashtag="128";    break;
		case HASHLEN160: hashtag="160";   break;
		case HASHLEN224: hashtag="224"; break;
		case HASHLEN256: hashtag="256"; break;
		case HASHLEN384: hashtag="384"; break;
		case HASHLEN512: hashtag="512"; break;
		default:
			fprintf(stderr, "Token is incorrect!\n");
			return NULL;
			break;
	}

	if(secure) {
		snprintf(&url[0], sizeof(url), "https://%s:%u/store/%s/%s", host, port, hashtag, token);
	} else {
		snprintf(&url[0], sizeof(url), "http://%s:%u/store/%s/%s", host, port, hashtag, token);
	}
	return strdup(url);
}

#endif
