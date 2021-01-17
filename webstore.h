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

#ifndef __WEBSTORE_H__
#define __WEBSTORE_H__

#define MAXENCMSGSIZ (10*1024*1024)

#define HASHMD5LEN    (32)
#define HASHSHA1LEN   (40)
#define HASHSHA224LEN (56)
#define HASHSHA256LEN (64)
#define HASHSHA384LEN (96)
#define HASHSHA512LEN (128)

#define HASHMD5    (1)
#define HASHSHA1   (2)
#define HASHSHA224 (3)
#define HASHSHA256 (4)
#define HASHSHA384 (5)
#define HASHSHA512 (6)

#endif
