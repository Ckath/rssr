#ifndef CURL_UTIL
#define CURL_UTIL

typedef struct chunk {
	char *memory;
	size_t size;
} chunk;

char *curl_req(char *url, chunk *res);
char *curl_download(char *url, char *dest, char *name);

#endif
