#include "hmm.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static void iteration_init(const HMM* model);
static void iteration_per_sample(const HMM* model, const char* sample);
static void iteration_update(HMM* model, int num_samples);
static void print_usage();

int main(int argc, char* argv[])
{
  HMM model;
  int num_iterations;

  if (argc != 5)
    print_usage();

  // Initialize from arguments
  num_iterations = strtol(argv[1], NULL, 10);
  loadHMM(&model, argv[2]);
  // argv[3] will be used later
  // argv[4] will be used later

  // Process
  {
    char sample[MAX_LINE];
    int nsamples;

    FILE* fin = fopen(argv[3], "r");
    if (!fin) {
      perror(argv[3]);
      exit(1);
    }

    for (int i = 0; i < num_iterations; ++i) {
      fseek(fin, 0, SEEK_SET);
      iteration_init(&model);
      for (nsamples = 0; fscanf(fin, "%s", sample) > 0; ++nsamples) {
        iteration_per_sample(&model, sample);
      }
      iteration_update(&model, nsamples);
      // dumpHMM(stderr, &model);
    }

    fclose(fin);
  }

  // Generate Output
  {
    FILE* fout = fopen(argv[4], "w");
    if (!fout) {
      perror(argv[4]);
      exit(1);
    }

    dumpHMM(fout, &model);

    fclose(fout);
  }

  return 0;
}

static double _alpha[MAX_SEQ][MAX_STATE];
static double _beta[MAX_SEQ][MAX_STATE];
static double _gamma[MAX_SEQ][MAX_STATE];
static double _epsilon[MAX_SEQ][MAX_STATE][MAX_STATE];
static double _update_pi[MAX_STATE];
static double _sum_gamma[MAX_STATE][MAX_OBSERV];
static double _sum_gamma_tail[MAX_STATE];
static double _sum_epsilon[MAX_STATE][MAX_STATE];

static void _iteration_init_sample(const HMM* model, const char* sample, int len)
{
  int num_states = model->state_num;

  for (int t = 0; t < len; ++t) {
    for (int i = 0; i < num_states; ++i) {
      for (int j = 0; j < num_states; ++j)
        _epsilon[t][i][j] = 0;
      _alpha[t][i] = _beta[t][i] = _gamma[t][i] = 0;
    }
  }
}

static void _iteration_alpha(const HMM* model, const char* sample, int len)
{
  int num_states = model->state_num;

  // Initialize
  {
    int observ = sample[0] - 'A';
    for (int i = 0; i < num_states; ++i)
      _alpha[0][i] = model->initial[i] * model->observation[observ][i];
  }

  // Induction
  for (int t = 1; t < len; ++t) {
    int observ = sample[t] - 'A';
    for (int i = 0; i < num_states; ++i) {
      double tmp = 0.0;
      for (int j = 0; j < num_states; ++j)
        tmp += _alpha[t-1][j] * model->transition[j][i];
      _alpha[t][i] = tmp * model->observation[observ][i];
    }
  }
}

static void _iteration_beta(const HMM* model, const char* sample, int len)
{
  int num_states = model->state_num;

  // Initialize
  for (int i = 0; i < num_states; ++i)
    _beta[len-1][i] = 1;

  // Induction
  for (int t = len - 2; t >= 0; --t) {
    int observ = sample[t+1] - 'A';
    for (int i = 0; i < num_states; ++i)
      for (int j = 0; j < num_states; ++j) {
        _beta[t][i] += model->transition[i][j] *
                       model->observation[observ][j] *
                       _beta[t+1][j];
      }
  }
}

static void _iteration_eps_gamma(const HMM* model, const char* sample, int len)
{
  int num_states = model->state_num;

  for (int t = 0; t < len; ++t) {
    double tmp = 0.0;
    for (int i = 0; i < num_states; ++i) {
      tmp += (_gamma[t][i] = _alpha[t][i] * _beta[t][i]);
    }
    for (int i = 0; i < num_states; ++i)
      _gamma[t][i] /= tmp;
  }

  for (int t = 0; t < len-1; ++t) {
    int observ = sample[t+1] - 'A';
    double tmp = 0.0;
    for (int i = 0; i < num_states; ++i)
      for (int j = 0; j < num_states; ++j) {
        _epsilon[t][i][j] = _alpha[t][i] *
                            model->transition[i][j] *
                            model->observation[observ][j] *
                            _beta[t+1][j];
        tmp += _epsilon[t][i][j];
      }
    for (int i = 0; i < num_states; ++i)
      for (int j = 0; j < num_states; ++j)
        _epsilon[t][i][j] /= tmp;
  }

  for (int i = 0; i < num_states; ++i)
    _update_pi[i] += _gamma[0][i];

  for (int i = 0; i < num_states; ++i) {
    for (int t = 0; t < len; ++t) {
      int observ = sample[t] - 'A';
      _sum_gamma[i][observ] += _gamma[t][i];
    }
    _sum_gamma_tail[i] += _gamma[len-1][i];
  }

  for (int i = 0; i < num_states; ++i)
    for (int j = 0; j < num_states; ++j) {
      double tmp = 0.0;
      for (int t = 0; t < len-1; ++t)
        tmp += _epsilon[t][i][j];
      _sum_epsilon[i][j] += tmp;
    }
}

static void iteration_init(const HMM* model)
{
  int num_states = model->state_num,
      num_observation = model->observ_num;

  for (int i = 0; i < num_states; ++i) {
    _update_pi[i] = _sum_gamma_tail[i] = 0;
    for (int j = 0; j < num_states; ++j)
      _sum_epsilon[i][j] = 0;
    for (int j = 0; j < num_observation; ++j)
      _sum_gamma[i][j] = 0;
  }
}

static void iteration_per_sample(const HMM* model, const char* sample)
{
  int length = strlen(sample),
      num_states = model->state_num;
  // Initialize
  _iteration_init_sample(model, sample, length);
  // Compute Alpha (Forward Probabilities)
  _iteration_alpha(model, sample, length);
  // Compute Beta (Backward Probabilities)
  _iteration_beta(model, sample, length);
  // Find epsilon and gamma from alpha and beta
  _iteration_eps_gamma(model, sample, length);
}

static void iteration_update(HMM* model, int num_samples)
{
  int num_states = model->state_num,
      num_observations = model->observ_num;

  for (int i = 0; i < num_states; ++i)
    model->initial[i] = _update_pi[i] / num_samples;

  for (int i = 0; i < num_states; ++i) {
    double tmp = 0.0;
    for (int j = 0; j < num_observations; ++j)
      tmp += _sum_gamma[i][j];
    tmp -= _sum_gamma_tail[i];
    for (int j = 0; j < num_states; ++j)
      model->transition[i][j] = _sum_epsilon[i][j] / tmp;
  }

  for (int i = 0; i < num_states; ++i) {
    double tmp = 0.0;
    for (int j = 0; j < num_observations; ++j)
      tmp += _sum_gamma[i][j];
    for (int j = 0; j < num_observations; ++j)
      model->observation[j][i] = _sum_gamma[i][j] / tmp;
  }
}

static void print_usage()
{
  static const char* text[] = {
    "train $iter model_init.txt seq_model_01.txt model_01.txt",
    "train $iter model_init.txt seq_model_02.txt model_02.txt",
    "train $iter model_init.txt seq_model_03.txt model_03.txt",
    "train $iter model_init.txt seq_model_04.txt model_04.txt",
    "train $iter model_init.txt seq_model_05.txt model_05.txt",
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
