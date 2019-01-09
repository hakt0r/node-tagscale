
#include "functions.h"

Napi::Object init_all(Napi::Env env, Napi::Object exports) {
  tagscale_init();
  Tools::Init(env,exports);
  XScale::Init(env,exports);
  XIndex::Init(env,exports);
  XCursor::Init(env,exports);
  return exports; }

NODE_API_MODULE(NativeExtension,init_all)
