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

#ifndef __WEBSTORE_GCRYPT_HELPERS__
#define __WEBSTORE_GCRYPT_HELPERS__

#include <gcrypt.h>

#define WEBSTORE_GCRYPT_MINVERS "1.6.0"
#define WEBSTORE_USING_SECURE_MEMORY
#define WEBSTORE_SECMEM_SIZE (65536)

// USE THIS TO PICK ALGORITHMS FROM LIBGCRYPT
#define GCALG128 (GCRY_MD_MD5)
#define GCALG160 (GCRY_MD_SHA1)
#define GCALG224 (GCRY_MD_SHA224)
#define GCALG256 (GCRY_MD_SHA256)
#define GCALG384 (GCRY_MD_SHA384)
#define GCALG512 (GCRY_MD_SHA512)

char* create_token(int, char *, size_t);
int ws_gcinit(char *);

#endif
