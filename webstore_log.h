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

#ifndef __WEBSTORE_LOGGING_H__
#define __WEBSTORE_LOGGING_H__

#define	WSLOG_INFO		1
#define	WSLOG_INFO_STR	"INF"
#define	WSLOG_WARN		2
#define	WSLOG_WARN_STR	"WRN"
#define	WSLOG_ERR		3
#define	WSLOG_ERR_STR	"ERR"
#define	WSLOG_CRIT		4
#define	WSLOG_CRIT_STR	"CRT"
#define	WSLOG_DEBUG		5
#define	WSLOG_DEBUG_STR	"DBG"

void log_add(int, const char *, ...);
int log_open(char *);
void log_close(void);

#endif
