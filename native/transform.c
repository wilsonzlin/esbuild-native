#include "common.h"
#include "esb.h"
#include "receiver.h"
#include "transform.h"

// This is passed to Go and called from a goroutine.
void transform_go_callback(
  void* cb_data,
	ffiapi_string js,
	ffiapi_string js_source_map,
	ffiapi_message* errors,
	size_t errors_len,
	ffiapi_message* warnings,
	size_t warnings_len
) {
  receiver_result* res = (receiver_result*) cb_data;
  res->transform_result.js = js;
  res->transform_result.js_source_map = js_source_map;
  res->transform_result.errors = errors;
  res->transform_result.errors_len = errors_len;
  res->transform_result.warnings = warnings;
  res->transform_result.warnings_len = warnings_len;
  call_js_receiver(res);
}

// Called from `js_receiver_handler`.
void process_transform_result(napi_env env, transform_result res) {
  napi_value undefined = nn_undefined(env);

  napi_value res_obj = undefined;

  nn_ok(napi_create_object(env, &res_obj));

  napi_value js_buffer = buffer_from_ffiapi_string(env, res.js);
  nn_ok(napi_set_named_property(env, res_obj, "js", js_buffer));

  napi_value js_source_map_buffer = buffer_from_ffiapi_string(env, res.js_source_map);
  nn_ok(napi_set_named_property(env, res_obj, "jsSourceMap", js_source_map_buffer));

  napi_value js_errors = array_from_ffiapi_message_array(env, res.errors, res.errors_len);
  nn_ok(napi_set_named_property(env, res_obj, "errors", js_errors));

  napi_value js_warnings = array_from_ffiapi_message_array(env, res.warnings, res.warnings_len);
  nn_ok(napi_set_named_property(env, res_obj, "warnings", js_warnings));

  // Cleanup: release source buffer.
  nn_ok(napi_delete_reference(env, res.src_buffer_ref));

  nn_ok(napi_resolve_deferred(env, res.deferred, res_obj));
}

void destroy_ffiapi_transform_options(ffiapi_transform_options* opt) {
  destroy_ffiapi_engine_array(opt->engines, opt->engines_len);
  destroy_gostring(opt->jsx_factory);
  destroy_gostring(opt->jsx_fragment);
  destroy_ffiapi_define_array(opt->defines, opt->defines_len);
  destroy_ffiapi_gostring_goslice(opt->pure_functions);
  free(opt);
}

// Called by Node.js when built options JS external value returned from
// `node_method_create_transform_options` is ready to be garbage collected.
void transform_options_finalizer(napi_env env, void* finalize_data, void* _finalize_hint) {
  destroy_ffiapi_transform_options(finalize_data);
}

napi_value node_method_create_transform_options(napi_env env, napi_callback_info info) {
  napi_value undefined = nn_undefined(env);

  ffiapi_transform_options* options = NULL;

  nn_fn_prelude(info, 1);
  napi_value arg_opt_obj = argv[0];
  napi_value res_options = undefined;

  // Use zero memory to allow destructor to work safely for rollback.
  options = nn_calloc(1, sizeof(ffiapi_transform_options));
  nn_get_prop_as_type(int32_t, source_map, env, arg_opt_obj, "sourceMap", napi_get_value_int32);
  options->source_map = source_map;
  nn_get_prop_as_type(int32_t, target, env, arg_opt_obj, "target", napi_get_value_int32);
  options->target = target;

  nn_expect_prop_is(jsval_engines, env, arg_opt_obj, "engines", napi_is_array);
  if (!copy_into_ffiapi_engine_array(env, jsval_engines, &options->engines, &options->engines_len)) {
    goto rollback;
  }

  nn_get_prop_as_type(bool, strict_nullish_coalescing, env, arg_opt_obj, "strictNullishCoalescing", napi_get_value_bool);
  options->strict_nullish_coalescing = strict_nullish_coalescing;
  nn_get_prop_as_type(bool, strict_class_fields, env, arg_opt_obj, "strictClassFields", napi_get_value_bool);
  options->strict_class_fields = strict_class_fields;

  nn_get_prop_as_type(bool, minify_whitespace, env, arg_opt_obj, "minifyWhitespace", napi_get_value_bool);
  options->minify_whitespace = minify_whitespace;
  nn_get_prop_as_type(bool, minify_identifiers, env, arg_opt_obj, "minifyIdentifiers", napi_get_value_bool);
  options->minify_identifiers = minify_identifiers;
  nn_get_prop_as_type(bool, minify_syntax, env, arg_opt_obj, "minifySyntax", napi_get_value_bool);
  options->minify_syntax = minify_syntax;

  nn_expect_prop_is_typeof(jsval_jsx_factory, env, arg_opt_obj, "jsxFactory", napi_string);
  options->jsx_factory = copy_into_gostring(env, jsval_jsx_factory);
  nn_expect_prop_is_typeof(jsval_jsx_fragment, env, arg_opt_obj, "jsxFragment", napi_string);
  options->jsx_fragment = copy_into_gostring(env, jsval_jsx_fragment);

  nn_expect_prop_is(jsval_defines, env, arg_opt_obj, "defines", napi_is_array);
  if (!copy_into_ffiapi_define_array(env, jsval_defines, &options->defines, &options->defines_len)) {
    goto rollback;
  }

  nn_expect_prop_is(jsval_pure_functions, env, arg_opt_obj, "pureFunctions", napi_is_array);
  if (!copy_into_ffiapi_gostring_goslice(env, jsval_pure_functions, &options->pure_functions)) {
    goto rollback;
  }

  nn_expect_prop_is_typeof(jsval_source_file, env, arg_opt_obj, "sourceFile", napi_string);
  options->source_file = copy_into_gostring(env, jsval_source_file);

  nn_get_prop_as_type(int32_t, loader, env, arg_opt_obj, "loader", napi_get_value_int32);
  options->loader = loader;

  nn_ok(napi_create_external(env, (void*) options, transform_options_finalizer, NULL, &res_options));

  goto cleanup;

rollback:
  destroy_ffiapi_transform_options(options);

cleanup:

  return res_options;
}

napi_value node_method_transform(napi_env env, napi_callback_info info) {
  napi_value undefined = nn_undefined(env);

  bool src_buffer_ref_set = false;
  napi_ref src_buffer_ref;
  bool promise_created = false;
  napi_deferred deferred;
  receiver_result* result = NULL;

  nn_fn_prelude(info, 2);
  napi_value arg_buffer = argv[0];
  napi_value arg_options = argv[1];
  napi_value res_promise = undefined;

  bool arg_buffer_is_buffer;
  nn_ok(napi_is_buffer(env, arg_buffer, &arg_buffer_is_buffer));
  if (!arg_buffer_is_buffer) {
    nn_ok(napi_throw_type_error(env, NULL, "First argument is not a buffer"));
    goto cleanup;
  }

  // Get pointer to bytes in buffer.
  void* buffer_data;
  size_t buffer_len;
  // If Buffer is empty, Node.js will provide NULL, so do not assert.
  if (napi_get_buffer_info(env, arg_buffer, &buffer_data, &buffer_len) != napi_ok || buffer_data == NULL) {
    nn_ok(napi_throw_type_error(env, NULL, "Failed to read source buffer"));
    goto rollback;
  }

  // Ensure buffer lives long enough until minification has finished.
  nn_ok(napi_create_reference(env, arg_buffer, 1, &src_buffer_ref));
  src_buffer_ref_set = true;

  void* options_raw_ptr;
  if (napi_get_value_external(env, arg_options, &options_raw_ptr) != napi_ok || options_raw_ptr == NULL) {
    nn_ok(napi_throw_type_error(env, NULL, "Options argument invalid"));
    goto rollback;
  }

  nn_ok(napi_create_promise(env, &deferred, &res_promise));
  promise_created = true;

  GoString src_code = {
    .p = (char const*) buffer_data,
    .n = buffer_len,
  };

  result = nn_malloc(sizeof(receiver_result));
  result->type = RECEIVER_RESULT_TYPE_TRANSFORM;
  result->transform_result.deferred = deferred;
  result->transform_result.src_buffer_ref = src_buffer_ref;

  ref_js_receiver(env);
  GoTransform(
    nn_malloc,
    transform_go_callback,
    (void*) result,
    src_code,
    (ffiapi_transform_options*) options_raw_ptr
  );

  goto cleanup;

rollback:
  if (src_buffer_ref_set) {
    // Release source buffer.
    nn_ok(napi_delete_reference(env, src_buffer_ref));
  }
  if (promise_created) {
    nn_ok(napi_reject_deferred(env, deferred, undefined));
  }
  free(result);

cleanup:

  return res_promise;
}
