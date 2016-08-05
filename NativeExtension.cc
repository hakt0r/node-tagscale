#include "functions.h"

using v8::FunctionTemplate;

void InitAll(v8::Local<v8::Object> exports) {

  Nan::Set(exports, Nan::New("flushAll").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_flushAll)).ToLocalChecked());

  Nan::Set(exports, Nan::New("closeAll").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_closeAll)).ToLocalChecked());

  XScale::Init(exports);
  XIndex::Init(exports);
  XCursor::Init(exports);

  tagscale_init(); }

NODE_MODULE(NativeExtension, InitAll)
