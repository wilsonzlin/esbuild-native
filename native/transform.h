#pragma once

#include "nn.h"

typedef struct transform_result {
  napi_deferred deferred;
  napi_ref src_buffer_ref;

  // These fields are only set after a goroutine calls `transform_go_callback`.
  // We define them here to avoid more allocations.
  ffiapi_string js;
	ffiapi_string js_source_map;
	ffiapi_message* errors;
	size_t errors_len;
	ffiapi_message* warnings;
	size_t warnings_len;
} transform_result;

void process_transform_result(napi_env env, transform_result res);

napi_value node_method_create_transform_options(napi_env env, napi_callback_info info);
napi_value node_method_transform(napi_env env, napi_callback_info info);
