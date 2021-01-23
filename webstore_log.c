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
	char date[9];
	char time[7];
	char timestamp[32];
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
	clock_gettime(CLOCK_REALTIME, &ts);
	strftime(date, sizeof(date), "%y%m%d", localtime(&ts.tv_sec));
	strftime(time, sizeof(time), "%H%M%S", localtime(&ts.tv_sec));
	snprintf(timestamp, sizeof(timestamp), "%s %s %09ld", date, time, ts.tv_nsec);

	// Lock
	pthread_mutex_lock(&g_log_lock);

	// Print Header
	fprintf(g_lf, "%s %s ", timestamp, l_ptr);

	// Print Log Message
	va_start(argptr, fmt);
	vfprintf(g_lf, fmt, argptr);
	va_end(argptr);

	fprintf(g_lf, "\n");

	// Unlock
	pthread_mutex_unlock(&g_log_lock);
}

int log_open(char *logfile)
{
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
	if(g_lf) { fclose(g_lf); }
	pthread_mutex_unlock(&g_log_lock);
}

void log_flush(void)
{
	pthread_mutex_lock(&g_log_lock);
	if(g_lf) { fflush(g_lf); }
	pthread_mutex_unlock(&g_log_lock);
}
