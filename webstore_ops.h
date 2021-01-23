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

#ifndef __WEBSTORE_OPERATIONS_H__
#define __WEBSTORE_OPERATIONS_H__

#include "searest.h"
#include "rai.h"

// Default values for any single IP address:
// 5 requests allowed per 2 seconds
#define REQPERIOD (2)
#define REQCOUNT (5)

typedef struct {
	char *http_ip;
	unsigned short http_port;
	int use_threads;
	long max_post_data_size;
	char *certfile;
	char *keyfile;

	char *rdest;			// Redis Dest
	unsigned short rport;	// Redis Port
} srv_opts_t;

void webstore_start(srv_opts_t *);
void webstore_stop(void);

char* node128(char *, int, srci_t *, void *, void *);
char* node160(char *, int, srci_t *, void *, void *);
char* node224(char *, int, srci_t *, void *, void *);
char* node256(char *, int, srci_t *, void *, void *);
char* node384(char *, int, srci_t *, void *, void *);
char* node512(char *, int, srci_t *, void *, void *);

// WebStore Runtime data
typedef struct {
	rai_t rc;	//Redis Context
	int multithreaded;
} wsrt_t;

// WebStore Request Info
typedef struct {
	int type;
	int hashlen;
	char *url;
	int urllen;
} wsreq_t;

#endif
