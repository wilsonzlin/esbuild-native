/**
 * Helper header file for making N-API easier to use.
 * `nn` doesn't mean anything specific, it's just conveniently short.
 * Including this also provides `node_api.h`.
 */

#pragma once

#include <stdlib.h>
#include <stdio.h>
#define NAPI_VERSION 4
#include <node_api.h>

#define FATAL_EXIT_STATUS 0x58

#define nn_conststr(env, const_str, out) napi_create_string_utf8(env, const_str, sizeof(const_str) - 1, out)

#define nn_fn_prelude(info, n) \
  size_t argc = n; \
  napi_value argv[n]; \
  napi_value arg_this; \
  void* fn_data; \
  \
  nn_ok(napi_get_cb_info(env, info, &argc, argv, &arg_this, &fn_data))

#define nn_get_prop(out_var, env, obj, name) \
  napi_value out_var; \
  if (napi_get_named_property(env, obj, name, &out_var) != napi_ok) { \
    nn_ok(napi_throw_error(env, NULL, "Property \"" name "\" is missing")); \
    goto rollback; \
  }

#define nn_expect_prop_is_typeof(out_var, env, obj, name, typeof) \
  nn_get_prop(out_var, env, obj, name); \
  { \
    napi_valuetype actual; \
    nn_ok(napi_typeof(env, out_var, &actual)); \
    if (actual != typeof) { \
      nn_ok(napi_throw_type_error(env, NULL, "Property \"" name "\" is not the correct type")); \
      goto rollback; \
    } \
  }

#define nn_expect_prop_is(out_var, env, obj, name, pred_fn) \
  nn_get_prop(out_var, env, obj, name); \
  { \
    bool is; \
    nn_ok(pred_fn(env, out_var, &is)); \
    if (!is) { \
      nn_ok(napi_throw_type_error(env, NULL, "Property \"" name "\" is not the correct type")); \
      goto rollback; \
    } \
  }

#define nn_get_prop_as_type(out_var_type, out_var, env, obj, name, type_fn) \
  out_var_type out_var; \
  { \
    nn_get_prop(out_var##_js_value, env, obj, name); \
    if (type_fn(env, out_var##_js_value, &out_var) != napi_ok) { \
      nn_ok(napi_throw_type_error(env, NULL, "Property \"" name "\" is not the correct type")); \
      goto rollback; \
    } \
  }

void* nn_malloc(size_t bytes);
void* nn_calloc(size_t nmemb, size_t size);
void nn_ok(napi_status status);
napi_value nn_undefined(napi_env env);
