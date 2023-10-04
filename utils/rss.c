#define _GNU_SOURCE /* strcasecmp */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "curl.h"
#include "rss.h"

static char *
rss_item(char *rss)
{
	/* get to item */
	char *item = strcasestr(rss, "<item>");
	if (!item) {
		return NULL;
	}

	return item+6;
}

static char *
item_link(char *item)
{
	/* get to link */
	char *link = strcasestr(item, "<link>");
	if (!link) {
		return NULL;
	}

	/* cleanup, return */
	strcasestr(link, "</link>")[0] = '\0';
	return link+6;
}

static bool
item_match(char *item, char *filter)
{
	/* get to title */
	char *title = strcasestr(item, "<title>");
	if (!title) {
		return NULL;
	}

	/* cleanup, return */
	strcasestr(title, "</title>")[0] = '\0';
	return strcasestr(title, filter);
}


char **
rss_download(char *url, char *dest, char *filter, char **downloaded)
{
	/* make sure downloaded is clean */
	if (downloaded) {
		free(downloaded);
	}

	/* get entire rss from url */
	chunk res = { .memory = malloc(1), .size = 0 };
	char *rss = curl_req(url, &res);
	if (!rss) {
		return NULL;
	}

	/* loop over each item and download matches */
	char *item = rss;
	int i = 1;
	while ((item = (rss_item(item)))) {
		/* filter if set */
		if (filter && filter[0] && !item_match(strdupa(item), filter)) {
			continue;
		}

		/* download and save filename, only when non existant */
		char title[512];
		if (!curl_download(item_link(strdupa(item)), dest, title)) {
			continue;
		}
		downloaded = realloc(downloaded, ++i*sizeof(char *));
		downloaded[i-2] = malloc(512);
		strcpy(downloaded[i-2], title);
	}

	if (downloaded) {
		downloaded[i-1] = malloc(1);
		downloaded[i-1][0] = '\0';
	}
	free(res.memory);
	return downloaded;
}
