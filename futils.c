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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

//off_t is of type (unsigned long int)? - 8 bytes on 64-bit systems
//what happens to large files on 32-bit systems?
static long file_size(const char *path, int v)
{
	struct stat stat_buf;
	int stat_ret = stat(path, &stat_buf);

	if(stat_ret != 0) {
		if(errno == ENOENT) {
			if(v) { fprintf(stderr, "%s does not exist!\n", path); }
			return -1;
		} else {
			if(v) { fprintf(stderr, "stat(%s) failed! %s\n", path, strerror(errno)); }
			return -2;
		}
	}

	if(!S_ISREG(stat_buf.st_mode)) {
		if(v) { fprintf(stderr, "%s is not a regular file!\n", path); }
		return -3;
	}

	return (long)stat_buf.st_size;
}

// This must be free()'d
char* get_file(const char *path)
{
	FILE *f;
	long size;
	size_t z;
	char *data;

	size = file_size(path, 1);
	if(size <= 0) { return NULL; }

	f = fopen(path, "r");
	if(!f) {
		fprintf(stderr, "fopen(%s, r) failed! %s\n", path, strerror(errno));
		return NULL;
	}

	data = malloc(size+1);
	z = fread(data, size, 1, f);
	if (z != 1) {
		fprintf(stderr, "fread(data, %ld, 1, %s) failed!\n", size, path);
		free(data);
		return NULL;
	}

	fclose(f);
	data[size] = 0;
	return data;
}
