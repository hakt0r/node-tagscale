/*

  * c) 2016 Sebastian Glaser <anx@ulzq.de>

  This file is part of tagscale.

  tagscale is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  tagscale is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with tagscale.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "functions.h"

#define DBNAME_KEYS 1
#define DBNAME_UID  2
#define DBNAME_MORE 3

static inline char *COPY_TO_CHAR(Handle<Value> val) {
  String::Utf8Value utf8(val->ToString());
  int len = utf8.length() + 1;
  char *str = (char *) calloc(sizeof(char), len);
  strncpy(str, *utf8, len);
  return str; }

static int string_compare (
  ups_db_t *db, const uint8_t *lhs, uint32_t lhs_length, const uint8_t *rhs, uint32_t rhs_length
) {
  int s = strncmp((const char *)lhs, (const char *)rhs, lhs_length < rhs_length ? lhs_length : rhs_length);
  if (s < 0) return -1; if (s > 0) return +1; return 0; }

void tagscale_init(void){ ups_register_compare("string",string_compare); }

void tagscale_flushAll(void){}
NAN_METHOD(upb_flushAll){ tagscale_flushAll(); info.GetReturnValue().Set(true); }

void tagscale_closeAll(void){ for(int i = 0;i<TABLE_MAX;i++) if ( TABLE[i] != NULL ) TABLE[i]->close(); }
NAN_METHOD(upb_closeAll){ tagscale_closeAll(); info.GetReturnValue().Set(true); }

/*
 ░░░        ░▒▓▓▒░   ░▒▓▓▓▒░      ░▓██▓▒    ░███░
 ▒▓▓▒░▒░▒░███████▒  ▒███▓█▓▒     ▒▓█████▓▒   ▓███▓▒
          ▓▓▓       ▒▓▓░         ▓▓▓   ▓▓▓   ▓▓▓░▓▓▒   ░▒▒░
          ░▒▓▒░      ▒▓█▒░       ▓▓▓   ▓▓▓   ░▓▓▒░▓▓▒   ▓▓▓
       ░░░  ░▒▓▒░     ▒▓█▓▓▒░    ▓▓▓  ░▒▒░    ▓▓▓ ▓▓▓   ▓▓▓
      ░▓▓▒   ░▒▓▒░    ▒▒▓▒██▓░   ▓▓▓  ▒▒▒     ▓▓▓ ▓▓▓   ▓▓▓
      ░▓██▓▓▓▓▓██▓    ▓▓▓░▓██▒   ▒███▓█▓▒     ▓▓▓ ▓▓▓  ░▓▓▒
          ░▒▓▓▓▓▒░    ░▓███▒░     ░▒▓▓▒░      ░░░ ▒██▒░▓▓▒
                       ▒██▒                       ░███▓▓▒░
*/

void Json::Init(){
  Isolate *isolate = Isolate::GetCurrent();
  Local<Object> json = isolate->GetCurrentContext()->Global()->Get(String::NewFromUtf8(isolate,"JSON"))->ToObject();
  Local<Function> stringify = json->Get(String::NewFromUtf8(isolate,"stringify")).As<Function>();
  Json::_JSON_.Reset(json);
  Json::_STRINGIFY_.Reset(stringify); }

inline const char* Json::stringify(Local<Value> value, uint32_t *valLen){
  Isolate *isolate = Isolate::GetCurrent();
  Local<Object>        json = Local<Object>::New(isolate,Json::_JSON_);
  Local<Function> stringify = Local<Function>::New(isolate,Json::_STRINGIFY_);;
  Local<Value> _stringify_args[] = { value };
  Local<Value> _stringify_result = stringify->Call(json, 1, _stringify_args);
  String::Utf8Value const _stringify_val(_stringify_result);
  *valLen = strlen(*_stringify_val);
  return strndup(*_stringify_val,*valLen); }

inline Local<Value> Json::parse (const char* data){
  Isolate *isolate = Isolate::GetCurrent();
  Local<String> json = String::NewFromUtf8( isolate, (char *) data);
  return JSON::Parse(json); }

Nan::Persistent<Object>   Json::_JSON_;
Nan::Persistent<Function> Json::_STRINGIFY_;

/*
 ░██▓░         ░▓▓░    ░█████▓▒   ░▓████████▒  ▓██░     ░█████████████▒
   ▒███░    ░▓██▒▒████████░ ░███░ ░▓█▒ ██░ ▒██  ░██▒      ▓██      ░▓█▒
     ░███░▒███▒▓███▒▒█▒    ░██ ▓██░    ███████░   ██      ░██████▓░
      ░█████▓░      ▒█▒   ░██▓ ░██▒    ████████░  ██       ███
      ▒██████░      ░██░░▒██████████   ██▒ ░▒▓██▒░██      ░██▓▓▓▒▒▒▒░
    ░██▒    ▒███▒  ░███▓███▓░     ▓██▓▓█████▓▒▒▒▓████████████████████▒
   ▒██▒       ▒██░ ▒██████▒        ▓███▓▒░     ░▓██░    ▒▓▒▒░    ░▓██▓
*/

NAN_METHOD(XTable::Find){
  if ( info.Length() != 1 ){ info.GetReturnValue().Set(false); return; }
  const int argc = 2; Isolate* isolate = info.GetIsolate();
  Local<Value> argv[argc] = { info.Holder(), info[0] };
  Local<Function> cons = Local<Function>::New(isolate, XCursor::constructor);
  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
  if ( !instance->Has(Nan::New("current").ToLocalChecked()) ){ info.GetReturnValue().Set(false); return; }
  info.GetReturnValue().Set(instance); }

/*
░▓▓░      ███░  ░▓██▒    ░▓███▓░    ▒█▓░     ▓█▒        ▒███████▓░
 ░██░    ░██▓  ░████▒   ░█████▓░   ░████▒░   ██▒       ░▓██▒░
  ▓██░  ░███   ▒██░    ░██▓░       ▓█▓▒██▒   ▓█▒       ▒▓▓▒
  ░███░░███▒   ░██▓░   ▓█▓         ██░ ░██░  ▒█▒       ▒▓███▓▒░
   ░██████▒     ▒███▓▒░██░        ▒██   ▓█▓  ▓█▒       ░▓███▓▒░
    ░███▓        ░▓███▓▓█░       ▒█████████▒ ██░        ▒██▒
    ░▓███▓░   ░░    ░██▒█▓▒░    ░███████▓▓██░██▒▒░      ░████▒   ░░
    ▓█▓▓████▒ ▒███████▓░▓█████▓▓███▓      ██░███████▓▒▒░ ▒████████▓
  ░▓██░  ▒███░░▓█████▓░ ░▒▓███▓▓▓▒▒░      ░░ ▒▓▒▓███▓▒▒░  ░▒██████▒
*/

void XScale::Init(v8::Local<v8::Object> exports) {
  Json::Init();
  Nan::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("XScale").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  Nan::SetPrototypeMethod(tpl, "close", Close);
  Nan::SetPrototypeMethod(tpl, "set", Set);
  Nan::SetPrototypeMethod(tpl, "get", Get);
  Nan::SetPrototypeMethod(tpl, "del", Del);
  Nan::SetPrototypeMethod(tpl, "defineIndex", DefineIndex);
  Nan::SetPrototypeMethod(tpl, "find", Find);
  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("XScale").ToLocalChecked(), tpl->GetFunction()); }
Nan::Persistent<Function> XScale::constructor;

XScale::XScale(const char* path){
  ups_parameter_t uid_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT32}, {0,0} };
  ups_parameter_t key_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)},
    {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  this->query_flags = UPS_FIND_NEAR_MATCH | UPS_SKIP_DUPLICATES;
  this->id = ALLOC_TABLE_ID(this);
  if( UPS_SUCCESS != ups_env_open      (&this->env, path, 0, NULL ))
  if( UPS_SUCCESS != ups_env_create    (&this->env, path, 0, 0664, 0 )) return;
  if( UPS_SUCCESS != ups_env_open_db   ( this->env, &this->keys, DBNAME_KEYS, 0,0 ))
  if( UPS_SUCCESS != ups_env_create_db ( this->env, &this->keys, DBNAME_KEYS, 0, &key_params[0]) ) return;
  if( UPS_SUCCESS != ups_env_open_db   ( this->env, &this->data, DBNAME_UID,  0,0))
  if( UPS_SUCCESS != ups_env_create_db ( this->env, &this->data, DBNAME_UID,  UPS_RECORD_NUMBER32, &uid_params[0]) ) return;
  this->path = strndup(path,strlen(path));
  this->open = true; }

inline void XScale::close(void){
  if ( this->open ){
    ups_env_close(this->env, UPS_AUTO_CLEANUP);
    this->open = false; }}

XScale::~XScale(){
  TABLE[this->id] = NULL;
  this->close();
  free(this->path); }

NAN_METHOD(XScale::New){
  if ( (!info.IsConstructCall()) || (info.Length() != 1) ){ info.GetReturnValue().Set(false); return; }
  String::Utf8Value pathAsUtf8(info[0]); const char * path = *pathAsUtf8;
  XScale* obj = new XScale(path);
  obj->Wrap(info.This());
  // obj->persistent().ClearWeak();
  info.GetReturnValue().Set(info.This()); }

NAN_METHOD(XScale::Close) {
  Local<Object> node_that = info.Holder();
  XScale* that = Nan::ObjectWrap::Unwrap<XScale>(node_that);
  that->close();
  info.GetReturnValue().Set(true); }

NAN_METHOD(XScale::Set) {
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  XScale* that = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  if ( info.Length() != 2 || !info[0]->IsString() || !info[1]->IsObject() ){ info.GetReturnValue().Set(false); return; }
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  const char *key = COPY_TO_CHAR(info[0]);
  uint32_t valLen = -1; const char* val = Json::stringify(info[1],&valLen);
  // CHECK_EXISTS (key->uid)
  _key.data = (void*)key; _key.size = (uint32_t) valLen + 1; _val.data = &recordId; _val.size = sizeof(recordId);
  if ( UPS_SUCCESS == ups_db_find(that->keys, 0, &_key, &_val, 0) ){
    recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
    if ( UPS_SUCCESS != ups_db_find(that->data, 0, &_key, &_val, 0 )){ info.GetReturnValue().Set(false); return; }
    Local<Object> obj = Json::parse((char *)_val.data)->ToObject();
    // update that->data (data)
    _key.data = &recordId; _key.size = sizeof(recordId); _val.data = (void*)val; _val.size = (uint32_t)strlen(val) + 1;
    if ( UPS_SUCCESS != ups_db_insert(that->data, 0, &_key, &_val, UPS_OVERWRITE )){ info.GetReturnValue().Set(false); return; }
    for(int i=0; i<that->indexCount; i++){
      that->index[i]->del( recordId, obj ); }
  } else {
    _key.flags = UPS_KEY_USER_ALLOC; // alloc new that->data (data)
    _key.data = &recordId; _key.size = sizeof(recordId); _val.data = (void*)val; _val.size = (uint32_t)strlen(val) + 1;
    if ( UPS_SUCCESS != ups_db_insert(that->data, 0, &_key, &_val, 0 )){ info.GetReturnValue().Set(false); return; }
    // that->keys (key->uid)
    _key.flags = 0;
    _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1; _val.data = &recordId; _val.size = sizeof(recordId);
    if ( UPS_SUCCESS != ups_db_insert(that->keys, 0, &_key, &_val, 0 )){ info.GetReturnValue().Set(false); return; }}
  free((void*)key);
  free((void*)val);
  for(int i=0; i<that->indexCount; i++){
    that->index[i]->set( recordId, info[1]->ToObject()); }
  info.GetReturnValue().Set(Nan::New<Number>(recordId)); }

NAN_METHOD(XScale::Get) {
  XScale* t = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  if ( info.Length() != 1 ){ info.GetReturnValue().Set(false); return; }
  ups_status_t STATUS; uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  String::Utf8Value keyAsUtf8(info[0]); const char *key = *keyAsUtf8;
  _key.data = (void*)key; _key.size = (uint32_t) strlen(key) + 1;
  if ( UPS_SUCCESS != ( STATUS = ups_db_find(t->keys, 0, &_key, &_val, 0 ))){
    // printf("XScale get %s: %s\n",key,ups_strerror(STATUS));
    info.GetReturnValue().Set(false); return; }
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != ( STATUS = ups_db_find(t->data, 0, &_key, &_val, 0 ))){
    // printf("XScale get-data %s[%i]: %s\n",key,recordId,ups_strerror(STATUS));
    info.GetReturnValue().Set(false); return; }
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  Local<String> json = String::NewFromUtf8(isolate,(char *)_val.data);
  Local<Value> value = JSON::Parse(json);
  info.GetReturnValue().Set(value); }

NAN_METHOD(XScale::Del) {
  XScale* that = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  if ( info.Length() != 1 ){ info.GetReturnValue().Set(false); return; }
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  String::Utf8Value keyAsUtf8(info[0]); const char *key = *keyAsUtf8;
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  if ( UPS_SUCCESS !=  ups_db_find(that->keys, 0, &_key, &_val, 0 )){ info.GetReturnValue().Set(false); return; }
  if ( UPS_SUCCESS !=  ups_db_erase(that->keys, 0, &_key, 0 )){ info.GetReturnValue().Set(false); return; }
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS !=  ups_db_find(that->data, 0, &_key, &_val, 0 )){ info.GetReturnValue().Set(false); return; }
  Local<Object> obj = JSON::Parse(String::NewFromUtf8(isolate,(char *)_val.data))->ToObject();
  for(int i=0; i<that->indexCount; i++){ that->index[i]->del( recordId, obj ); }
  _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS !=  ups_db_erase(that->data, 0, &_key, 0 )){ info.GetReturnValue().Set(false); return; }
  info.GetReturnValue().Set(true); }

NAN_METHOD(XScale::DefineIndex) {
  if ( info.Length() != 2 || !info[0]->IsString() ){ info.GetReturnValue().Set(false); return; }
  Isolate* isolate = info.GetIsolate();
  const int argc = 3;
  Local<Value> argv[argc] = { info.Holder(), info[0], info[1] };
  Local<Function> cons = Local<Function>::New(isolate, XIndex::constructor);
  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
  XScale* that  = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  XIndex* index = Nan::ObjectWrap::Unwrap<XIndex>(instance);
  that->index[that->indexCount++] = index;
  info.GetReturnValue().Set(instance); }

/*
     ░▒▓▒░       ░▒▓▓▒░            ░░░  ░▒▓▓▓▓▒░        ░▒▓▓▓▓▓▒░ ░░░   ░░░
      ░▒▓▓▒░     ▒▓█▓▒░    ░░░     ▒▒▒  ░▒██████▒░     ░▓██▓▓▓▓▒░ ░▒▒░ ░▓▓▒
        ░▒▓▓▒░░▒▒▒░░  ▒▒▒  ▓███░   ▓▓▓    ▓▓▓░▒▓▓▓▒░   ▓██░       ░▒▓▒░▓██░
          ░▒▓▓▓▓▒░    ▓▓▓  ▒███▒░  ▓▓▓    ▓▓▓   ░▒▓▒░  ███▓▒▒░▒░▒▒▓▒▒▒▓█▓▒
           ░▒███░     ▓▓▓   ▓██▓▒░ ▓▓▓    ▓▓▓    ░▓▓▒  ███▓▒▒░▒░▒▒▓▒░░▓██▒
          ░▒▒▓▓█▒░    ▓▓▓   ▓▓▓░▓█▓█▓▓    ▓▓▓     ░▒▓▒░▓▓▓           ░▓█▓▒░░
         ░▒▓▒░░▓█▓░   ▒▒▒   ▒▒▒ ▒████▓    ▓▓▓░░░   ░▓▓▒▓██▓▓▓▒░      ▒▓▓░░░░
         ▒██▒  ▒██▒   ░░░   ░░░ ░▓██▓▒    ▒▒▒▒▓█▒░░▒██▓▒▓█▓▓▓▒░      ▓▓▓ ░▒▒░
         ░▒▒░   ░▓█▓░                        ░▒███▓▓▓▒░             ░██▓  ░▓▓▒
*/

void XIndex::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("XIndex").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  Nan::SetPrototypeMethod(tpl, "close", Close);
  Nan::SetPrototypeMethod(tpl, "find", Find);
  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("XIndex").ToLocalChecked(), tpl->GetFunction()); }
Nan::Persistent<Function> XIndex::constructor;

XIndex::XIndex(XScale *parent, const char *name, uint32_t flags, Local<Object> This){
  this->query_flags = UPS_ONLY_DUPLICATES;
  int id = parent->indexCount;
  this->name    = (char*) name;
  this->flags   = flags;
  this->parent  = parent;
  this->data    = parent->data;
  ups_parameter_t date_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT64}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)}, {0,0}};
  ups_parameter_t string_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)},
    {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  ups_parameter_t* params = NULL;
  if ( flags == 1 ) params = &date_params[0];
  else params = &string_params[0];
  if ( UPS_SUCCESS != ups_env_open_db(parent->env, &this->keys, id+128, 0,0) ){
    if ( UPS_SUCCESS != ups_env_create_db(parent->env, &this->keys, id+128, UPS_ENABLE_DUPLICATE_KEYS, params)){
      return; }}
  this->open = true; }
XIndex::~XIndex(){ this->close(); }

inline void XIndex::close(void){
  if ( this->open ){ free(this->name); this->open = false; }}

NAN_METHOD(XIndex::New){
  if ( (!info.IsConstructCall()) || (info.Length() != 3) ){ info.GetReturnValue().Set(false); return; }
  Local<Object> This = info.This();
  Local<Object> Parent = info[0]->ToObject();
  Local<String> IndexKey = info[1]->ToString();
  uint32_t flags = info[2]->Uint32Value();
  XScale* parent = ObjectWrap::Unwrap<XScale>(Parent);
  const char *name = COPY_TO_CHAR(IndexKey);
  XIndex* obj = new XIndex(parent,name,flags,This);
  obj->Wrap(This);
  Parent->Set(IndexKey,This);
  info.GetReturnValue().Set(This); }

NAN_METHOD(XIndex::Close) {
  // XIndex* t = Nan::ObjectWrap::Unwrap<XIndex>(info.Holder());
  info.GetReturnValue().Set(true); }

inline void XIndex::set(uint32_t recordId, Local<Object> Subject){
  ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _val.data = &recordId; _val.size = sizeof(recordId);
  Local<String> IndexKey = String::NewFromUtf8(Isolate::GetCurrent(),this->name);
  if ( !Subject->Has(IndexKey) ) return;
  if ( this->flags == 1 ){
    struct timespec spec; clock_gettime(CLOCK_REALTIME, &spec); uint64_t secs = (uint64_t)spec.tv_sec;
    _key.data = (void*)&secs; _key.size = sizeof(uint64_t);
    if ( UPS_SUCCESS != ups_db_insert(this->keys, 0, &_key, &_val, 0 )) return; }
  if ( this->flags == 2 ){
    const char *key = COPY_TO_CHAR(Subject->Get(IndexKey)->ToString());
    _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
    free((void*)key);
    if ( UPS_SUCCESS != ups_db_insert(this->keys, 0, &_key, &_val, 0 )) return; }
  if ( this->flags == 3 ){
    Local<Array> List = Local<Array>::Cast(Subject->Get(IndexKey));
    int length = List->Length();
    const char *key; Local<Value> item;
    if ( List->IsArray() && length > 0 ) for(int i = 0; i < length; i++){
      if ( !(item = List->Get(i))->IsString() ){ continue; }
      key = COPY_TO_CHAR(item); _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
      ups_db_insert(this->keys, 0, &_key, &_val, UPS_DUPLICATE);
      free((void*)key); }}}

static inline bool cursor_remove_string(ups_cursor_t *cursor, uint32_t recordId, const char *key){
  uint32_t foundId; uint32_t duplicateKeys = 0; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _key.data = (void*)key;  _key.size = (uint32_t) strlen(key) + 1;
  if ( UPS_SUCCESS != ups_cursor_find(cursor, &_key, &_val, 0)                  ) return false;
  if ( UPS_SUCCESS != ups_cursor_get_duplicate_count(cursor, &duplicateKeys, 0) ) return false;
  for(uint32_t i=0;i<duplicateKeys;i++){
    if ( ( foundId = *(uint32_t *) _val.data ) == recordId ){ ups_cursor_erase (cursor,0); break; }
    if ( UPS_SUCCESS != ups_cursor_move (cursor, &_key, &_val, UPS_CURSOR_NEXT + UPS_ONLY_DUPLICATES) ){ break; }}
  return true; }

inline void XIndex::del(uint32_t recordId, Local<Object> Subject){
  Local<String> IndexKey = String::NewFromUtf8(Isolate::GetCurrent(),this->name);
  if ( !Subject->Has(IndexKey) ) return;
  ups_cursor_t *cursor; if ( UPS_SUCCESS != ups_cursor_create(&cursor, this->keys, 0, 0) ){ return; }
  const char* value;
  if ( this->flags == 1 ){}
  if ( this->flags == 2 ){
    value = COPY_TO_CHAR(Subject->Get(IndexKey));
    cursor_remove_string(cursor,recordId,value);
    free((void*)value); }
  if ( this->flags == 3 ){
    Local<Value> Member = Subject->Get(IndexKey);
    if ( Member->IsArray() ){
      Local<Array> List = Local<Array>::Cast(Member);
        int length = List->Length();
        if ( length > 0 ) for( int i = 0; i < length; i++){
          value = COPY_TO_CHAR(List->Get(i));
          cursor_remove_string(cursor,recordId,value);
          free((void*)value);}}}
  ups_cursor_close(cursor); }

/*
░░          ░▓▓░                         ▒██████▓░   ▒██████▒   ▓████░   ▓████████▓▒░
▒█▓░      ░▒███▒░▒▓████▓░ ░▓▒     ▒▓░  ░▓███▒▒▒▓██▒░ ██▓▒▓███░░▓█████▓   ███▒▒▒▒▓▓▒▓▒
░███▒    ░█████▓███████▓░ ▒█▓     ██▒  ▒███▓    ▓██▒ ██░  ░▓█▒▓██▒ ▓██░  ▓██    ▒▓▒▓▒
 ░████░  ░███▓▓██▓▒░      ▒█▒     ██▒  ░▓██▓░  ░██▓░ ▒██░   ░░██░  ░▓█▓  ▒██  ░▒██▓▒░
  ░▓███▓▒▓██▒▒██▒         ▒█▒     ██▒   ▒█▓▓▓▒▓███░   ▓███▒   ██    ░██ ░▓██▓█████▒
    ▒██████▒ ▒█▓          ▒█▒     ██▒   ▒█▒▒████▓░    ░▒▓██▒ ░██    ░██ ▒█████████▓░
    ▒█████▓░ ▒█▓          ▒█▒     ██▓   ▒█▓█████░ ░▓▓░   ░██▒▒█▓    ▓█▒ ▒███▒▒▒▒▒▓█▓
 ░█████▓████░░▓█████████▓░▒█▓▒▒▒▒▓███▒░ ▒████▓▓██▒▒██░   ░▓██▒██░ ░▓██  ▒██▒     ░██
 ▒███▓▒▒▒▒███░ ▒████████▓░▒██████████▓░ ▓██░  ░▓██▒█████████▓░███████▒  ▒██░      ██
 ░░       ░▓▓░            ░▒▒▒▒▒▒▒░▒▓░  ▒█▒     ▒▓░▒█████▓▒▒░ ░▓███▓░   ░██       ▓█▒▒░
*/

void XCursor::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("XCursor").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  Nan::SetPrototypeMethod(tpl, "close", Close);
  Nan::SetPrototypeMethod(tpl, "next", Next);
  Nan::SetPrototypeMethod(tpl, "prev", Prev);
  Nan::SetPrototypeMethod(tpl, "first", First);
  Nan::SetPrototypeMethod(tpl, "last", Last);
  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("XCursor").ToLocalChecked(), tpl->GetFunction()); }
Nan::Persistent<Function> XCursor::constructor;

XCursor::XCursor(XTable *parent, const char* key, uint32_t extra_flags){
  ups_status_t s;
  this->parent = parent;
  this->keys = parent->keys;
  this->data = parent->data;
  this->query_flags = parent->query_flags | extra_flags;
  this->length = strlen(key);
  this->key = strndup(key,this->length);
  if ( UPS_SUCCESS != (s= ups_cursor_create(&this->cur, keys, 0, 0 ))){ return; }
  // First Request
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _key.data = (void*)this->key; _key.size = this->length + 1;
  if ( UPS_SUCCESS != ( s=ups_cursor_find(this->cur, &_key, &_val, 0 ))){
    return; }
  if ( 0 != strcmp(key,(const char*)_key.data) ){ return; }
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != (s= ups_db_find(data,0, &_key, &_val, 0 ))){ return; }
  this->current = Json::parse((char *)_val.data)->ToObject();
  this->open = true; }
XCursor::~XCursor(){ this->close(); }

void XCursor::close(){
  if ( !this->open ) return;
  ups_cursor_close(this->cur);
  free((void*)this->key);
  this->open = false; }

NAN_METHOD(XCursor::New){
  if ( (!info.IsConstructCall()) || (info.Length() != 2) ){ info.GetReturnValue().Set(false); return; }
  Local<Object> Parent = info[0]->ToObject();
  Local<Object> This = info.This();
  const char *query = COPY_TO_CHAR(info[1]);
  XTable* parent = ObjectWrap::Unwrap<XTable>(Parent);
  XCursor* obj = new XCursor(parent, query, 0);
  if ( obj->open == false ){ delete obj; return; }
  obj->Wrap(info.This());
  This->Set(Nan::New("current").ToLocalChecked(),obj->current);
  info.GetReturnValue().Set(info.This()); }

NAN_METHOD(XCursor::Close) {
  XCursor* that = Nan::ObjectWrap::Unwrap<XCursor>(info.Holder());
  that->close(); info.GetReturnValue().Set(true); }

inline void XCursor::move(const Nan::FunctionCallbackInfo<v8::Value>& info, uint32_t flags){
  XCursor* that = Nan::ObjectWrap::Unwrap<XCursor>(info.Holder());
  ups_status_t s; uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  again: if ( UPS_SUCCESS != (s= ups_cursor_move (that->cur, &_key, &_val, flags | that->query_flags ))){
    info.Holder()->Set(Nan::New("current").ToLocalChecked(),Nan::New(false));
    info.GetReturnValue().Set(false); return; }
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != ( s = ups_db_find(that->data, 0, &_key, &_val, 0 ))){ goto again; }
  that->current = Json::parse((char*)_val.data)->ToObject();
  info.Holder()->Set(Nan::New("current").ToLocalChecked(),that->current);
  info.GetReturnValue().Set(true); }

NAN_METHOD(XCursor::Next){  move(info, UPS_CURSOR_NEXT ); }
NAN_METHOD(XCursor::Prev){  move(info, UPS_CURSOR_PREVIOUS ); }
NAN_METHOD(XCursor::First){ move(info, UPS_CURSOR_FIRST ); }
NAN_METHOD(XCursor::Last){  move(info, UPS_CURSOR_LAST ); }
