/*
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

// Compile with:
// -I/usr/include/hiredis rai.c -o binary -lhiredis

/*
#ifdef DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif
*/

#include <string.h>
#include <unistd.h>

#include "rai.h"

void rai_lock(rai_t *r)
{
	pthread_mutex_lock(&r->rl);
}

void rai_unlock(rai_t *r)
{
	pthread_mutex_unlock(&r->rl);
}

// return -1 means you have an open redis handle already (or it appears that way), bail
// return -2 means pthread_mutex_init() failed, bail
// return -3 means redisConnect() failed, bail
// return -4 means there was an connetion error during redisConnect()
int rai_connect(rai_t *r, char *dest, unsigned short port)
{
	int z;
	if(r->connected || r->c) return -1;

	z = pthread_mutex_init(&r->rl, NULL);
	if(z) {
		//fprintf(stderr, "pthread_mutex_init(&r->rl, NULL) failed!\n");
		return -2;
	}

	if(port) r->c = redisConnect(dest, port);
	else	r->c = redisConnectUnix(dest);

	if(!r->c) {
		//fprintf(stderr, "Connection error: can't allocate redis context\n");
		return -3;
	}

	if(r->c->err) {
		//fprintf(stderr, "Connection error: %s\n", c->errstr);
		redisFree(r->c);
		r->c = NULL;
		return -4;
	}

	r->connected = 1;
	return 0;
}

// returns the last known state of our redis handle
int rai_is_connected(rai_t *r)
{
	return r->connected;
}

void rai_disconnect(rai_t *r)
{
	rai_lock(r);

	//Disconnects and frees the context
	if(r->c) { redisFree(r->c); r->c = NULL; }
	r->connected = 0;

	rai_unlock(r);
}
