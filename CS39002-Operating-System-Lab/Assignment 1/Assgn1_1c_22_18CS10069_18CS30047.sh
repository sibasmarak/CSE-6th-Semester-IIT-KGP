#!/bin/bash
[[ $# -ne 2 ]] && echo "Two arguments needed" && exit
[[ ! -f $1 ]] && echo "File absent" && exit
(( $2<1 || $2>4 )) && echo "Column not in 1-4!" && exit
awk -v column=$2 '{print tolower($column)}' $1 | 
sort |
uniq -c | 
awk '{print $2,$1}' | 
sort -k 2 -rno 1c_output_$2_column.freq 