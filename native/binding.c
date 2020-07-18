#include <stdlib.h>

#define NAPI_VERSION 4
#include <node_api.h>

#ifdef _MSC_VER
#include <MemoryModule.h>
#include <esbuild.dll.h>
#include <esbuild.h>
#else
#include <libesbuild.h>
#endif

static char const* JS_RECEIVER_DESC = "esbuild-native JavaScript receiver callback";
static char const* ERRMSG_INTERR_CREATE_RES_BUFFER_FAILED = "Failed to create result buffer";

#define napi_conststr(env, const_str, out) napi_create_string_utf8(env, const_str, sizeof(const_str) - 1, out)

struct invocation_data {
  napi_deferred deferred;
  napi_ref src_buffer_ref;
};

struct call_js_receiver_data {
  struct invocation_data* invocation_data;
  void* min_code;
  unsigned long long min_code_len;
};

// We need to use a threadsafe_function as the Goroutines can call us from any thread.
bool js_receiver_created = false;
napi_threadsafe_function js_receiver;

static inline void* assert_malloc(size_t bytes) {
  void* ptr = malloc(bytes);
  if (ptr == NULL) {
    // TODO
  }
  return ptr;
}

static inline void assert_ok(napi_status status) {
  if (status != napi_ok) {
    // TODO
  }
}

static inline napi_value get_undefined(napi_env env) {
  napi_value undefined;
  assert_ok(napi_get_undefined(env, &undefined));
  return undefined;
}

void call_js_receiver(
  napi_env env,
  napi_value js_callback,
  void* _ctx,
  void* data_raw
) {
  napi_value undefined = get_undefined(env);

  napi_value error_msg = undefined;
  napi_value res_buffer = undefined;

  struct call_js_receiver_data* data = (struct call_js_receiver_data*) data_raw;
  struct invocation_data* invocation_data = (struct invocation_data*) data->invocation_data;

  // Create Node.js buffer from minified code in C memory.
  if (napi_create_external_buffer(env, data->min_code_len, data->min_code, NULL, NULL, &res_buffer) != napi_ok) {
      assert_ok(napi_conststr(env, ERRMSG_INTERR_CREATE_RES_BUFFER_FAILED, &error_msg));
      goto finally;
  }

finally:
  // Release source buffer.
  assert_ok(napi_delete_reference(env, invocation_data->src_buffer_ref));

  if (error_msg != undefined) {
    napi_value error = undefined;
    assert_ok(napi_create_error(env, NULL, error_msg, &error));
    assert_ok(napi_reject_deferred(env, invocation_data->deferred, error));
  } else {
    assert_ok(napi_resolve_deferred(env, invocation_data->deferred, res_buffer));
  }

  free(invocation_data);
  free(data);
}

void minify_js_complete_handler(
  void* invocation_data,
  void* min_code,
  unsigned long long min_code_len
) {
  struct call_js_receiver_data* data = assert_malloc(sizeof(struct call_js_receiver_data));
  data->invocation_data = invocation_data;
  data->min_code = min_code;
  data->min_code_len = min_code_len;
  assert_ok(napi_call_threadsafe_function(js_receiver, (void*) data, napi_tsfn_nonblocking));
}

napi_value node_method_start_service(napi_env env, napi_callback_info info) {
  napi_value undefined = get_undefined(env);

  if (js_receiver_created) {
    assert_ok(napi_throw_error(env, "STARTED", "Service already started"));
    return undefined;
  }

  napi_value js_receiver_desc;
  if (napi_conststr(env, JS_RECEIVER_DESC, &js_receiver_desc) != napi_ok) {
    assert_ok(napi_throw_error(env, "INTERR_CREATE_JS_RECEIVER_DESC", "Failed to create JS receiver callback description string"));
    return undefined;
  }

  if (napi_create_threadsafe_function(
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
    &call_js_receiver,
    // napi_threadsafe_function* result.
    &js_receiver
  ) != napi_ok) {
    assert_ok(napi_throw_error(env, "INTERR_CREATE_JS_RECEIVER", "Failed to create JS receiver"));
    return undefined;
  }

  js_receiver_created = true;
  return undefined;
}

napi_value node_method_stop_service(napi_env env, napi_callback_info info) {
  napi_value undefined = get_undefined(env);

  if (!js_receiver_created) {
    assert_ok(napi_throw_error(env, "STOPPED", "Service not started"));
    return undefined;
  }

  if (napi_release_threadsafe_function(js_receiver, napi_tsfn_abort) != napi_ok) {
    assert_ok(napi_throw_error(env, "INTERR_DESTROY_JS_RECEIVER", "Failed to destroy JS receiver"));
    return undefined;
  }

  js_receiver_created = false;
  return undefined;
}

napi_value node_method_minify(napi_env env, napi_callback_info info) {
  napi_value undefined = get_undefined(env);

  size_t argc = 1;
  napi_value argv[1];
  napi_value _this;
  void* _data;

  // Get the arguments.
  if (napi_get_cb_info(env, info, &argc, argv, &_this, &_data) != napi_ok) {
    assert_ok(napi_throw_error(env, "INTERR_GET_CB_INFO", "Failed to get callback info"));
    return undefined;
  }

  // Ensure buffer lives long enough until minification has finished.
  napi_value buffer_arg = argv[0];
  napi_ref buffer_arg_ref;
  if (napi_create_reference(env, buffer_arg, 1, &buffer_arg_ref) != napi_ok) {
    assert_ok(napi_throw_error(env, "INTERR_CREATE_SRC_BUFFER_REF", "Failed to create reference for source buffer"));
    return undefined;
  }

  // Get pointer to bytes in buffer.
  void* buffer_data;
  size_t buffer_len;
  if (napi_get_buffer_info(env, buffer_arg, &buffer_data, &buffer_len) != napi_ok
      || buffer_len == 0 || buffer_data == NULL) {
    assert_ok(napi_throw_type_error(env, "GET_SRC_BUFFER_INFO", "Failed to read source buffer"));
    return undefined;
  }

  napi_deferred deferred;
  napi_value promise;
  if (napi_create_promise(env, &deferred, &promise) != napi_ok) {
    assert_ok(napi_throw_error(env, "INTERR_CREATE_PROMISE", "Failed to create Promise"));
    return undefined;
  }

  GoString buffer_as_gostr = {
    .p = (char const*) buffer_data,
    .n = buffer_len,
  };

  struct invocation_data* invocation_data = assert_malloc(sizeof(struct invocation_data));
  invocation_data->deferred = deferred;
  invocation_data->src_buffer_ref = buffer_arg_ref;

  MinifyJs(
    buffer_as_gostr,
    &minify_js_complete_handler,
    (void*) invocation_data
  );

  return promise;
}

napi_value node_module_init(napi_env env, napi_value exports) {
  napi_property_descriptor props[] = {
    {"minify", NULL, node_method_minify, NULL, NULL, NULL, napi_default, NULL},
    {"startService", NULL, node_method_start_service, NULL, NULL, NULL, napi_default, NULL},
    {"stopService", NULL, node_method_stop_service, NULL, NULL, NULL, napi_default, NULL},
  };
  assert_ok(napi_define_properties(env, exports, 3, props));
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, node_module_init)
