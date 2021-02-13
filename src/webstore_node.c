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
//#include <unistd.h>
#include <ctype.h>

#include "webstore.h"
#include "webstore_ops.h"
#include "webstore_log.h"

static const char* base85 =
{
   "0123456789"
   "abcdefghij"
   "klmnopqrst"
   "uvwxyzABCD"
   "EFGHIJKLMN"
   "OPQRSTUVWX"
   "YZ.-:+=^!/"
   "*?&<>()[]{"
   "}@%$#"
};

// returns TRUE if the incoming digit is a valid z85 digit
static int is_Z85_digit(char digit)
{
	int i;
	if(isalnum(digit)) return 1;	//TRUE
	for(i=62; i<85; i++) {
		if(digit == base85[i]) return 1;	//TRUE
	}
	return 0;	//FALSE
}

// return FALSE if any character in ptr is not a valid z85 digit
static int Z85_validate(const unsigned char *ptr, size_t len)
{
	size_t i;
	for(i=0; i<len; i++) {
		if(!is_Z85_digit(ptr[i])) return 1;
	}
	return 0;	// did not validate properly
}

// Validate proper hex digits in URL and convert to all lowercase
// return strdup(newhash) if valid
// return NULL if NOT VALID
// this must be free()'d
static char* convert_hash(char *input, int len)
{
	int i,j;
	char newhash[256];

	if(len > sizeof(newhash)-1) {
		fprintf(stderr, "SOMETHING WENT HORRIBLY WRONG!\n");
		exit(1);
	}

	memset(&newhash[0], 0, sizeof(newhash));
	for(i=0,j=0; i<len; i++,j++) {
		if(!isxdigit(input[i])) { return NULL; }
		newhash[j] = tolower(input[i]);
	}
	newhash[len] = 0;

	return strdup(newhash);
}

static inline void do_redis_del(rai_t *rc, char *hash)
{
	redisReply *reply;
	reply = redisCommand(rc->c, "DEL %s", hash);
	freeReplyObject(reply);
}

static char* get(wsreq_t *req, wsrt_t *rt, srci_t *ri)
{
	int err = 0;
	char *hash;
	char *page = NULL;
	char *log_fmt;
	char log_entry[512];
	redisReply *reply;
	rai_t *rc = &rt->rc;

	// Check the URL length
	if(req->urllen != req->hashlen) {
		srci_set_return_code(ri, MHD_HTTP_BAD_REQUEST);
		return strdup("malformed request");
	}

	hash = convert_hash(req->url, req->urllen);
	if(!hash) {
		srci_set_return_code(ri, MHD_HTTP_BAD_REQUEST);
		return strdup("malformed request");
	}

	//LOCK RAI if redis calls are in their own thread, using a shared context
	if(rt->multithreaded) { rai_lock(rc); }
	reply = redisCommand(rc->c, "GET %s", hash);
	if(!reply) {
		err = 503;
		handle_redis_error(rc);
	} else {
		if(reply->type == REDIS_REPLY_STRING) { page = strdup(reply->str); }
		if(page && rt->bar) { do_redis_del(rc, hash); }
		freeReplyObject(reply);
	}
	if(rt->multithreaded) { rai_unlock(rc); }
	free(hash);

	if(err == 503) {
		srci_set_return_code(ri, MHD_HTTP_SERVICE_UNAVAILABLE);
		return strdup("service unavailable");
	}

	if(!page) {
		srci_set_return_code(ri, MHD_HTTP_NOT_FOUND);
		log_add(WSLOG_INFO, "%s %d GET %s", srci_get_client_ip(ri), MHD_HTTP_NOT_FOUND, req->url);
		return strdup("not found");
	}

	srci_set_return_code(ri, MHD_HTTP_OK);
	if(rt->bar) { log_fmt = "%s %d GET %s BURNT"; }
	else { log_fmt = "%s %d GET %s"; }
	snprintf(log_entry, sizeof(log_entry), log_fmt, srci_get_client_ip(ri), MHD_HTTP_OK, req->url);
	log_add(WSLOG_INFO, "%s", log_entry);
	return page;
}

static int do_redis_post(wsrt_t *rt, const char *hash, const unsigned char *dataptr, size_t datalen)
{
	int err = 500;
	char *datastr;
	redisReply *reply;
	rai_t *rc = &rt->rc;

	//Turn our uploaded data into a string with a finalizing NULL
	datastr = calloc(1, datalen+1);
	memcpy(datastr, dataptr, datalen);

	//LOCK RAI if redis calls are in their own thread, using a shared context
	if(rt->multithreaded) { rai_lock(rc); }
	if((rt->expiration) && (rt->immutable)) {
		reply = redisCommand(rc->c, "SET %s %s EX %ld NX", hash, datastr, rt->expiration);
	} else if(rt->expiration) {
		reply = redisCommand(rc->c, "SET %s %s EX %ld", hash, datastr, rt->expiration);
	} else if(rt->immutable) {
		reply = redisCommand(rc->c, "SET %s %s NX", hash, datastr);
	} else {
		reply = redisCommand(rc->c, "SET %s %s", hash, datastr);
	}

	if(!reply) {
		err = 503;
		handle_redis_error(rc);
	} else {
		if(reply->type == REDIS_REPLY_ERROR) { err = 417; }
		if(reply->type == REDIS_REPLY_NIL) { err = 304; }
		if(reply->type == REDIS_REPLY_STATUS) {
			if(strncmp("OK", reply->str, 2) == 0) { err = 0; }
		}
		freeReplyObject(reply);
	}
	if(rt->multithreaded) { rai_unlock(rc); }

	free(datastr);
	return err;
}

static char* post(wsreq_t *req, wsrt_t *rt, srci_t *ri)
{
	int z;
	const unsigned char *dataptr;
	size_t datalen;
	char *hash;

	// Check the URL length
	if(req->urllen != req->hashlen) {
		srci_set_return_code(ri, MHD_HTTP_BAD_REQUEST);
		return strdup("malformed request - invalid url");
	}

	// Check the length of uploaded data
	datalen = srci_get_post_data_size(ri);
	if(datalen < 5) {
		srci_set_return_code(ri, MHD_HTTP_BAD_REQUEST);
		return strdup("malformed request - invalid length");
	}

	// Validate the uploaded data
	dataptr = srci_get_post_data_ptr(ri);
	z = Z85_validate(dataptr, datalen);
	if(z) {
		srci_set_return_code(ri, MHD_HTTP_BAD_REQUEST);
		return strdup("malformed request - invalid Z85");
	}

#ifdef DEBUG
	//printf("%lu) %s\n", datalen, dataptr);
#endif

	hash = convert_hash(req->url, req->urllen);
	if(!hash) {
		srci_set_return_code(ri, MHD_HTTP_BAD_REQUEST);
		return strdup("malformed request - invalid token");
	}

	z = do_redis_post(rt, hash, dataptr, datalen);
	free(hash);
	if(z) {
		srci_set_return_code(ri, z);
		switch(z) {
			case 304:
				log_add(WSLOG_INFO, "%s %d POST %s NOTMOD", srci_get_client_ip(ri), z, req->url);
				return strdup("object immutable - not modified");
				break;
			case 417:
				return strdup("redis reply error");
				break;
			case 503:
				return strdup("service unavailable");
				break;
			default:
				return strdup("internal server error");
		}
	}

	srci_set_return_code(ri, MHD_HTTP_OK);
	log_add(WSLOG_INFO, "%s %d POST %s", srci_get_client_ip(ri), MHD_HTTP_OK, req->url);
	return strdup("ok");
}

static inline char* shutdownmsg(srci_t *ri)
{
	srci_set_return_code(ri, MHD_HTTP_SERVICE_UNAVAILABLE);
	return strdup("service unavailable: shutting down");
}

char* node128(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	if(shutting_down()) { return shutdownmsg(ri); }

	req.type = HASHALG128;
	req.hashlen = HASHLEN128;
	req.url = url;
	req.urllen = urllen;
	rt = (wsrt_t *)sri_user_data;

	switch(METHOD(ri)) {
		case METHOD_GET:
			page = get(&req, rt, ri);
			break;
		case METHOD_POST:
			page = post(&req, rt, ri);
			break;
		/*case METHOD_DEL:
			break;*/
		default:
			srci_set_return_code(ri, MHD_HTTP_METHOD_NOT_ALLOWED);
			log_add(WSLOG_WARN, "%s %d METHOD_NOT_ALLOWED", srci_get_client_ip(ri), ri->return_code);
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* node160(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	if(shutting_down()) { return shutdownmsg(ri); }

	req.type = HASHALG160;
	req.hashlen = HASHLEN160;
	req.url = url;
	req.urllen = urllen;
	rt = (wsrt_t *)sri_user_data;

	switch(METHOD(ri)) {
		case METHOD_GET:
			page = get(&req, rt, ri);
			break;
		case METHOD_POST:
			page = post(&req, rt, ri);
			break;
		/*case METHOD_DEL:
			break;*/
		default:
			srci_set_return_code(ri, MHD_HTTP_METHOD_NOT_ALLOWED);
			log_add(WSLOG_WARN, "%s %d METHOD_NOT_ALLOWED", srci_get_client_ip(ri), ri->return_code);
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* node224(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	if(shutting_down()) { return shutdownmsg(ri); }

	req.type = HASHALG224;
	req.hashlen = HASHLEN224;
	req.url = url;
	req.urllen = urllen;
	rt = (wsrt_t *)sri_user_data;

	switch(METHOD(ri)) {
		case METHOD_GET:
			page = get(&req, rt, ri);
			break;
		case METHOD_POST:
			page = post(&req, rt, ri);
			break;
		/*case METHOD_DEL:
			break;*/
		default:
			srci_set_return_code(ri, MHD_HTTP_METHOD_NOT_ALLOWED);
			log_add(WSLOG_WARN, "%s %d METHOD_NOT_ALLOWED", srci_get_client_ip(ri), ri->return_code);
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* node256(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	if(shutting_down()) { return shutdownmsg(ri); }

	req.type = HASHALG256;
	req.hashlen = HASHLEN256;
	req.url = url;
	req.urllen = urllen;
	rt = (wsrt_t *)sri_user_data;

	switch(METHOD(ri)) {
		case METHOD_GET:
			page = get(&req, rt, ri);
			break;
		case METHOD_POST:
			page = post(&req, rt, ri);
			break;
		/*case METHOD_DEL:
			break;*/
		default:
			srci_set_return_code(ri, MHD_HTTP_METHOD_NOT_ALLOWED);
			log_add(WSLOG_WARN, "%s %d METHOD_NOT_ALLOWED", srci_get_client_ip(ri), ri->return_code);
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* node384(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	if(shutting_down()) { return shutdownmsg(ri); }

	req.type = HASHALG384;
	req.hashlen = HASHLEN384;
	req.url = url;
	req.urllen = urllen;
	rt = (wsrt_t *)sri_user_data;

	switch(METHOD(ri)) {
		case METHOD_GET:
			page = get(&req, rt, ri);
			break;
		case METHOD_POST:
			page = post(&req, rt, ri);
			break;
		/*case METHOD_DEL:
			break;*/
		default:
			srci_set_return_code(ri, MHD_HTTP_METHOD_NOT_ALLOWED);
			log_add(WSLOG_WARN, "%s %d METHOD_NOT_ALLOWED", srci_get_client_ip(ri), ri->return_code);
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* node512(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	if(shutting_down()) { return shutdownmsg(ri); }

	req.type = HASHALG512;
	req.hashlen = HASHLEN512;
	req.url = url;
	req.urllen = urllen;
	rt = (wsrt_t *)sri_user_data;

	switch(METHOD(ri)) {
		case METHOD_GET:
			page = get(&req, rt, ri);
			break;
		case METHOD_POST:
			page = post(&req, rt, ri);
			break;
		/*case METHOD_DEL:
			break;*/
		default:
			srci_set_return_code(ri, MHD_HTTP_METHOD_NOT_ALLOWED);
			log_add(WSLOG_WARN, "%s %d METHOD_NOT_ALLOWED", srci_get_client_ip(ri), ri->return_code);
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* nodecfg(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsrt_t *rt;
	int maxuploadsize = 0;
	int expiration = 0;
	int immutable = 0;
	int bar = 0;
	int n = 0;
	char page[128*4];

	if(shutting_down()) { return shutdownmsg(ri); }
	rt = (wsrt_t *)sri_user_data;

	if(METHOD(ri) != METHOD_GET) {
		srci_set_return_code(ri, MHD_HTTP_METHOD_NOT_ALLOWED);
		log_add(WSLOG_WARN, "%s %d METHOD_NOT_ALLOWED", srci_get_client_ip(ri), ri->return_code);
		return strdup("method not allowed");
	}

	memset(page, 0, sizeof(page));
	if(strcmp(url, "all") == 0) {
		maxuploadsize = 1;
		expiration = 1;
		immutable = 1;
		bar = 1;
	} else {
		if(strcmp(url, "maxuploadsize") == 0) { maxuploadsize = 1; }
		if(strcmp(url, "expiration") == 0) { expiration = 1; }
		if(strcmp(url, "immutable") == 0) { immutable = 1; }
		if(strcmp(url, "bar") == 0) { bar = 1; }
	}

	if(maxuploadsize + expiration + immutable + bar == 0) {
		srci_set_return_code(ri, MHD_HTTP_NOT_FOUND);
		snprintf(page, sizeof(page), "Unknown Config Option: %s\n", url);
		return strdup(page);
	}

	if(maxuploadsize) {
		n += snprintf(page+n, sizeof(page)-n, "MAXUPLOADSIZE: %ld bytes\n", rt->max_post_data_size);
	}

	if(expiration) {
		n += snprintf(page+n, sizeof(page)-n, "EXPIRATION: %ld seconds\n", rt->expiration);
	}

	if(immutable) {
		n += snprintf(page+n, sizeof(page)-n, "IMMUTABLE: %d\n", rt->immutable);
	}

	if(bar) {
		n += snprintf(page+n, sizeof(page)-n, "BAR: %d\n", rt->bar);
	}

	srci_set_return_code(ri, MHD_HTTP_OK);
	return strdup(page);
}
