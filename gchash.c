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

#include "webstore.h"
#include "gchash.h"

#define mygcerr(s, e) fprintf(stderr, "%s failed: %s\n", s, gcry_strerror(e));

// this must be free()'d
char* gen_hash(unsigned char *digest, int dsize)
{
	int i, hash_len;
	char *hash;

	hash_len = (dsize*2)+1;
	hash = malloc(hash_len);
	memset(hash, 0, hash_len);
	for(i=0; i<dsize; i++) { sprintf(&hash[i*2], "%02x", digest[i]); }

	return hash;
}

char* create_token(int alg, char *msg, size_t msglen)
{
	gcry_md_hd_t h;
	gcry_error_t err;
	unsigned int flags;
	int dsize, gcalg;
	unsigned char *digest;
	char *token;

	// flags = GCRY_MD_FLAG_SECURE; TEST THIS
	flags = 0;
	switch(alg) {
		case HASHMD5:
			gcalg = GCRY_MD_MD5;
			err = gcry_md_open (&h, GCRY_MD_MD5, flags);
			break;
		case HASHSHA1:
			gcalg = GCRY_MD_SHA1;
			err = gcry_md_open (&h, GCRY_MD_SHA1, flags);
			break;
		case HASHSHA224:
			gcalg = GCRY_MD_SHA224;
			err = gcry_md_open (&h, GCRY_MD_SHA224, flags);
			break;
		case HASHSHA256:
			gcalg = GCRY_MD_SHA256;
			err = gcry_md_open (&h, GCRY_MD_SHA256, flags);
			break;
		case HASHSHA384:
			gcalg = GCRY_MD_SHA384;
			err = gcry_md_open (&h, GCRY_MD_SHA384, flags);
			break;
		case HASHSHA512:
			gcalg = GCRY_MD_SHA512;
			err = gcry_md_open (&h, GCRY_MD_SHA512, flags);
			break;
		default: return NULL;
	}

	if(err) { mygcerr("gcry_md_open()", err); return NULL; }

#if 0
	//err = gcry_md_open (&h, GCRY_MD_MD5, GCRY_MD_FLAG_SECURE); NEED MORE MEMORY
	err = gcry_md_open (&h, GCRY_MD_MD5, 0);
	if(err) { mygcerr("gcry_md_open()", err); return NULL; }
	err = gcry_md_enable (h, GCRY_MD_SHA1);
	if(err) { mygcerr("gcry_md_enable()", err); return NULL; }
	err = gcry_md_enable (h, GCRY_MD_SHA224);
	if(err) { mygcerr("gcry_md_enable()", err); return NULL; }
	err = gcry_md_enable (h, GCRY_MD_SHA256);
	if(err) { mygcerr("gcry_md_enable()", err); return NULL; }
	err = gcry_md_enable (h, GCRY_MD_SHA384);
	if(err) { mygcerr("gcry_md_enable()", err); return NULL; }
	err = gcry_md_enable (h, GCRY_MD_SHA512);
	if(err) { mygcerr("gcry_md_enable()", err); return NULL; }
#endif

	gcry_md_write (h, msg, msglen);
	gcry_md_final (h);

	dsize = gcry_md_get_algo_dlen(gcalg);
	digest = gcry_md_read (h, gcalg);
	token = gen_hash(digest, dsize);

	gcry_md_close(h);
	return token;
}

int ws_gcinit(char *vers)
{

/*  If the application requests FIPS mode using the control command GCRYCTL_FORCE_FIPS_MODE.
 *  This must be done prior to any initialization (i.e. before gcry_check_version). */
#ifdef WEBSTORE_USING_FIPS
	gcry_control (GCRYCTL_FORCE_FIPS_MODE);
#endif

/*  Version check should be the very first call because it
 *  makes sure that important subsystems are initialized.
 *  #define NEED_LIBGCRYPT_VERSION to the minimum required version. */
	if (!gcry_check_version (vers)) {
		fprintf (stderr, "libgcrypt is too old (need %s, have %s)\n",
		vers, gcry_check_version (NULL));
		return 1;
	}

/*  We don't want to see any warnings, e.g. because we have not yet
 *  parsed program options which might be used to suppress such warnings. */
	gcry_control (GCRYCTL_SUSPEND_SECMEM_WARN);

/*  ... If required, other initialization goes here.  Note that the
 *  process might still be running with increased privileges and that
 *  the secure memory has not been initialized.  */

/*  Allocate a pool of 16k secure memory.  This makes the secure memory
	available and also drops privileges where needed.  Note that by
	using functions like gcry_xmalloc_secure and gcry_mpi_snew Libgcrypt
	may expand the secure memory pool with memory which lacks the
	property of not being swapped out to disk.   */
#ifdef WEBSTORE_USING_SECURE_MEMORY
	gcry_control (GCRYCTL_INIT_SECMEM, WEBSTORE_SECMEM_SIZE, 0);
#else
	gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
#endif

/*  It is now okay to let Libgcrypt complain when there was/is
 *  a problem with the secure memory. */
	gcry_control (GCRYCTL_RESUME_SECMEM_WARN);

/*  ... If required, other initialization goes here.  */

/*  Tell Libgcrypt that initialization has completed. */
	gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);

	return 0;
}
