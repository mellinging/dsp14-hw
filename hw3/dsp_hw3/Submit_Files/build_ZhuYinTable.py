#!/usr/bin/env python
# -*- coding: utf-8
from argparse import ArgumentParser
import io, subprocess as sp, sys

def parse_args():
    parser = ArgumentParser()
    parser.add_argument('input', help='Input File')
    return parser.parse_args()
ARGS = parse_args()

content=sp.check_output(['iconv','-f','BIG5','-t','UTF-8', ARGS.input])
content = content.decode('utf-8')

bopomofo = dict()

for line in content.split('\n'):
    if len(line) == 0: continue
    #print(line)
    word, pho = line.split(' ')
    pho = pho.split('/')
    bopomofo[word] = word
    for p in pho:
        c = p[0]
        if c not in bopomofo:
            bopomofo[c] = list(word)
        else:
            bopomofo[c].append(word)

for key in bopomofo:
    value = ' '.join(bopomofo[key])
    s = "{}\t{}\n".format(key, value)
    b = s.encode('utf-8')
    #sys.stdout.write(s)
    sys.stdout.buffer.write(b)
