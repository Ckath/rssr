#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "utils/log.h"
#include "utils/rss.h"
#include "utils/utils.h"

/* config, initialize with default */
static int delay = 22;
static char cmd[1337] ="transmission-remote -a \"{file}\" -g \"{dest}\"";
static char dir[1337] = "~/.config/rssr/";

static void
usage()
{
	fputs("usage: rssr [-t delay] [-c cmd] [-l dir] [-a url;dest;filter] [-d id] [-p c/u/d]\n"\
			"options\n"\
			"-t\tdelay between requests (seconds), default 22\n"\
			"-c\ttorrent add cmd (use {file} and {dest} in it), default transmission-remote -a \"{file}\" -g \"{dest}\"\n"\
			"-l\tconfig and cache location, default ~/.config/rssr/\n"\
			"actions\n"\
			"-a\tadd url;dest to config the ;filter is optional\n"\
			"-d\tdelete entry matching id\n"\
			"-p\tprint current feeds, c csv, u urls, d destinations\n",
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
	strcat(config, "feeds");

	/* check and open */
	FILE *feeds;
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
	/* build path to feeds dir/file */
	char config[2000];
	if (strchr(dir, '/')[strlen(dir)-1] != '/') {
		strcat(dir, "/");
	}
	strcpy(config, dir);

	/* make sure the dir exists */
	if(mkpath(config)) {
		log_err("failed to create config dir\n");
		exit(1);
	}

	/* open file and add feed */
	FILE *feeds;
	strcat(config, "feeds");
	if (!(feeds = fopen(config, "a"))) {
		log_err("failed to open feeds file\n");
		exit(1);
	}
	fprintf(feeds, "%s;%s;%s;\n", url, dest, filter);
	fclose(feeds);
	log_info("added feed\n");
}

static void
del_rss(long id)
{
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
	/* build config path */
	char config[2000];
	if (strchr(dir, '/')[strlen(dir)-1] != '/') {
		strcat(dir, "/");
	}
	strcpy(config, dir);
	strcat(config, "feeds");

	/* check and open */
	FILE *feeds;
	if (access(config, F_OK) != 0) {
		log_info("no feeds, nothing to show\n");
		exit(0);
	} if (!(feeds = fopen(config, "r"))) {
		log_err("failed to open feeds file\n");
		exit(1);
	}

	/* read and print in desired format */
	char line[2000];
	char **downloaded;
	int id = 0;
	while(fgets(line, 2000, feeds) != NULL) {
		/* filter url, destination, filter */
		switch(option[0]) {
			case 'u':
				strchr(line, ';')[0] = '\0';
				strcat(line, "\n");
				break;
			case 'd':
				strcpy(line, strchr(line, ';')+1);
				strchr(line, ';')[0] = '\0';
				strcat(line, "\n");
				break;
			case 'c':
			default:
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
		char c[1337];

		strcpy(path, dir); 
		strcat(path, "cache/");
		strcat(path, files[i]);
		strrplc(path, "\"", "\\\"");
		strcpy(c, cmd);
		strrplc(c, "{file}", path);
		strrplc(c, "{dest}", dest);
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
	char config[2000];
	if (strchr(dir, '/')[strlen(dir)-1] != '/') {
		strcat(dir, "/");
	}
	strcpy(config, dir);
	strcat(config, "feeds");

	if (access(config, F_OK) != 0) {
		log_info("no feeds, nothing to do\n");
		exit(0);
	}

	FILE *feeds;
	if (!(feeds = fopen(config, "r"))) {
		log_err("failed to open feeds file\n");
		exit(1);
	}

	/* setup cache dir for torrent files */
	char cache[2000];
	strcpy(cache, dir);
	strcat(cache, "cache");

	/* parse each line */
	char line[2000];
	char **downloaded;
	while(fgets(line, 2000, feeds) != NULL) {
		/* filter url, destination, filter */
		char url[2000];
		char dest[2000];
		char filter[2000];
		strcpy(url, line);
		strcpy(dest, strchr(line, ';')+1);
		strcpy(filter, strchr(dest, ';')+1);
		strchr(url, ';')[0] = '\0';
		strchr(dest, ';')[0] = '\0';
		strchr(filter, ';')[0] = '\0';

		/* download and add results */
		add_torrents(rss_download(url, cache, filter, downloaded), dest);
		sleep(delay);
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
	while ((c = getopt(argc, argv, "t:c:l:a:d:p:h")) != -1) {
		switch (c) {
			case 't':
				delay = strtoul(optarg, NULL, 0);
				break;
			case 'c':
				if (strlen(optarg) > 1337) { 
					log_err("cmd too long\n");
					exit(1);
				}
				strcpy(cmd, optarg);
				break;
			case 'l':
				if (strlen(optarg) > 2000) { 
					log_err("path too long\n");
					exit(1);
				}
				strcpy(dir, optarg);
				strrplc(dir, "~", home);
				break;
			case 'a':
				if (!strchr(optarg, ';') || strlen(optarg) > 2000) {
					log_err("invalid format\n");
					exit(1);
				}
				strcpy(url, optarg);
				strchr(url, ';')[0] = '\0';
				strcpy(dest, strchr(optarg, ';')+1);
				if (strchr(dest, ';')) {
					strcpy (filter, strchr(dest, ';')+1);
					strchr(dest, ';')[0] = '\0';
				}
				add_rss(url, dest, filter);
				exit(0);
				break;
			case 'd':
				id = strtol(optarg, NULL, 10);
				del_rss(id);
				exit(0);
			case 'p':
				list_rss(optarg ? optarg : "");
				exit(0);
			case 'h':
			default:
				usage();
		}
	}

	/* not doing any action, do the thing */
	handle_feeds();
	return 0;
}
