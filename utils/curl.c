#include <curl/curl.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "curl.h"
#include "utils.h"
#include "log.h"

static bool curl_init = 0;

static size_t
curl_wrcb(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	chunk *mem = (chunk *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

static size_t
curl_hcb(char *buffer, size_t size, size_t nitems, void *userdata)
{
	/* capture filename from content-disposition */
	if (!strncmp(buffer, "content-disposition:", 20)) {
		char *name = (char *) userdata;
		strcpy(name, strstr(buffer, "filename")+10);
		strchr(name, '"')[0] = '\0';
	}

	return nitems * size;
}
 
char *
curl_req(char *url, chunk *res)
{
	if (!curl_init) { /* req always done before download so this is fine */
		curl_global_init(CURL_GLOBAL_ALL);
	}

	/* configure curl request */
	CURL *curl = curl_easy_init(); 
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_wrcb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, res);

	/* request and return */
	CURLcode r = curl_easy_perform(curl);
	if (r != CURLE_OK) {
		log_err("curl error: %s\n", curl_easy_strerror(r));
	}
	curl_easy_cleanup(curl);
	return res->memory;
}

char *
curl_download(char *url, char *dest, char *name)
{
	chunk res = { .memory = malloc(1), .size = 0 };

	/* configure curl request */
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_hcb);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, name);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_wrcb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);

	/* request and check filename */
	CURLcode r = curl_easy_perform(curl);
	if (r != CURLE_OK) {
		log_err("curl error: %s\n", curl_easy_strerror(r));
	}
	curl_easy_cleanup(curl);
	if (strlen(name) < 1) {
		strcpy(name, basename(url));
	}
	strcpy(name, curl_easy_unescape(curl, name, 0, &(int){0}));
	
	/* make sure path exists */
	if(mkpath(dest)) {
		log_err("failed to create config dir\n");
		exit(1);
	}

	/* check if file exists */
	char path[2000];
	strcpy(path, dest);
	strcat(path, "/");
	strcat(path, name);
	if (access(path, F_OK) == 0) {
		return NULL;
	}

	/* save to file */
	FILE *torrent = fopen(path, "w");
	if (!torrent) {
		log_err("failed to create file: %s\n", path);
	}
	fwrite(res.memory, 1, res.size, torrent);
	fclose(torrent);
	return name;
}
