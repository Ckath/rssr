# RSSR
minimal (read: sensible for me only) rss torrent utility for torrent clients

## features
- genuinely serviceless, run to update, throw in cron or systemd timer to automate
- optimized for dedicated feeds, delays to not hit ratelimits
- compatible with any torrent client that allows adding .torrent through command
- works for me

## install
```
make install
```
besides having a reasonable OS, libcurl should be the only dep

## usage
```
usage: rssr [-t delay] [-c cmd] [-l dir] [-a url -w dest -f filter] [-d id] [-p c/u/d]
options
-t	delay between requests (seconds), default 22
-c	torrent add cmd (use {file} and {dest} in it), default transmission-remote -a "{file}" -w "{dest}"
-l	config and cache location, default ~/.config/rssr/
actions
-a	add "url" -w "dest" to config the -f filter is optional
-d	delete entry matching id
-p	print current feeds in format, c csv, u urls, d destinations
```

the rss parser is minimal and meant to work with most rss feeds, there doesnt seem to be a totally agreed format but it should be able to get the info from most. if it doesnt make an issue I guess.

basic usage will be very simple:
```
# add feed
rssr -a "https://url.com/rssfeed" -w "/mnt/drive/myquestionablemedia/qualityshow"
# grab and add every (new) torrent found in feed
rssr
# or automate it
ln -s $(which rssr) /etc/cron.hourly/
# see feeds with ids
rssr -p c
# delete feed at position 0
rssr -d 0
# if for some reason you need torrents to be readded
rm -rf ~/.config/rssr/cache
```

and thats all there is, use the other flags as mentioned in usage to overwrite default config. any further torrent client specific integration is out of the scope for this utility, its only job is to watch feeds and add new torrents.

## reviews
> You should parse XML with a library. You generally shouldn't write parsers on your own, those take a lifetime to get right, specially for something as awful as XML. A malicious server could pwn your parser.

\- Bernd
> Your string handling seems to be full of bugs, poor practices, and very low performance in general. There's lots of unnecessary copying, you unnecessarily use hardcoded 2000 byte buffers with 2000 as a magic number everywhere, and often there is no reason to make the buffer 2000 bytes long either. The is also more complicated than it needs to be.

\- Bernd
