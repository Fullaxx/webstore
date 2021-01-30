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

#ifndef __REDIS_ADVANCED_INTERFACE_H__
#define __REDIS_ADVANCED_INTERFACE_H__

#include <pthread.h>
#include <hiredis/hiredis.h>

typedef struct {
	redisContext *c;
	pthread_mutex_t rl;
	int connected;
} rai_t;

void rai_lock(rai_t *r);
void rai_unlock(rai_t *r);

int rai_connect(rai_t *r, char *dest, unsigned short port);
int rai_check_connection(rai_t *r);
int rai_is_connected(rai_t *r);
void rai_disconnect(rai_t *r);

#endif
