#include "functions.h"

using v8::FunctionTemplate;

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New("tags_open").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_tags_open)).ToLocalChecked());
  Nan::Set(target, Nan::New("tags_close").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_tags_close)).ToLocalChecked());
  Nan::Set(target, Nan::New("tags_set").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_tags_set)).ToLocalChecked());
  Nan::Set(target, Nan::New("tags_get").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_tags_get)).ToLocalChecked());
  Nan::Set(target, Nan::New("tags_del").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_tags_del)).ToLocalChecked());
  Nan::Set(target, Nan::New("table_open").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_table_open)).ToLocalChecked());
  Nan::Set(target, Nan::New("table_close").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_table_close)).ToLocalChecked());
  Nan::Set(target, Nan::New("table_get").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_table_get)).ToLocalChecked());
  Nan::Set(target, Nan::New("table_set").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_table_set)).ToLocalChecked());
  Nan::Set(target, Nan::New("table_del").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_table_del)).ToLocalChecked());
  Nan::Set(target, Nan::New("find").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_find)).ToLocalChecked());
  Nan::Set(target, Nan::New("next").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_next)).ToLocalChecked());
  Nan::Set(target, Nan::New("flushAll").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_flushAll)).ToLocalChecked());
  Nan::Set(target, Nan::New("closeAll").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(upb_closeAll)).ToLocalChecked());
  tagscale_init(); }

NODE_MODULE(NativeExtension, InitAll)
