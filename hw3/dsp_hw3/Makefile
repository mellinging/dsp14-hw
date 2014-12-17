#
#
#

SRILM_BIN ?= /opt/srilm/bin/linux

all: ZhuYin-Big5.map bigram.lm trigram.lm

corpus_seg.txt: corpus.txt
	perl separator_big5.pl $^ > $@

lm.2.cnt: corpus_seg.txt
	$(SRILM_BIN)/ngram-count -text $^ -write $@ -order 2

lm.3.cnt: corpus_seg.txt
	$(SRILM_BIN)/ngram-count -text $^ -write $@ -order 3

bigram.lm: lm.2.cnt
	$(SRILM_BIN)/ngram-count -read $^ -lm $@ -unk -order 2

trigram.lm: lm.3.cnt
	$(SRILM_BIN)/ngram-count -read $^ -lm $@ -unk -order 3

ZhuYin-Big5.map: Big5-ZhuYin.map
	python build_ZhuYinTable.py $^ | iconv -f UTF-8 -t BIG5 > $@



clean:
	$(RM) -f corpus_seg.txt
	$(RM) -f lm.2.cnt lm.3.cnt
	$(RM) -f bigram.lm trigram.lm
	$(RM) -f ZhuYin-Big5.map

.PHONY: all clean
