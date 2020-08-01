#include <stdlib.h>
#include "esb.h"
#include "nn.h"
#include "receiver.h"
#include "transform.h"

napi_value node_module_init(napi_env env, napi_value exports) {
  init_js_receiver(env);
  napi_property_descriptor props[] = {
    {"createTransformOptions", NULL, node_method_create_transform_options, NULL, NULL, NULL, napi_default, NULL},
    {"transform", NULL, node_method_transform, NULL, NULL, NULL, napi_default, NULL},
  };
  nn_ok(napi_define_properties(env, exports, 2, props));
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, node_module_init)
