// compression.h - Simplistic API to miniz
// Public domain, 28 Jan 2021, Brett Kuskie, fullaxx@gmail.com

#ifndef __WEBSTORE_COMPRESSION_API_H__
#define __WEBSTORE_COMPRESSION_API_H__

#include <sys/types.h>

typedef struct miniz_data {
	unsigned char *orig_data;
	size_t orig_len;
	unsigned char *comp_data;
	size_t comp_len;
	int err;
} md_t;

md_t* mza_squash(const unsigned char *, size_t);
md_t* mza_restore(const unsigned char *, size_t, size_t);
void mza_free(md_t *);

#endif	/* __WEBSTORE_COMPRESSION_API_H__ */
