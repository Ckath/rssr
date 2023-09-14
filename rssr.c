#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "utils/log.h"
#include "utils/rss.h"
#include "utils/utils.h"

#define ADD 0
#define DEL 1
#define LIST 2

/* config, initialize with default */
static int delay = 22;
static char cmd[2000] ="transmission-remote -a \"{file}\" -g \"{dest}\"";
static char dir[2000] = "~/.config/rssr/";

static void
usage()
{
	fputs("usage: rssr [-t delay] [-c cmd] [-l dir] [-a url -g dest -f filter] [-d id] [-p c/u/d]\n"\
			"options\n"\
			"-t\tdelay between requests (seconds), default 22\n"\
			"-c\ttorrent add cmd (use {file} and {dest} in it), default transmission-remote -a \"{file}\" -g \"{dest}\"\n"\
			"-l\tconfig and cache location, default ~/.config/rssr/\n"\
			"actions\n"\
			"-a\tadd \"url\" -g \"dest\" to config the -f filter is optional\n"\
			"-d\tdelete entry matching id\n"\
			"-p\tprint current feeds in format, c csv, u urls, d destinations\n",
			stderr);
	exit(1);
}

static FILE *
feeds_open(char *mode)
{
	/* build config path */
	char config[2000];
	if (strchr(dir, '/')[strlen(dir)-1] != '/') {
		strcat(dir, "/");
	}
	strcpy(config, dir);

	/* if possibly creating, ensure path */
	if(mode[0] == 'a' && mkpath(config)) {
		log_err("failed to create config dir\n");
		exit(1);
	}

	/* check and open */
	FILE *feeds;
	strcat(config, "feeds");
	if (mode[0] != 'a' && access(config, F_OK) != 0) {
		log_info("no feeds, nothing to do\n");
		exit(0);
	} if (!(feeds = fopen(config, mode))) {
		log_err("failed to open feeds file\n");
		exit(1);
	}

	return feeds;
}

static void
add_rss(char *url, char *dest, char *filter)
{
	/* check/open feeds first */
	FILE *feeds = feeds_open("a");

	/* add feed */
	fprintf(feeds, "%s%s%s\n", url, dest, filter);
	fclose(feeds);
	log_info("added feed\n");
}

static void
del_rss(long id)
{
	/* check/open feeds first */
	FILE *feeds = feeds_open("r+");

	/* create buffer to hold modified file */
	fseek(feeds, 0L, SEEK_END);
	char *buf = malloc(ftell(feeds)+1);
	fseek(feeds, 0L, SEEK_SET);
	strcpy(buf, "");
	

	/* fish out all lines but deleted one */
	long i = 0L;
	char line[2000];
	while(fgets(line, 2000, feeds) != NULL) {
		if (i++ == id) {
			log_info("deleted feed\n");
			continue;
		}
		strcat(buf, line);
	}
	fseek(feeds, 0L, SEEK_SET);
	fwrite(buf, 1, strlen(buf), feeds);
	ftruncate(fileno(feeds), strlen(buf));
	fclose(feeds);
}

static void
list_rss(char *option)
{
	/* check/open feeds first */
	FILE *feeds = feeds_open("r");

	/* print all depending on option */
	int id = 0;
	char line[2000];
	while(fgets(line, 2000, feeds) != NULL) {
		switch(option[0]) {
			case 'u':
				strchr(line, '')[0] = '\0';
				strcat(line, "\n");
				break;
			case 'd':
				strcpy(line, strchr(line, '')+1);
				strchr(line, '')[0] = '\0';
				strcat(line, "\n");
				break;
			case 'c':
			default:
				strrplc(line, "", ",");
				break;
		}
		printf("%i: %s", id++, line);
	}
}

static void
add_torrents(char **files, char *dest)
{
	if (!files) {
		return; /* nothing to do */
	} for (int i = 0; files[i]; ++i) {
		char path[2000];
		char c[2000];

		strcpy(path, dir); 
		strcat(path, "cache/");
		strcat(path, files[i]);
		strrplc(path, "\"", "\\\"");
		strcpy(c, cmd);
		strrplc(c, "{file}", path);
		strrplc(c, "{dest}", dest);
		log_info("cmd output: ");
		if (!system(c)) {
			log_info("torrent %s added\n", files[i]);
		} else {
			log_err("failed to add %s\n", files[i]); 
		}
	}
}

static void
handle_feeds()
{
	/* check/open feeds first */
	FILE *feeds = feeds_open("r");

	/* cache path for torrent files */
	char cache[2000];
	strcpy(cache, dir);
	strcat(cache, "cache");

	/* parse each line */
	char line[2000];
	char **downloaded;
	while(fgets(line, 2000, feeds) != NULL && !sleep(delay)) {
		/* filter url, destination, filter */
		char url[2000];
		char dest[2000];
		char filter[2000];
		strcpy(url, line);
		strcpy(dest, strchr(line, '')+1);
		strcpy(filter, strchr(dest, '')+1);
		strchr(url, '')[0] = '\0';
		strchr(dest, '')[0] = '\0';
		strchr(filter, '\n')[0] = '\0';

		/* download and add results */
		add_torrents(rss_download(url, cache, filter, downloaded), dest);
	}
}

int
main(int argc, char *argv[])
{
	/* parameters */
	char url[2000] = { '\0' };
	char dest[2000] = { '\0' };
	char filter[2000] = { '\0' };
	long id = -1;

	/* resolve home for path */
	char *home;
	if (!(home = getenv("HOME"))) {
		home = getpwuid(getuid())->pw_dir;
	}
	strrplc(dir, "~", home);

	/* handle options, update values */
	unsigned c;
	char a = -1;
	while ((c = getopt(argc, argv, "t:c:l:g:f:a:d:p:h")) != -1) {
		switch (c) {
			case 't':
				delay = strtoul(optarg, NULL, 0);
				break;
			case 'c':
				if (strlen(optarg) > 2000) { 
					log_err("cmd too long\n");
					exit(1);
				}
				strcpy(cmd, optarg);
				break;
			case 'l':
				if (strlen(optarg) > 2000) { 
					log_err("path too long\n");
					return 1;
				}
				strcpy(dir, optarg);
				strrplc(dir, "~", home);
				break;
			case 'g':
				if (strlen(optarg) > 2000) { 
					log_err("path too long\n");
					return 1;
				}
				strcpy(dest, optarg);
				break;
			case 'f':
				if (strlen(optarg) > 2000) { 
					log_err("filter too long\n");
					return 1;
				}
				strcpy(filter, optarg);
				break;
			case 'a':
				a = ADD;
				strcpy(url, optarg);
				break;
			case 'd':
				a = DEL;
				id = strtol(optarg, NULL, 10);
				break;
			case 'p':
				a = LIST;
				strcpy(filter, optarg ? optarg : "");
				break;
			case 'h':
			default:
				usage();
		}
	}

	switch(a) {
		case ADD:
			if (!dest[0]) {
				log_err("missing dest\n");
				return 1;
			}
			add_rss(url, dest, filter);
			break;
		case DEL:
			del_rss(id);
			break;
		case LIST:
			list_rss(filter);
			break;
		default:
			handle_feeds();
			break;
	}

	/* not doing any action, do the thing */
	return 0;
}
