#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <stdint.h>
#include <locale.h>
#include <ctype.h>
#include "Ngram.h"

//#define DEBUG

static Vocab Vocabulary;

static void init_LanguageModel(const char* filename, int order);
static double getWordProb(VocabIndex wid1, VocabIndex wid2);
static double getWordProb(VocabIndex wid1, VocabIndex wid2, VocabIndex wid3);
static VocabIndex findWord(const char* w);

static void viterbi_init(int num_words, const uint16_t* words);
static void viterbi_iterate(int num_words, const uint16_t* words);
static std::vector<uint16_t> viterbi_backtrack();

inline uint16_t makeBig5(uint8_t first, uint8_t second)
{
  return (((uint16_t)first) << 8 | ((uint16_t)second));
}

inline uint16_t makeBig5(const char* s)
{
  return makeBig5(s[0], s[1]);
}

inline VocabIndex findWordBig5(uint16_t big5)
{
  uint8_t str[3] = {0};
  str[0] = (uint8_t) (big5 >> 8);
  str[1] = (uint8_t) (big5 & 0xFF);
  return findWord((const char*)str);
}

// usage: my_disambig LM ORDER
int main(int argc, char *argv[])
{
  fprintf(stderr, "Locale = %s\n", setlocale(LC_ALL, "zh_TW.BIG5"));

  const char* lm_filename = argv[1];
  int ngram_order = 2; //strtol(argv[2], NULL, 10);
  const char* bopomofo_filename = argv[2];
  const char* input_filename = argv[3];

  init_LanguageModel(lm_filename, ngram_order);

  std::map<uint16_t, std::vector<uint16_t> > BopomofoMap;

  // Build Bopomofo Lookup Table
  std::ifstream bopomofo_file(bopomofo_filename);
  for (std::string line; std::getline(bopomofo_file, line);) {
    const char* ptr = line.c_str();

    uint16_t key = makeBig5(ptr[0], ptr[1]);
    std::vector<uint16_t> value;
    for (const char* p = ptr+3; *p; ) {
      uint16_t v = makeBig5(p[0], p[1]);
      value.push_back(v);
      p += 2;
      while (isspace(*p)) ++p;
    }
    BopomofoMap[key] = value;
    //std::cout << line << std::endl;
  }
  bopomofo_file.close();

  // test
#if false
  {
    uint16_t key = makeBig5("ㄛ");
    std::vector<uint16_t> value = BopomofoMap[key];
    for (int i = 0, n = value.size(); i < n; ++i) {
      uint8_t v[3] = { 0 };
      v[0] = value[i] >> 8;
      v[1] = value[i];
      std::cout << (const char*)v << '\n';
    }
  }
#endif
#if false
  {
    VocabIndex w1a = findWord("兩");
    VocabIndex w1b = findWord("擄");
    VocabIndex w2 = findWord("人");
    std::cerr << "Prob(兩人) = " << getWordProb(w1a, w2) << "\n"
              << "Prob(擄人) = " << getWordProb(w1b, w2)
              << std::endl;
  }
#endif

  std::ifstream input_file(input_filename);

  // Perform Viterbi Algorithm
  for (std::string line; std::getline(input_file, line);) {
    const char* ptr = line.c_str();
    bool first_time = true;

#ifdef DEBUG
    std::cerr << "(LOG) INPUT: " << line << std::endl;
#endif

    while (isspace(*ptr)) ++ptr; // skip leading spaces
    for (const char* p = ptr; *p;) {
      uint16_t key = makeBig5(p);
      std::vector<uint16_t>& words = BopomofoMap[key];

      if (first_time) {
        viterbi_init(words.size(), &words.front());
        first_time = false;
      } else {
        viterbi_iterate(words.size(), &words.front());
      }

      p += 2; // next word
      while (isspace(*p)) ++p; // skip spaces
    }

    std::vector<uint16_t> word_list = viterbi_backtrack();
    std::cout << "<s> ";
    for (int i = 0, n = word_list.size(); i < n; ++i) {
      uint16_t w = word_list[i];
      uint8_t s[3] = {0};
      s[0] = (w >> 8);
      s[1] = (w & 0xFF);
      std::cout << (const char*)s << " ";
    }
    std::cout << "</s>" << std::endl;

#ifdef DEBUG
    std::cerr << "(LOG) Press ENTER to continue..." << std::endl;
    std::getline(std::cin, line);
#endif
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
// Language Model
static Ngram *LangModel = NULL;

//
static void init_LanguageModel(const char* filename, int order)
{
  LangModel = new Ngram(Vocabulary, order);

  File lmFile(filename, "r");
  LangModel->read(lmFile);
  lmFile.close();
}

static VocabIndex findWord(const char* w)
{
  VocabIndex wd = Vocabulary.getIndex(w);
  //if (wd == Vocab_None)
  //  wd = Vocabulary.getIndex(Vocab_Unknown);
  return wd;
}

// Get P(w2 | w1) -- bigram
static double getWordProb(VocabIndex wid1, VocabIndex wid2)
{
  VocabIndex context[] = { wid1, Vocab_None };
  return LangModel->wordProb(wid2, context);
}

// Get P(w3 | w1, w2) -- trigram
static double getWordProb(VocabIndex wid1, VocabIndex wid2, VocabIndex wid3)
{
  VocabIndex context[] = { wid2, wid1, Vocab_None };
  return LangModel->wordProb(wid3, context);
}

////////////////////////////////////////////////////////////////////
// Viterbi Algorithm
typedef std::map<uint16_t, double> DeltaMap;
static DeltaMap delta[2];
static std::vector<std::map<uint16_t, uint16_t> > psi;

static void viterbi_init(int num_words, const uint16_t* words)
{
  delta[0].clear();
  delta[1].clear();
  psi.clear();

  for (int i = 0; i < num_words; ++i) {
    delta[0][words[i]] = 0.0; // Log Prob
  }

#ifdef DEBUG
  std::cerr << "(LOG) Init Viterbi with";
  for (int i = 0; i < num_words; ++i) {
    uint16_t w = words[i];
    uint8_t str[3] = { (uint8_t)(w >> 8), (uint8_t)w, 0};
    std::cerr << " " << (const char*)str;
  }
  std::cerr << std::endl;
#endif
}

static void viterbi_iterate(int num_words, const uint16_t* words)
{
  delta[1].clear();
  psi.push_back(std::map<uint16_t, uint16_t>());

  for (int i = 0; i < num_words; ++i) {
    uint16_t word = words[i];
    VocabIndex word_id = findWordBig5(word);
    if (word_id == Vocab_None) {
      word_id = Vocabulary.getIndex(Vocab_Unknown);
      //uint8_t str[3] = {0};
      //str[0] = word >> 8;
      //str[1] = word & 0xFF;
      //std::cerr << "cannot find the word " << (const char*)str << "\n";
      //exit(1);
    }

    double max_prob = -1.0 / 0.0; // negative infinity
    uint16_t max_prob_word = 0;
    {
      DeltaMap::iterator it = delta[0].begin(),
                         it_end = delta[0].end();
      for (; it != it_end; ++it) {
        uint16_t prev_word = it->first;
        VocabIndex prev_word_id = findWordBig5(prev_word);
        double prev_prob = it->second;
        // Log Prob
        LogP p = getWordProb(prev_word_id, word_id);
        if (p == LogP_Zero)
          continue; // skip this prev_word
        double prob = prev_prob + p;

        if (prob > max_prob) {
          max_prob = prob;
          max_prob_word = prev_word;
        }
      }
    }
    if (max_prob_word != 0) {
      delta[1][word] = max_prob;
      psi.back()[word] = max_prob_word;
    } else {
      abort();
    }
  }

  std::swap(delta[0], delta[1]);
}

static std::vector<uint16_t> viterbi_backtrack()
{
  // Find Termination
  double max_prob = -1.0/0.0;
  uint16_t word = 0;
  DeltaMap::iterator it = delta[0].begin(),
                     it_end = delta[0].end();
  for (; it != it_end; ++it) {
    uint16_t wd = it->first;
    double prob = it->second;
    if (prob > max_prob) {
      word = wd, max_prob = prob;
    }
  }

  std::vector<uint16_t> word_list;
  word_list.push_back(word);

  // Backtrack
  while (!psi.empty()) {
    word = psi.back()[word];
    psi.pop_back();
    word_list.push_back(word);
  }

  std::reverse(word_list.begin(), word_list.end());
  return word_list;
}
