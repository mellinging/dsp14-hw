DSP HW3 Report
==============

R02944024 David Lin

## Environment

- OS: Linux 3.17.6-1
- CPU: x86-64

## Generate ZhuYin-Big5.map

Please read `Makefile` and `build_ZhuYinTable.py`

    make ZhuYin-Big5.map

## Compile

    make

## Execute

    ./my_disambig bigram.lm ZhuYin-Big5.map input.txt > output.txt

## What I have done

- Train language model with SRILM
- Convert Big5-ZhuYin.map to ZhuYin-Big5.map with Python Language
- Decode ZhuYin sentences with Bigram Viterbi Algorithm

