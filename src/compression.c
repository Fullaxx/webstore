// compression.c - Simplistic API to miniz
// Public domain, 28 Jan 2021, Brett Kuskie, fullaxx@gmail.com

#include <stdlib.h>

#ifdef MINIZ_COMPRESSION

#include "compression.h"
#include "miniz.c"

/*
void print_error(md_t *mzstor)
{
	if(mzstor->err) { }
}
*/

md_t* mza_squash(const unsigned char *data, size_t size)
{
	md_t *mz;

	mz = calloc(1, sizeof(md_t));
	if(!mz) { return NULL; }

	mz->comp_len = mz_compressBound(size);
	mz->comp_data = calloc(1, mz->comp_len);
	if(!mz->comp_data) {
		mz->err = Z_MEM_ERROR;
		mz->comp_len = 0;
		return mz;
	}

	mz->err = mz_compress(mz->comp_data, &mz->comp_len, data, size);
	if(mz->err != Z_OK) {
		free(mz->comp_data);
		mz->comp_data = NULL;
		mz->comp_len = 0;
	}

	return mz;
}

md_t* mza_restore(const unsigned char *comp_str, size_t comp_len, size_t orig_len)
{
	md_t *mz;

	mz = calloc(1, sizeof(md_t));
	if(!mz) { return NULL; }

	if(orig_len > 0) { mz->orig_len = orig_len; }
	else { mz->orig_len = 10*comp_len; }
	mz->orig_data = calloc(1, mz->orig_len);
	if(!mz->orig_data) {
		mz->err = Z_MEM_ERROR;
		mz->orig_len = 0;
		return mz;
	}

	mz->err = mz_uncompress(mz->orig_data, &mz->orig_len, comp_str, comp_len);
	if(mz->err != Z_OK) {
		free(mz->orig_data);
		mz->orig_data = NULL;
		mz->orig_len = 0;
	}

	return mz;
}

void mza_free(md_t *mz)
{
	if(mz) {
		if(mz->orig_data) { memset(mz->orig_data, 0, mz->orig_len); free(mz->orig_data); }
		if(mz->comp_data) { memset(mz->comp_data, 0, mz->comp_len); free(mz->comp_data); }
		memset(mz, 0, sizeof(md_t));
		free(mz);
	}
}

#endif
