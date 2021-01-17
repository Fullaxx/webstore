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
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

#include "webstore_log.h"

pthread_mutex_t g_log_lock = PTHREAD_MUTEX_INITIALIZER;
FILE *g_lf = NULL;

void log_add(int level, const char *fmt, ...)
{
	char *l_ptr;
	struct timespec ts;
	char datetime[16];
	va_list argptr;

	if(!g_lf) { return; }

	switch(level) {
		case WSLOG_INFO:	l_ptr = WSLOG_INFO_STR;	break;
		case WSLOG_WARN:	l_ptr = WSLOG_WARN_STR;	break;
		case WSLOG_ERR:		l_ptr = WSLOG_ERR_STR;	break;
		case WSLOG_CRIT:	l_ptr = WSLOG_CRIT_STR;	break;
#ifdef DEBUG
		case WSLOG_DEBUG:	l_ptr = WSLOG_DEBUG_STR;	break;
#endif
		default: return;
	}

	// Calculate Timestamp
	clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
	strftime(datetime, sizeof(datetime), "%y%m%d-%H%M%S", localtime(&ts.tv_sec));

	// Lock
	pthread_mutex_lock(&g_log_lock);

	// Print Header
	fprintf(g_lf, "%s.%09ld %s ", datetime, ts.tv_nsec, l_ptr);

	// Print Log Message
	va_start(argptr, fmt);
	vfprintf(g_lf, fmt, argptr);
	va_end(argptr);

	fprintf(g_lf, "\n");
	fflush(g_lf);

	// Unlock
	pthread_mutex_unlock(&g_log_lock);
}

int log_open(char *logfile)
{
	//if(!logfile) { return; }

	g_lf = fopen(logfile, "a");
	if(!g_lf) {
		fprintf(stderr, "Error opening logfile: %s\n", logfile);
		return -1;
	}

	return 0;
}

void log_close(void)
{
	pthread_mutex_lock(&g_log_lock);
	if(g_lf) {
		fprintf(g_lf, "\n");
		fclose(g_lf);
	}
	pthread_mutex_unlock(&g_log_lock);
}
