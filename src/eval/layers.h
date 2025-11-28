#include "../consts.h"

template <int inputsize, int outputsize> struct SparseAffine {
  alignas(64) I8 weights[outputbuckets * inputsize * outputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  alignas(64) I32 output[outputsize];
  void load(char* stream);
  I32 *transform(U8 *input, int bucket);
};

template <int inputsize, int outputsize> struct DenseAffine {
  alignas(64) I32 weights[outputbuckets * inputsize * outputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  alignas(64) I32 output[outputsize];
  void load(char* stream);
  I32 *transform(I32 *input, int bucket);
};

template <int inputsize> struct PerspectiveSCReLU {
  alignas(64) I32 weights[outputbuckets * inputsize];
  alignas(64) I32 bias[outputbuckets];
  void load(char* stream);
  I32 transform(I32 *input, int bucket);
};

template <int inputsize> struct PerspectiveCReLU {
  alignas(64) I32 weights[outputbuckets * inputsize];
  alignas(64) I32 bias[outputbuckets];
  void load(char* stream);
  I32 transform(I32 *input, int bucket);
};

template <int inputsize> struct CReLUActivation {
  alignas(64) I32 output[inputsize];
  I32 *transform(I32 *input, I32 Q);
};

template <int inputsize> struct DualActivation {
  alignas(64) I32 output[2 * inputsize];
  I32 *transform(I32 *input, I32 Q);
};