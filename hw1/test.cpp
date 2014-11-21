#include "hmm.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MODEL_NUM 10

static void process(HMM* models, int num_models,
                    const char* data, int len,
                    int* out_model_index, double* out_max_prob);

static void print_usage();

int main(int argc, char* argv[])
{
  HMM models[MAX_MODEL_NUM];

  if (argc != 4)
    print_usage();

  // Initialize from arguments
  int num_models = load_models(argv[1], models, MAX_MODEL_NUM);
  FILE* fin = fopen(argv[2], "r");
  if (!fin) {
    perror(argv[2]);
    exit(1);
  }
  FILE* fout = fopen(argv[3], "w");
  if (!fout) {
    perror(argv[3]);
    exit(1);
  }

  // Process
  char data[MAX_LINE];
  while (fscanf(fin, "%s", data) > 0) {
    int model_index;
    double max_prob;
    process(models, num_models, data, strlen(data),
            &model_index, &max_prob);
    fprintf(fout, "%s %e\n", models[model_index].model_name, max_prob);
  }

  // Clean Up
  fclose(fin);
  fclose(fout);

  return 0;
}

static double _delta[2][MAX_STATE];

static void process(HMM* models, int num_models,
                    const char* data, int len,
                    int* out_model_index, double* out_max_prob)
{
  int model_index = -1;
  double max_prob = 0.0;

  for (int model_i = 0; model_i < num_models; ++model_i) {
    HMM* model = (models + model_i);
    int num_states = model->state_num;
    // Initialize
    {
      int observ = data[0] - 'A';
      for (int i = 0; i < num_states; ++i)
        _delta[0][i] = model->initial[i] * model->observation[observ][i];
    }
    //
    for (int t = 1; t < len; ++t) {
      int observ = data[t] - 'A';
      for (int i = 0; i < num_states; ++i) {
        double max_path_prob = 0.0;
        for (int j = 0; j < num_states; ++j) {
          double tmp = _delta[0][j] * model->transition[j][i];
          if (max_path_prob < tmp)
            max_path_prob = tmp;
        }
        _delta[1][i] = max_path_prob * model->observation[observ][i];
      }
      for (int i = 0; i < num_states; ++i)
        _delta[0][i] = _delta[1][i];
    }
    // 
    {
      double max_path_prob = 0.0;
      for (int i = 0; i < num_states; ++i) {
        if (max_path_prob < _delta[0][i])
          max_path_prob = _delta[0][i];
      }
      if (max_prob < max_path_prob) {
        max_prob = max_path_prob;
        model_index = model_i;
      }
    }
  }

  *out_model_index = model_index;
  *out_max_prob = max_prob;
}

static void print_usage()
{
  static const char* text[] = {
    "test modellist.txt testing_data1.txt result1.txt",
    "test modellist.txt testing_data2.txt result2.txt",
    "",
    "$iter is a integer > 0, which is iteration of Baum-Welch algorithm."
  };
  int i, n;
  n = sizeof(text) / sizeof(char*);
  for(i = 0; i < n; ++i) {
    fprintf(stderr, "%s\n", text[i]);
  }
  exit(0);
}
