#pragma once

#include "esb.h"
#include "nn.h"

_GoString_ copy_into_gostring(napi_env env, napi_value js_val);
void destroy_gostring(_GoString_ gostr);

bool copy_into_ffiapi_gostring_goslice(napi_env env, napi_value js_array, ffiapi_gostring_goslice* out);
void destroy_ffiapi_gostring_goslice(ffiapi_gostring_goslice slice);

bool copy_into_ffiapi_define_array(napi_env env, napi_value js_array, ffiapi_define** out_ary, size_t* out_len);
void destroy_ffiapi_define_array(ffiapi_define* ary, size_t len);

bool copy_into_ffiapi_engine_array(napi_env env, napi_value js_array, ffiapi_engine** out_ary, size_t* out_len);
void destroy_ffiapi_engine_array(ffiapi_engine* ary, size_t len);

napi_value buffer_from_ffiapi_string(napi_env env, ffiapi_string str);
napi_value array_from_ffiapi_message_array(napi_env env, ffiapi_message* arr, size_t len);
