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

//#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
//#include <ctype.h>

#include "webstore_ops.h"
#include "webstore_log.h"

static inline void save_ip(rai_t *rc, char *ip, int found, int reqperiod)
{
	redisReply *reply;

	if(found == 0) {
		reply = redisCommand(rc->c, "SET IPS:%s 1 EX %d", ip, reqperiod);
	} else {
		reply = redisCommand(rc->c, "INCR IPS:%s", ip);
	}
	if(!reply) { handle_redis_error(rc->c); return; }
	freeReplyObject(reply);
}

static int check_ip(wsrt_t *lrt, char *ip)
{
	int count, retval = 1;
	redisReply *reply;
	rai_t *rc = &lrt->rc;

	reply = redisCommand(rc->c, "GET IPS:%s", ip);
	if(!reply) { handle_redis_error(rc->c); return 0; }
	if(reply->type == REDIS_REPLY_NIL) {
		// KEY DOES NOT EXIST
		log_add(WSLOG_INFO, "%s new connection allowed (count: 1)", ip);
		save_ip(rc, ip, 0, lrt->reqperiod);
	} else if(reply->type == REDIS_REPLY_STRING) {
		count = atoi(reply->str);
		if(count < lrt->reqcount) {
			log_add(WSLOG_INFO, "%s new connection allowed (count: %d)", ip, count+1);
			save_ip(rc, ip, 1, lrt->reqperiod);
		} else {
			log_add(WSLOG_INFO, "%s new connection denied (count: %d)", ip, count+1);
			retval = 0;
		}
	} else { retval = 0; }	// THIS SHOULD NEVER HAPPEN

	freeReplyObject(reply);
	return retval;
}

// Use redis to handle rate liminting for inbound connections per IP address
// Return 1 if connection is allowed
// Return 0 if connection is denied
int allow_ip(wsrt_t *lrt, char *ip)
{
	int retval;

	//LOCK RAI if redis calls are in their own thread, using a shared context
	if(lrt->multithreaded) { rai_lock(&lrt->rc); }
	retval = check_ip(lrt, ip);
	if(lrt->multithreaded) { rai_unlock(&lrt->rc); }

	return retval;
}
