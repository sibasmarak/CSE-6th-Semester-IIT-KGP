#!/bin/bash
[[ ! -d 1.b.files.out ]] && mkdir 1.b.files.out
for file in 1.b.files/*
do
	sort -rn $file > "1.b.files.out/$(basename $file)"
done
sort -rn 1.b.files.out/*.txt > 1.b.out.txt