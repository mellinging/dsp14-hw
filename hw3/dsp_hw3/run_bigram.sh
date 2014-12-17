#!/bin/bash
DISAMBIG=${SRILM_BIN:-/opt/srilm/bin/linux}/disambig
perl separator_big5.pl $1 | \
$DISAMBIG -text /dev/stdin -map ZhuYin-Big5.map -lm bigram.lm -order 2
