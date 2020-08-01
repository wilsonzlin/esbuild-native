#include "nn.h"

void* nn_malloc(size_t bytes) {
  void* ptr = malloc(bytes);
  if (ptr == NULL) {
    printf("FATAL: malloc failed.\n");
    exit(FATAL_EXIT_STATUS);
  }
  return ptr;
}

void* nn_calloc(size_t nmemb, size_t size) {
  void* ptr = calloc(nmemb, size);
  if (ptr == NULL) {
    printf("FATAL: calloc failed.\n");
    exit(FATAL_EXIT_STATUS);
  }
  return ptr;
}

void nn_ok(napi_status status) {
  if (status != napi_ok) {
    printf("FATAL: N-API call failed.\n");
    exit(FATAL_EXIT_STATUS);
  }
}

napi_value nn_undefined(napi_env env) {
  napi_value undefined;
  nn_ok(napi_get_undefined(env, &undefined));
  return undefined;
}
