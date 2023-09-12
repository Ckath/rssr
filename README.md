# RSSR
minimal (read: sensible for me only) rss torrent utility for torrent clients

## features
- genuinly serviceless, run to update, throw in cron or systemd timer to automate
- optimized for dedicated feeds, delays to not timeout host
- compatible with any torrent client that allows adding .torrent through command

## usage
```
usage: rssr [-t delay] [-c cmd] [-l dir] [-a url;dest;filter] [-d id] [-p c/u/d]
options
-t	delay between feeds (seconds), default 22
-c	torrent add cmd (use {file} and {dest} in it), default transmission-remote -a "{file}" -g "{dest}"
-l	config and cache location, default ~/.config/rssr/
actions
-a	add url;dest to config the ;filter is optional
-d	delete entry matching id
-p	print current feeds, c csv, u urls, d destinations
```
while the feeds file is a basic `;` separated csv. due to the limited parsing flexibility its highly recommended to only manage it with `-a` and `-d`. it will segfault otherwise and I will not fix it.
