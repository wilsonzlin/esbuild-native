#include "common.h"
#include "esb.h"
#include "nn.h"

// Copies a JavaScript string into a heap-allocated UTF-8 GoString.
_GoString_ copy_into_gostring(napi_env env, napi_value js_val) {
  size_t len;
  nn_ok(napi_get_value_string_utf8(env, js_val, NULL, 0, &len));
  void* buf;
  if (len == 0) {
    buf = NULL;
  } else {
    buf = nn_malloc(len + 1);
    nn_ok(napi_get_value_string_utf8(env, js_val, buf, len + 1, NULL));
  }
  _GoString_ gostr = {
    .n = len,
    .p = buf,
  };
  return gostr;
}

void destroy_gostring(_GoString_ gostr) {
  free(gostr.p);
  gostr.p = NULL;
}

void destroy_ffiapi_gostring_goslice(ffiapi_gostring_goslice slice) {
  for (ptrdiff_t i = 0; i < slice.len; i++) {
    destroy_gostring(slice.data[i]);
  }
  free(slice.data);
  slice.data = NULL;
}

// If false is returned, a JS error was thrown.
bool copy_into_ffiapi_gostring_goslice(napi_env env, napi_value js_array, ffiapi_gostring_goslice* out) {
  bool res_success = true;

  uint32_t len;
  nn_ok(napi_get_array_length(env, js_array, &len));

  out->data = nn_malloc(sizeof(_GoString_) * len);
  out->cap = len;
  // We will use out->len to track how many GoString elements we've created, in case we need to rollback before we've created all of them.
  for (out->len = 0; out->len < len; out->len++) {
    napi_value js_elem;
    nn_ok(napi_get_element(env, js_array, out->len, &js_elem));

    napi_valuetype js_elem_type;
    nn_ok(napi_typeof(env, js_elem, &js_elem_type));
    if (js_elem_type != napi_string) {
      nn_ok(napi_throw_type_error(env, NULL, "Non-string value found in string array"));
      goto rollback;
    }
    out->data[out->len] = copy_into_gostring(env, js_elem);
  }

  goto cleanup;

rollback:
  res_success = false;
  destroy_ffiapi_gostring_goslice(*out);

cleanup:

  return res_success;
}

void destroy_ffiapi_define_array(ffiapi_define* ary, size_t len) {
  for (size_t i = 0; i < len; i++) {
    destroy_gostring(ary[i].name);
    destroy_gostring(ary[i].value);
  }
  free(ary);
}

// If false is returned, a JS error was thrown.
bool copy_into_ffiapi_define_array(napi_env env, napi_value js_array, ffiapi_define** out_ary, size_t* out_len) {
  bool res_success = true;
  ffiapi_define* ary = NULL;

  uint32_t len;
  nn_ok(napi_get_array_length(env, js_array, &len));

  // Use zero memory so that freeing in rollback is free(NULL) for last allocated element.
  ary = nn_calloc(len, sizeof(ffiapi_define));
  uint32_t i = 0;
  for ( ; i < len; i++) {
    napi_value js_elem;
    nn_ok(napi_get_element(env, js_array, i, &js_elem));

    nn_expect_prop_is_typeof(jsval_name, env, js_elem, "name", napi_string);
    ary[i].name = copy_into_gostring(env, jsval_name);

    nn_expect_prop_is_typeof(jsval_value, env, js_elem, "value", napi_string);
    ary[i].value = copy_into_gostring(env, jsval_value);
  }

  *out_ary = ary;
  *out_len = (size_t) len;
  goto cleanup;

rollback:
  res_success = false;
  // If we didn't complete the array, the last element might have partial data that needs to be cleaned up.
  destroy_ffiapi_define_array(ary, i + (i < len));

cleanup:

  return res_success;
}

void destroy_ffiapi_engine_array(ffiapi_engine* ary, size_t len) {
  for (size_t i = 0; i < len; i++) {
    destroy_gostring(ary[i].version);
  }
  free(ary);
}

// If false is returned, a JS error was thrown.
bool copy_into_ffiapi_engine_array(napi_env env, napi_value js_array, ffiapi_engine** out_ary, size_t* out_len) {
  bool res_success = true;
  ffiapi_engine* ary = NULL;

  uint32_t len;
  nn_ok(napi_get_array_length(env, js_array, &len));

  // Use zero memory so that freeing in rollback is free(NULL) for last allocated element.
  ary = nn_calloc(len, sizeof(ffiapi_engine));
  uint32_t i = 0;
  for ( ; i < len; i++) {
    napi_value js_elem;
    nn_ok(napi_get_element(env, js_array, i, &js_elem));

    nn_get_prop_as_type(int32_t, name, env, js_elem, "name", napi_get_value_int32);
    ary[i].name = name;

    nn_expect_prop_is_typeof(jsval_version, env, js_elem, "version", napi_string);
    ary[i].version = copy_into_gostring(env, jsval_version);
  }

  *out_ary = ary;
  *out_len = (size_t) len;
  goto cleanup;

rollback:
  res_success = false;
  // If we didn't complete the array, the last element might have partial data that needs to be cleaned up.
  destroy_ffiapi_engine_array(ary, i + (i < len));

cleanup:

  return res_success;
}

void buffer_from_ffiapi_string_finalizer(napi_env env, void* finalize_data, void* finalize_hint) {
  free(finalize_data);
}

// This takes ownership of `str`.
napi_value buffer_from_ffiapi_string(napi_env env, ffiapi_string str) {
  napi_value res_buffer;
  nn_ok(napi_create_external_buffer(env, str.len, str.data, buffer_from_ffiapi_string_finalizer, NULL, &res_buffer));
  return res_buffer;
}

// This takes ownership of `arr`.
napi_value array_from_ffiapi_message_array(napi_env env, ffiapi_message* arr, size_t len) {
  napi_value res_array;
  nn_ok(napi_create_array_with_length(env, len, &res_array));
  for (size_t i = 0; i < len; i++) {
    ffiapi_message elem = arr[i];

    napi_value js_elem;
    nn_ok(napi_create_object(env, &js_elem));
    nn_ok(napi_set_element(env, res_array, i, js_elem));

    napi_value js_prop_file;
    nn_ok(napi_create_string_utf8(env, elem.file.data, elem.file.len, &js_prop_file));
    nn_ok(napi_set_named_property(env, js_elem, "file", js_prop_file));
    free(elem.file.data);

    napi_value js_prop_line;
    nn_ok(napi_create_int64(env, elem.line, &js_prop_line));
    nn_ok(napi_set_named_property(env, js_elem, "line", js_prop_line));

    napi_value js_prop_column;
    nn_ok(napi_create_int64(env, elem.column, &js_prop_column));
    nn_ok(napi_set_named_property(env, js_elem, "column", js_prop_column));

    napi_value js_prop_length;
    nn_ok(napi_create_int64(env, elem.length, &js_prop_length));
    nn_ok(napi_set_named_property(env, js_elem, "length", js_prop_length));

    napi_value js_prop_text;
    nn_ok(napi_create_string_utf8(env, elem.text.data, elem.text.len, &js_prop_text));
    nn_ok(napi_set_named_property(env, js_elem, "text", js_prop_file));
    free(elem.text.data);
  }
  free(arr);
  return res_array;
}
