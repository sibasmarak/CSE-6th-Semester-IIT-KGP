#!/bin/bash
[[ ! -f $1 ]] && echo "File absent" && exit
case $1 in
	*.tar.gz|*.tar|*.tgz|*.tbz2|*.tar.bz2) tar -xvf $1 || echo "Install tar";;
	*.bz2) bzip2 -dkvf $1 || echo "Install bzip2";;
	*.rar) rar x $1 || echo "Install rar";;
	*.gz) gzip -dkvf $1 || echo "Install gzip";;
	*.zip) unzip $1 || echo "Install unzip";;
	*.Z) uncompress -k $1 || echo "Install ncompress";;
	*.7z) 7z x $1 || echo "Install p7zip-full";;
	*) echo "Unknown file format: cannot extract";;
esac