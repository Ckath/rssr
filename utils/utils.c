#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utils.h"

int
mkpath(char *path)
{
	if (access(path, F_OK) == 0) {
		return 0;
	}

	char tmp[2000];
	char *p = NULL;
	size_t len;
	int e = 0;

	strcpy(tmp, path);
	len = strlen(tmp);
	if (tmp[len - 1] == '/') {
		tmp[len - 1] = '\0';
	} for (p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			if (access(tmp, F_OK) != 0) {
				e = mkdir(tmp, 0755);
			}
			*p = '/';
		}
	} if (!e && access(tmp, F_OK) != 0) {
		e = mkdir(tmp, 0755);
	}
	return e;
}

size_t
strrplc(char *haystack, char *needle, char *replace)
{	
	/* replace occurrences and return fixed string */
	char *match;
	size_t matches = 0;
	while ((match = strstr(haystack, needle))) {
		char tmp[2000] = {'\0'};
		strncpy(tmp, haystack, match-haystack);
		strcat(tmp, replace);
		strcat(tmp, match+strlen(needle));
		strcpy(haystack, tmp);
		matches++;
	}
	return matches;
}
