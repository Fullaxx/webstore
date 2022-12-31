/*
	SeaRest is a RESTFul service framework leveraging libmicrohttpd
	Copyright (C) 2022 Brett Kuskie <fullaxx@gmail.com>

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

// https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html
// https://www.gnu.org/software/libmicrohttpd/manual/html_node/
// https://www.gnu.org/software/libmicrohttpd/tutorial.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "searest.h"

#ifdef SRNODECHRONOMETRY
#include "chronometry.h"
void searest_node_save_time(srn_t *n, long duration);
#endif

size_t searest_node_len(srn_t *n);
void searest_node_set_access(srn_t *n);
int searest_node_is_disabled(srn_t *n);
void searest_node_destroy_all(sri_t *ws);
srn_t* searest_node_find(sri_t *ws, char *rootname);

char* srci_get_client_ip(srci_t *ri)
{
	return ri->ip;
}

char* srci_get_browser_auth(srci_t *ri)
{
	return ri->auth;
}

int srci_browser_requests_text(srci_t *ri)
{
	if(!ri->accept) { return 0; }
	if(strcasecmp(ri->accept, MIMETYPETXTPLAINSTR) == 0) { return 1; }
	return 0;
}

int srci_browser_requests_xml(srci_t *ri)
{
	if(!ri->accept) { return 0; }
	if(strcasecmp(ri->accept, MIMETYPEAPPXMLSTR) == 0) { return 1; }
	return 0;
}

int srci_browser_requests_json(srci_t *ri)
{
	if(!ri->accept) { return 0; }
	if(strcasecmp(ri->accept, MIMETYPEAPPJSONSTR) == 0) { return 1; }
	return 0;
}

int srci_get_method_type(srci_t *ri)
{
	return ri->method_type;
}

void srci_set_response_content_type(srci_t *ri, char *ct)
{
	if(ri->content_type) { free(ri->content_type); }
	ri->content_type = strdup(ct);
}

void srci_set_response_allow(srci_t *ri, char *a)
{
	if(ri->allow) { free(ri->allow); }
	ri->allow = strdup(a);
}

void srci_set_response_cors(srci_t *ri)
{
	// https://daveceddia.com/access-control-allow-origin-cors-errors-in-angular/
	// https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/OPTIONS
	ri->cors = 1;
}

const unsigned char* srci_get_post_data_ptr(srci_t *ri)
{
	return ri->post_data;
}

size_t srci_get_post_data_size(srci_t *ri)
{
	return ri->post_data_len;
}

void srci_set_return_code(srci_t *ri, int code)
{
	ri->return_code = code;
}

// THIS MUST BE FREE()'d -- and it does get free()'d in uhd_request_completed()
static char* client_ip_str (struct MHD_Connection *connection)
{
	const union MHD_ConnectionInfo *ci;
	struct sockaddr_in *in;
	char ip_str[128];

	ci = MHD_get_connection_info (connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
	if(!ci) { return NULL; }

	in = (struct sockaddr_in *)ci->client_addr;
	if(!in) { return NULL; }

	switch(in->sin_family) {
		case AF_INET:
			inet_ntop(AF_INET, &in->sin_addr, &ip_str[0], INET_ADDRSTRLEN);
			break;
		case AF_INET6:
			inet_ntop(AF_INET6, &in->sin_addr, &ip_str[0], INET6_ADDRSTRLEN);
			break;
		default:
			return NULL;
	}

	return strdup(ip_str);
}

static char* process_request(sri_t *ws, srci_t *ri, void *sri_user_data)
{
	srn_t *n;
	size_t nlen;
#ifdef SRNODECHRONOMETRY
	stopwatch_t sw;
#endif

	n = searest_node_find(ws, ri->url);
	if(!n) {
		ri->return_code = MHD_HTTP_NOT_FOUND;
		ri->return_page = strdup("node not found");
	} else if(searest_node_is_disabled(n)){
		ri->return_code = MHD_HTTP_SERVICE_UNAVAILABLE;
		ri->return_page = strdup("node not enabled");
	} else {
		nlen = searest_node_len(n);
#ifdef SRNODECHRONOMETRY
		chron_start(&sw, -1);
#endif
		ri->return_page = n->cb(ri->url+nlen, ri->urllen-nlen, ri, sri_user_data, n->nud);
#ifdef SRNODECHRONOMETRY
		searest_node_save_time(n, chron_stop(&sw));
#endif
		searest_node_set_access(n);
	}

	return ri->return_page;
}

/* https://www.gnu.org/software/libmicrohttpd/manual/html_node/microhttpd_002dcb.html
 *
 * upload_data
 * the data being uploaded (excluding headers):
 *
 * POST data will be made available incrementally in upload_data; even if POST data is available,
 * the first time the callback is invoked there won’t be upload data, as this is done just after MHD parses the headers.
 * If supported by the client and the HTTP version, the application can at this point queue an error response to possibly avoid the upload entirely.
 * If no response is generated, MHD will (if required) automatically send a 100 CONTINUE reply to the client.
 *
 * Afterwards, POST data will be passed to the callback to be processed incrementally by the application.
 * The application may return MHD_NO to forcefully terminate the TCP connection without generating a proper HTTP response.
 * Once all of the upload data has been provided to the application, the application will be called again with 0 bytes of upload data.
 * At this point, a response should be queued to complete the handling of the request.
 *
 * upload_data_size
 * set initially to the size of the upload_data provided;
 * this callback must update this value to the number of bytes NOT processed;
 * unless external select is used, the callback maybe required to process at least some data.
 * If the callback fails to process data in multi-threaded or internal-select mode
 * and if the read-buffer is already at the maximum size that MHD is willing to use for reading
 * (about half of the maximum amount of memory allowed for the connection),
 * then MHD will abort handling the connection and return an internal server error to the client.
 * In order to avoid this, clients must be able to process upload data incrementally and reduce the value of upload_data_size.
 *
 */

static int uhd_request_started (void *sri_user_data, struct MHD_Connection *connection,
const char *url, const char *method, const char *version,
const char *upload_data, size_t *upload_data_size, void **con_cls)
{
	int ret = MHD_NO;
	char *page = NULL;
	sri_t *ws = sri_user_data;
	srci_t *ri = *con_cls;
	struct MHD_Response *response;
	const char *accept_header;
	const char *auth_header;
	const char *content_length_header;

	if(!url || !method || !version) { return MHD_NO; }

	if (!ri) {
#ifdef DEBUG
		//printf ("New %s request for %s using version %s\n", method, url, version);
#endif
		ri = calloc (1, sizeof(srci_t));
		if(!ri) { return MHD_NO; }
		*con_cls = (void *)ri;

		ri->url = strdup(url);
		ri->urllen = strlen(url);
		if(ri->urllen < ws->min_url_len) { return MHD_NO; }
		if(ri->urllen > ws->max_url_len) { return MHD_NO; }
		if(strlen(method) < 3) { return MHD_NO; }
		if(strlen(method) > 7) { return MHD_NO; }

		ri->ip = client_ip_str(connection);

		// Process Headers
		accept_header = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, HDRASTR);
		if(accept_header) { ri->accept = strdup(accept_header); }
		auth_header = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, HDRAUTHSTR);
		if(auth_header) { ri->auth = strdup(auth_header); }
		content_length_header = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, HDRCLSTR);
		if(content_length_header) {
			ri->content_length = atol(content_length_header);
			if(ri->content_length > ws->max_content_length) { return MHD_NO; }
		}

		// Process Method
		if (strcmp (method, "GET") == 0)			{ ri->method_type = METHOD_GET; }
		else if (strcmp (method, "POST") == 0)		{ ri->method_type = METHOD_POST; }
		else if (strcmp (method, "PUT") == 0)		{ ri->method_type = METHOD_PUT; }
		else if (strcmp (method, "DELETE") == 0)	{ ri->method_type = METHOD_DEL; }
		else if (strcmp (method, "OPTIONS") == 0)	{ ri->method_type = METHOD_OPT; }
		else { return MHD_NO; }

		//We need to ask the caller if XXX bytes of upload data is ok 

#ifdef DEBUG
		//if(content_length_header) printf ("Content-Length: %s \n", content_length_header);
#endif

		return MHD_YES;
	}

	// upload_data_size should always be a valid pointer
	// While we have post data to gather, gather and save
	if(upload_data && *upload_data_size) {
		size_t blobsize = *upload_data_size;
		size_t newbufsize = ri->post_data_len + blobsize;
		if(newbufsize > ri->content_length) { return MHD_NO; }

		ri->post_data = realloc(ri->post_data, newbufsize);
		memcpy(ri->post_data + ri->post_data_len, upload_data, blobsize);
		ri->post_data_len += blobsize;
		*upload_data_size = 0;	// Tell UHD that we processed all the data it gave us
		return MHD_YES;
	}

	// We should only get here after post processing is done
	// So now we check the length and make sure it matches
	if(ri->post_data_len < ri->content_length) { return MHD_NO; }

#ifdef SEAREST_UPLOAD_DEBUG
	if(ri->post_data) { printf ("Content: %s \n", ri->post_data); }
#endif

	page = process_request(ws, ri, ws->sri_user_data);

	if(page) {
		// What if the caller never set return code with srci_set_return_code() ?
		// it would appear that UHD will just hang and keep the connection open ?
		// Set return_code to OK and move on
		if(ri->return_code == 0) { ri->return_code = MHD_HTTP_OK; }

		// This will only work with text, modify this for binary file transfer
		response = MHD_create_response_from_buffer(strlen(page), page, MHD_RESPMEM_MUST_COPY);
		if(ri->content_type) { MHD_add_response_header(response, HDRCTSTR, ri->content_type); }
		if(ri->allow) { MHD_add_response_header(response, "Allow", ri->allow); }
		if(ri->cors) { MHD_add_response_header(response, "Access-Control-Allow-Origin", "*"); }
		ret = MHD_queue_response (connection, ri->return_code, response);
		MHD_destroy_response (response);
	}

#ifdef DEBUG
	//if(ret == MHD_NO)	{ fprintf (stderr, "Refusing Connection!\n"); }
	//else				{ fprintf (stderr, "Returning %d!\n", ri->return_code); }
#endif

	return ret;
}

static void uhd_request_completed (void *user_data, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe)
{
	srci_t *ri = *con_cls;
	if (!ri) { return; }

	if(ri->ip) { free(ri->ip); }
	if(ri->url) { free(ri->url); }
	if(ri->accept) { free(ri->accept); }
	if(ri->auth) { free(ri->auth); }
	if(ri->post_data) { free(ri->post_data); }
	if(ri->content_type) { free(ri->content_type); }
	if(ri->allow) { free(ri->allow); }
	if(ri->return_page) { free(ri->return_page); }
	free(ri);
	*con_cls = NULL;   
}

/* Specify a function that should be called before parsing the URI from the client.
The specified callback function can be used for processing the URI (including the options) before it is parsed.
The URI after parsing will no longer contain the options, which maybe inconvenient for logging.
This option should be followed by two arguments, the first one must be of the form
	void * my_logger(void * cls, const char * uri)
where the return value will be passed as *con_cls in calls to the MHD_AccessHandlerCallback when this request is processed later;
returning a value of NULL has no special significance; (however, note that if you return non-NULL,
you can no longer rely on the first call to the access handler having NULL == *con_cls on entry)
cls will be set to the second argument following MHD_OPTION_URI_LOG_CALLBACK.
Finally, uri will be the 0-terminated URI of the request. */
static void* uhd_logger (void *user_data, const char *uri)
{
#ifdef DEBUG_SEAREST_LOGGING
	printf("uhd_logger: %s\n", uri);
#endif
	return NULL;	//this becomes the value of *con_cls in request_started
}

static int uhd_client_connect (void *user_data, const struct sockaddr *addr, socklen_t addrlen)
{
	struct sockaddr_in *in = (struct sockaddr_in *)addr;
	sri_t *ws = user_data;
	int retval;
	char ip_str[128];
	char *ip;

	if(!addr) { return MHD_NO; }	//this should never happen
	if(!ws->addr_cb) { return MHD_YES; }

	switch(in->sin_family) {
		case AF_INET:
			inet_ntop(AF_INET, &in->sin_addr, &ip_str[0], INET_ADDRSTRLEN);
			break;
		case AF_INET6:
			inet_ntop(AF_INET6, &in->sin_addr, &ip_str[0], INET6_ADDRSTRLEN);
			break;
		default:
			return MHD_NO;
	}

#ifdef DEBUG
	//printf("New Connection from: %s\n", ip_str);
#endif

	ip = strdup(ip_str);
	retval = ws->addr_cb(ip, ws->sri_user_data);
	free(ip);
	return retval;
}

// Consider MHD_OPTION_CONNECTION_LIMIT here too
// Maximum number of concurrent connections to accept (followed by an unsigned int).
// The default is FD_SETSIZE - 4 (the maximum number of file descriptors supported by select minus four for stdin, stdout, stderr and the server socket).
// In other words, the default is as large as possible.
int searest_start(sri_t *ws, char *ip4addr, unsigned short port, void *sri_user_data)
{
#ifdef USEMHDOPTS
	int i;
	struct MHD_OptionItem *mhdops;
#endif
	//struct MHD_OptionItem mhdops[12];
	struct sockaddr_in server;

	// https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html
	// https://www.gnu.org/software/libmicrohttpd/manual/html_node/microhttpd_002dconst.html

	if(searest_node_count(ws) == 0) { return 1; }

	ws->sri_user_data = sri_user_data;

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if(!ip4addr) { server.sin_addr.s_addr = INADDR_ANY; }
	else { inet_pton(AF_INET, ip4addr, &server.sin_addr); }

#ifdef USEMHDOPTS
	i=0; mhdops=NULL;
	mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
	mhdops[i].option = MHD_OPTION_SOCK_ADDR;
	mhdops[i].value = (intptr_t)&server;
	mhdops[i++].ptr_value = NULL;

	mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
	mhdops[i].option = MHD_OPTION_URI_LOG_CALLBACK;
	mhdops[i].value = (intptr_t)&uhd_logger;
	mhdops[i++].ptr_value = sri_user_data;

	mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
	mhdops[i].option = MHD_OPTION_NOTIFY_COMPLETED;
	mhdops[i].value = (intptr_t)&uhd_request_completed;
	mhdops[i++].ptr_value = sri_user_data;

	mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
	mhdops[i].option = MHD_OPTION_CONNECTION_TIMEOUT;
	mhdops[i].value = ws->inactivity_timeout;
	mhdops[i++].ptr_value = NULL;

	mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
	mhdops[i].option = MHD_OPTION_CONNECTION_LIMIT;
	mhdops[i].value = ws->conn_limit;
	mhdops[i++].ptr_value = NULL;

	if(ws->https_cert && ws->https_key) {
		mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
		mhdops[i].option = MHD_OPTION_HTTPS_MEM_CERT;
		mhdops[i].value = (intptr_t)ws->https_cert;
		mhdops[i++].ptr_value = NULL;

		mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
		mhdops[i].option = MHD_OPTION_HTTPS_MEM_KEY;
		mhdops[i].value = (intptr_t)ws->https_key;
		mhdops[i++].ptr_value = NULL;

		if(ws->https_ca) {
			mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
			mhdops[i].option = MHD_OPTION_HTTPS_MEM_TRUST;
			mhdops[i].value = (intptr_t)ws->https_ca;
			mhdops[i++].ptr_value = NULL;
		}
	}

	mhdops = realloc(mhdops, (i+1)*sizeof(struct MHD_OptionItem));
	mhdops[i].option = MHD_OPTION_END;
	mhdops[i].value = 0;
	mhdops[i++].ptr_value = NULL;

	ws->mhd_srv = MHD_start_daemon (ws->socket_model | ws->ssl_flag, 0,
				&uhd_client_connect, ws,
				&uhd_request_started, ws,
				//MHD_OPTION_SOCK_ADDR, &server,
				MHD_OPTION_ARRAY, mhdops,
				MHD_OPTION_END);
#else
	if(ws->https_cert && ws->https_key) {
		if(ws->https_ca) {
			ws->mhd_srv = MHD_start_daemon (ws->socket_model | ws->ssl_flag, 0,
						&uhd_client_connect, ws,
						&uhd_request_started, ws,
						MHD_OPTION_SOCK_ADDR, &server, 
						MHD_OPTION_URI_LOG_CALLBACK, &uhd_logger, sri_user_data,
						MHD_OPTION_NOTIFY_COMPLETED, &uhd_request_completed, sri_user_data,
						MHD_OPTION_CONNECTION_TIMEOUT, ws->inactivity_timeout,
						MHD_OPTION_CONNECTION_LIMIT, ws->conn_limit,
						MHD_OPTION_HTTPS_MEM_CERT, ws->https_cert,
						MHD_OPTION_HTTPS_MEM_KEY, ws->https_key,
						MHD_OPTION_HTTPS_MEM_TRUST, ws->https_ca,
						MHD_OPTION_END);
		} else {
			ws->mhd_srv = MHD_start_daemon (ws->socket_model | ws->ssl_flag, 0,
						&uhd_client_connect, ws,
						&uhd_request_started, ws,
						MHD_OPTION_SOCK_ADDR, &server, 
						MHD_OPTION_URI_LOG_CALLBACK, &uhd_logger, sri_user_data,
						MHD_OPTION_NOTIFY_COMPLETED, &uhd_request_completed, sri_user_data,
						MHD_OPTION_CONNECTION_TIMEOUT, ws->inactivity_timeout,
						MHD_OPTION_CONNECTION_LIMIT, ws->conn_limit,
						MHD_OPTION_HTTPS_MEM_CERT, ws->https_cert,
						MHD_OPTION_HTTPS_MEM_KEY, ws->https_key,
						MHD_OPTION_END);
		}
	} else {
		ws->mhd_srv = MHD_start_daemon (ws->socket_model | ws->ssl_flag, 0,
						&uhd_client_connect, ws,
						&uhd_request_started, ws,
						MHD_OPTION_SOCK_ADDR, &server, 
						MHD_OPTION_URI_LOG_CALLBACK, &uhd_logger, sri_user_data,
						MHD_OPTION_NOTIFY_COMPLETED, &uhd_request_completed, sri_user_data,
						MHD_OPTION_CONNECTION_TIMEOUT, ws->inactivity_timeout,
						MHD_OPTION_CONNECTION_LIMIT, ws->conn_limit,
						MHD_OPTION_END);
	}
#endif

	if(!ws->mhd_srv) { return 2; }
	return 0;
}

void searest_set_https_cert(sri_t *ws, const char *cert)
{
	ws->https_cert = strdup(cert);
	ws->ssl_flag = MHD_USE_SSL;
}

void searest_set_https_key(sri_t *ws, const char *key)
{
	ws->https_key = strdup(key);
	ws->ssl_flag = MHD_USE_SSL;
}

void searest_set_https_ca(sri_t *ws, const char *ca)
{
	ws->https_ca = strdup(ca);
	ws->ssl_flag = MHD_USE_SSL;
}

void searest_set_conn_limit(sri_t *ws, unsigned int limit)
{
	ws->conn_limit = limit;
}

void searest_set_inactivity_timeout(sri_t *ws, int timeout)
{
	ws->inactivity_timeout = timeout;
}

void searest_set_internal_select(sri_t *ws)
{
	ws->socket_model = MHD_USE_SELECT_INTERNALLY;
}

void searest_set_addr_cb(sri_t *ws, void *func)
{
	ws->addr_cb = func;
}

void searest_stop(sri_t *ws)
{
	if(ws->mhd_srv) { MHD_stop_daemon(ws->mhd_srv); }
	ws->mhd_srv = NULL;
}

sri_t* searest_new(int urlmin, int urlmax, size_t contentmax)
{
	sri_t *ws = calloc(1, sizeof(sri_t));
	if(!ws) {
		fprintf(stderr, "calloc() failed!\n");
		exit(EXIT_FAILURE);
	}

	ws->socket_model = MHD_USE_THREAD_PER_CONNECTION;
	ws->conn_limit = FD_SETSIZE-4;  //-1 ??
	ws->min_url_len = urlmin;
	ws->max_url_len = urlmax;
	ws->max_content_length = contentmax;

	return ws;
}

void searest_del(sri_t *ws)
{
	searest_stop(ws);
	searest_node_destroy_all(ws);
	if(ws->https_cert) { free(ws->https_cert); }
	if(ws->https_key) { free(ws->https_key); }
	if(ws->https_ca) { free(ws->https_ca); }
	free(ws);
}
