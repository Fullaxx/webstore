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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "searest.h"

pthread_mutex_t nodelist_mutex;
void nl_lock(void) { pthread_mutex_lock(&nodelist_mutex); }
void nl_unlock(void) { pthread_mutex_unlock(&nodelist_mutex); }

srn_t* searest_node_find(sri_t *ws, char *rootname)
{
	size_t nodelen;
	srn_t *cursor;
	srn_t *answer = NULL;

	nl_lock();

	cursor = ws->nodelist_head;
	while(cursor) {
		nodelen = strlen(cursor->root);
		if(strncmp(rootname, cursor->root, nodelen) == 0) { answer = cursor; break; }
		cursor = cursor->next;
	}

	nl_unlock();
	return answer;
}

size_t searest_node_len(srn_t *n)
{
	return strlen(n->root);
}

void searest_node_set_access(srn_t *n)
{
	n->last_atime = time(NULL);
	n->acount++;
}

#ifdef SRNODECHRONOMETRY
long searest_node_get_avg_duration(sri_t *ws, char *rootname)
{
	int i;
	srn_t *n;
	unsigned long total = 0;

	n = searest_node_find(ws, rootname);
	if(!n) { return -1; }

	nl_lock();
	if(n->statsready) {
		for(i=0; i<TIMESLOTS; i++) { total += n->da[i]; }
	}
	nl_unlock();

	return (total / TIMESLOTS);
}

void searest_node_save_time(srn_t *n, long duration)
{
	int i;

	nl_lock();
	i = (n->dai++ % TIMESLOTS);
	n->da[i] = duration;
	if((n->dai > 1) && (i == 0)) { n->statsready = 1; }
	nl_unlock();
}
#endif

int searest_node_set_disabled(sri_t *ws, char *rootname)
{
	srn_t *n = searest_node_find(ws, rootname);
	if(!n) { return 1; }

	n->disabled = 1;

	return 0;
}

int searest_node_set_enabled(sri_t *ws, char *rootname)
{
	srn_t *n = searest_node_find(ws, rootname);
	if(!n) { return 1; }

	n->disabled = 0;

	return 0;
}

int searest_node_is_disabled(srn_t *n)
{
	return n->disabled;
}

int searest_node_add(sri_t *ws, char *rootname, void *func, void *node_user_data)
{
	srn_t *cursor;

	if(!rootname) { return 1; }
	if(!func) { return 2; }

	nl_lock();

	cursor = ws->nodelist_head;
	if(!cursor) {
		cursor = ws->nodelist_head = calloc(1, sizeof(srn_t));
		cursor->num = 1;
	} else {
		while(cursor->next) cursor = cursor->next;
		cursor->next = calloc(1, sizeof(srn_t));
		if(!cursor->next) { fprintf(stderr, "calloc() failed!"); exit(EXIT_FAILURE); };
		cursor->next->prev = cursor;
		cursor = cursor->next;
		cursor->num = cursor->prev->num + 1;
	}

	cursor->root = strdup(rootname);
	cursor->cb = func;
	cursor->nud = node_user_data;

	nl_unlock();
	return 0;
}

void searest_node_destroy_all(sri_t *ws)
{
	srn_t *prev;
	srn_t *cursor;

	nl_lock();

	cursor = ws->nodelist_head;
	while(cursor) {
		if(cursor->root) { free(cursor->root); }

		prev = cursor;
		cursor = cursor->next;
		free(prev);
	}

	nl_unlock();
}

unsigned int searest_node_count(sri_t *ws)
{
	srn_t *cursor;
	unsigned int retval = 0;

	nl_lock();

	cursor = ws->nodelist_head;
	while(cursor) {
		retval = cursor->num;
		cursor = cursor->next;
	}

	nl_unlock();
	return retval;
}
