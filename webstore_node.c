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

#if 0
	if(rc && !rai_is_connected(rc)) {
		srci_set_return_code(ri, MHD_HTTP_SERVICE_UNAVAILABLE);
		return strdup("Could not connect to HeartBeat DB");
	}

#endif

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

static char* get(wsreq_t *req, wsrt_t *rt, srci_t *ri)
{
	char *hash;
	char *page = NULL;
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
	if(reply->type == REDIS_REPLY_STRING) {
		page = strdup(reply->str);
	}
	freeReplyObject(reply);
	if(rt->multithreaded) { rai_unlock(rc); }
	free(hash);

	if(!page) {
		srci_set_return_code(ri, MHD_HTTP_NOT_FOUND);
		return strdup("not found");
	}

	srci_set_return_code(ri, MHD_HTTP_OK);
	return page;
}

static int do_redis_post(wsrt_t *rt, const char *hash, const unsigned char *dataptr, size_t datalen)
{
	int err = 1;
	char *datastr;
	redisReply *reply;
	rai_t *rc = &rt->rc;

	//Turn our uploaded data into a string with a finalizing NULL
	datastr = calloc(1, datalen+1);
	memcpy(datastr, dataptr, datalen);

	//LOCK RAI if redis calls are in their own thread, using a shared context
	if(rt->multithreaded) { rai_lock(rc); }
	reply = redisCommand(rc->c, "SET %s %s", hash, datastr);
	if(reply->type == REDIS_REPLY_STATUS) {
		if(strncmp("OK", reply->str, 2) == 0) { err = 0; }
	}
	freeReplyObject(reply);
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
		return strdup("malformed request - bad url");
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
		return strdup("malformed request - data did not validate");
	}

#ifdef DEBUG
	printf("%lu) %s\n", datalen, dataptr);
#endif

	hash = convert_hash(req->url, req->urllen);
	if(!hash) {
		srci_set_return_code(ri, MHD_HTTP_BAD_REQUEST);
		return strdup("malformed request - bad url");
	}

	z = do_redis_post(rt, hash, dataptr, datalen);
	free(hash);
	if(z) {
		srci_set_return_code(ri, MHD_HTTP_INTERNAL_SERVER_ERROR);
		return strdup("internal server error");
	}

	srci_set_return_code(ri, MHD_HTTP_OK);
	return strdup("ok");
	//free(hash);
	//return hash;
}

char* md5_node(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	req.type = HASHMD5;
	req.hashlen = HASHMD5LEN;
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
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* sha1_node(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	req.type = HASHSHA1;
	req.hashlen = HASHSHA1LEN;
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
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* sha224_node(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	req.type = HASHSHA224;
	req.hashlen = HASHSHA224LEN;
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
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* sha256_node(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	req.type = HASHSHA256;
	req.hashlen = HASHSHA256LEN;
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
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* sha384_node(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	req.type = HASHSHA384;
	req.hashlen = HASHSHA384LEN;
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
			page = strdup("method not allowed");
			break;
	}

	return page;
}

char* sha512_node(char *url, int urllen, srci_t *ri, void *sri_user_data, void *node_user_data)
{
	wsreq_t req;
	wsrt_t *rt;
	char *page = NULL;

	req.type = HASHSHA512;
	req.hashlen = HASHSHA512LEN;
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
			page = strdup("method not allowed");
			break;
	}

	return page;
}
