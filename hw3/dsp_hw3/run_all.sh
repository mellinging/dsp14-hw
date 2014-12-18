#!/bin/bash

DISAMBIG=${SRILM_BIN:-/opt/srilm/bin/linux}/disambig

for i in `seq 1 10`; do
  perl separator_big5.pl testdata/$i.txt | $DISAMBIG -text /dev/stdin -map ZhuYin-Big5.map -lm bigram.lm -order 2 > Submit_Files/result1/$i.txt
done

for i in `seq 1 10`; do
  ./my_disambig bigram.lm ZhuYin-Big5.map testdata/$i.txt > Submit_Files/result2/$i.txt
done

