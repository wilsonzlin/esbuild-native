#pragma once

#include "esb.h"
#include "transform.h"

typedef enum receiver_result_type {
  RECEIVER_RESULT_TYPE_BUILD,
  RECEIVER_RESULT_TYPE_TRANSFORM,
} receiver_result_type;

typedef struct receiver_result {
  receiver_result_type type;
  union {
    transform_result transform_result;
  };
} receiver_result;

void init_js_receiver(napi_env env);
void call_js_receiver(receiver_result* res);
void ref_js_receiver(napi_env env);
