/*
	SeaRest is a RESTFul service framework leveraging libmicrohttpd
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

#ifndef __SEAREST_H__
#define __SEAREST_H__

#include <time.h>
#include <microhttpd.h>

#ifdef SRNODECHRONOMETRY
#define TIMESLOTS (100)
#endif

#ifndef METHOD
#define METHOD(x) srci_get_method_type(x)
#endif

#define METHOD_GET	(1)
#define METHOD_POST	(2)
#define METHOD_PUT	(3)
#define METHOD_DEL	(4)
#define METHOD_OPT	(5)

#define HDRASTR "Accept"
#define HDRCTSTR "Content-Type"
#define HDRCLSTR "Content-Length"
#define HDRAUTHSTR "Authorization"

#define MIMETYPETXTPLAINSTR "text/plain"
#define MIMETYPEAPPBINSTR "application/octet-stream"
#define MIMETYPEAPPJSONSTR "application/json"
#define MIMETYPEAPPXMLSTR "application/xml"

#define SR_IP_ACCEPT	(MHD_YES)
#define SR_IP_DENY		(MHD_NO)

#define SR_ADDR_CALLBACK(CB)	int (CB)(char *, void *);
#define SR_NODE_CALLBACK(CB)	char* (CB)(char *, int, void *, void *, void *);

typedef struct searest_node {
	unsigned int num;
	int disabled;
	char *root;
	SR_NODE_CALLBACK(*cb);
	void *nud;

	time_t last_atime;
	unsigned long acount;
#ifdef SRNODECHRONOMETRY
	int dai;	//duration array index
	long da[TIMESLOTS];
	int statsready;
#endif

	struct searest_node *next;
	struct searest_node *prev;
} srn_t;

typedef struct searest_instance {
	void *sri_user_data;
	SR_ADDR_CALLBACK(*addr_cb);
	struct MHD_Daemon *mhd_srv;

	char *https_cert;
	char *https_key;
	char *https_ca;
	int ssl_flag;
	int socket_model;
	int inactivity_timeout;
	unsigned int conn_limit;
	int min_url_len;
	int max_url_len;
	size_t max_content_length;

	srn_t *nodelist_head;
} sri_t;

typedef struct searest_conn_info {
	char *ip;
	int method_type;
	char *url;
	int urllen;
	char *accept;			//request - from browser
	char *auth;				//request - from browser

	// Evreyhting we need for processing uploaded data
	size_t content_length;
	unsigned char *post_data;
	size_t post_data_len;

	char *content_type;	//response - to browser
	char *allow;		//response - to browser
	int cors;
	int return_code;
	char *return_page;
} srci_t;

char* srci_get_client_ip(srci_t *ri);
char *srci_get_browser_auth(srci_t *ri);
int srci_browser_requests_text(srci_t *ri);
int srci_browser_requests_xml(srci_t *ri);
int srci_browser_requests_json(srci_t *ri);
int srci_get_method_type(srci_t *ri);
void srci_set_response_content_type(srci_t *ri, char *ct);
void srci_set_response_allow(srci_t *ri, char *a);
void srci_set_response_cors(srci_t *ri);

const unsigned char* srci_get_post_data_ptr(srci_t *ri);
size_t srci_get_post_data_size(srci_t *ri);
void srci_set_return_code(srci_t *ri, int code);

void searest_set_https_cert(sri_t *ws, const char *cert);
void searest_set_https_key(sri_t *ws, const char *key);
void searest_set_https_ca(sri_t *ws, const char *ca);
void searest_set_inactivity_timeout(sri_t *ws, int timeout);
void searest_set_internal_select(sri_t *ws);
void searest_set_addr_cb(sri_t *ws, void *func);
void searest_stop(sri_t *ws);
int searest_start(sri_t *ws, char *ip4addr, unsigned short port, void *sri_user_data);
sri_t* searest_new(int urlmin, int urlmax, size_t contentmax);
void searest_del(sri_t *ws);

#ifdef SRNODECHRONOMETRY
long searest_node_get_avg_duration(sri_t *ws, char *rootname);
#endif

int searest_node_set_disabled(sri_t *ws, char *rootname);
int searest_node_set_enabled(sri_t *ws, char *rootname);
int searest_node_add(sri_t *ws, char *rootname, void *func, void *node_user_data);
unsigned int searest_node_count(sri_t *ws);

#endif
