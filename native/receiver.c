#include <stdlib.h>
#include "nn.h"
#include "receiver.h"

// We need to use a threadsafe_function as the goroutines can call us from any thread.
static napi_threadsafe_function js_receiver;

void js_receiver_handler(
  napi_env env,
  napi_value js_callback,
  void* _ctx,
  void* data_raw
) {
  receiver_result* res = (receiver_result*) data_raw;
  switch (res->type) {
  case RECEIVER_RESULT_TYPE_BUILD:
    // TODO
    break;

  case RECEIVER_RESULT_TYPE_TRANSFORM:
    process_transform_result(env, res->transform_result);
    break;
  }
  free(res);
  nn_ok(napi_unref_threadsafe_function(env, js_receiver));
}

void call_js_receiver(receiver_result* res) {
  nn_ok(napi_call_threadsafe_function(js_receiver, res, napi_tsfn_nonblocking));
}

void init_js_receiver(napi_env env) {
  napi_value js_receiver_desc;
  nn_ok(nn_conststr(env, "esbuild-native JavaScript receiver callback", &js_receiver_desc));

  nn_ok(napi_create_threadsafe_function(
    env,
    // napi_value func.
    NULL,
    // napi_value async_resource.
    NULL,
    // napi_value async_resource_name.
    js_receiver_desc,
    // size_t max_queue_size.
    0,
    // size_t initial_thread_count.
    1,
    // void* thread_finalize_data.
    NULL,
    // napi_finalize thread_finalize_cb.
    NULL,
    // void* context.
    NULL,
    // napi_threadsafe_function_call_js call_js_cb.
    &js_receiver_handler,
    // napi_threadsafe_function* result.
    &js_receiver
  ));
  nn_ok(napi_unref_threadsafe_function(env, js_receiver));
}

void ref_js_receiver(napi_env env) {
  nn_ok(napi_ref_threadsafe_function(env, js_receiver));
}
