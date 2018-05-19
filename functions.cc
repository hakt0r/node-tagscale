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

Nan::JSON NanJSON;

static inline char *COPY_TO_CHAR(Handle<Value> val) {
  String::Utf8Value utf8(val->ToString());
  return strndup( *utf8, utf8.length() + 1); }

static int string_compare (
  ups_db_t *db, const uint8_t *lhs, uint32_t lhs_length, const uint8_t *rhs, uint32_t rhs_length
) {
  int s = strncmp((const char *)lhs, (const char *)rhs, lhs_length < rhs_length ? lhs_length : rhs_length);
  if (s < 0) return -1; if (s > 0) return +1; return 0; }

void tagscale_init(void){ ups_register_compare("string",string_compare); }

void tagscale_flushAll(void) { for(int i = 0;i<TABLE_MAX;i++) if ( TABLE[i] != NULL ) ups_env_flush(TABLE[i]->env,0); }
NAN_METHOD(upb_flushAll){ tagscale_flushAll(); info.GetReturnValue().Set(true); }

void tagscale_closeAll(void){ for(int i = 0;i<TABLE_MAX;i++) if ( TABLE[i] != NULL ) TABLE[i]->close(); }
NAN_METHOD(upb_closeAll){ tagscale_closeAll(); info.GetReturnValue().Set(true); }

/*
 ░░░        ░▒▓▓▒░   ░▒▓▓▓▒░      ░▓██▓▒    ░███░       ░▒░
 ▒▓▓▒░▒░▒░███████▒  ▒███▓█▓▒     ▒▓█████▓▒   ▓███▓▒    ░▓▒░
          ▓▓▓       ▒▓▓░         ▓▓▓   ▓▓▓   ▓▓▓░▓▓▒   ░▓▒░
          ░▒▓▒░      ▒▓█▒░       ▓▓▓   ▓▓▓   ░▓▓▒░▓▓▒   ▓▓▓
       ░░░  ░▒▓▒░     ▒▓█▓▓▒░    ▓▓▓  ░▒▒░    ▓▓▓ ▓▓▓   ▓▓▓
      ░▓▓▒   ░▒▓▒░   ▒▓▒  ██▓░   ▓▓▓  ▒▒▒     ▓▓▓ ▓▓▓   ▓▓▓
      ░▓██▓▓▓▓▓██▓    ▒▓▓░▓██▒   ▒███▓█▓▒     ▓▓▓ ▓▓▓  ░▓▓▒
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
  return NanJSON.Parse(json).ToLocalChecked(); }

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
  if ( !instance->Has(Nan::New("current").ToLocalChecked()) ){
    info.GetReturnValue().Set(false); return; }
  info.GetReturnValue().Set(instance); }

NAN_METHOD(XTable::Each){
  if ( info.Length() != 2 ){ info.GetReturnValue().Set(false); return; }
  if ( !info[1]->IsFunction() ){
    printf("XTable::Each Error[argv#2] Not a function.\n"); info.GetReturnValue().Set(false); return; }
  const int argc = 2; Isolate* isolate = info.GetIsolate();
  Local<Value> argv[argc] = { info.Holder(), info[0] };
  Local<Function> cons = Local<Function>::New(isolate, XCursor::constructor);
  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
  if ( !instance->Has(Nan::New("current").ToLocalChecked()) ){
    info.GetReturnValue().Set(false); return; }
  // Callback Loop
  Local<Function> func = Local<Function>::Cast(info[1]);
  XCursor* c = Nan::ObjectWrap::Unwrap<XCursor>(instance);
  more:
  Local<Value> key = instance->Get(Nan::New("key").ToLocalChecked());
  Local<Value> current = instance->Get(Nan::New("current").ToLocalChecked());
  Handle<Value> argvcb[] = {key,current,instance};
  Nan::MaybeLocal<Value> result = func->Call( instance, 3, argvcb );
  if ( ! result.IsEmpty() ) if ( !result.ToLocalChecked()->BooleanValue() ) goto cleanup;
  if ( XCursor::move(instance, info, UPS_CURSOR_NEXT ) ) goto more;
  cleanup:
  c->close(); info.GetReturnValue().Set(true); }

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
  Nan::SetPrototypeMethod(tpl, "each", Each);
  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("XScale").ToLocalChecked(), tpl->GetFunction()); }
Nan::Persistent<Function> XScale::constructor;

XScale::XScale(const char* path){
  ups_parameter_t uid_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT32}, {0,0} };
  ups_parameter_t key_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)},
    {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  this->queryFlags = UPS_FIND_NEAR_MATCH | UPS_SKIP_DUPLICATES;
  uint32_t flags = UPS_AUTO_RECOVERY;
  int                             result =                      ups_env_open   (&this->env, path, flags, NULL );
  if      ( UPS_FILE_NOT_FOUND == result ) { if( UPS_SUCCESS != ups_env_create (&this->env, path, flags, 0664, 0 )) return; }
  else if ( UPS_NEED_RECOVERY  == result ) { if( UPS_SUCCESS != ups_env_open   (&this->env, path, flags, 0       )) return; }
  else if ( UPS_SUCCESS != result ) return;
  if( UPS_SUCCESS != ups_env_open_db   ( this->env, &this->keys, DBNAME_KEYS, 0,0 ))
  if( UPS_SUCCESS != ups_env_create_db ( this->env, &this->keys, DBNAME_KEYS, 0, &key_params[0]) ) return;
  if( UPS_SUCCESS != ups_env_open_db   ( this->env, &this->data, DBNAME_UID,  0,0))
  if( UPS_SUCCESS != ups_env_create_db ( this->env, &this->data, DBNAME_UID,  UPS_RECORD_NUMBER32, &uid_params[0]) ) return;
  this->id = ALLOC_TABLE_ID(this);
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
  if ( !that->open ) { return; }
  that->close();
  info.GetReturnValue().Set(true); }

NAN_METHOD(XScale::Set) {
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  XScale* that = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  if ( !that->open ) { return; }
  if ( info.Length() != 2 or !info[0]->IsString() ){
    printf("XScale::Set Error: invalid arguments count=%i firstIsString=%i.\n",info.Length(),(int) info[0]->IsString() );
    info.GetReturnValue().Set(false); return; }
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  const char *key = COPY_TO_CHAR(info[0]);
  uint32_t valLen = -1; const char* val = Json::stringify(info[1],&valLen);
  // CHECK_EXISTS (key->uid)
  _key.data = (void*)key; _key.size = (uint32_t) valLen + 1; _val.data = &recordId; _val.size = sizeof(recordId);
  if ( UPS_SUCCESS == ups_db_find(that->keys, 0, &_key, &_val, 0) ){
    recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
    if ( UPS_SUCCESS != ups_db_find(that->data, 0, &_key, &_val, 0 )){
      printf("XScale::Set Error: Key exists but has no data.\n");
      info.GetReturnValue().Set(false); return; }
    Local<Object> obj = Json::parse((char *)_val.data)->ToObject();
    // update that->data (data)
    _key.data = &recordId; _key.size = sizeof(recordId); _val.data = (void*)val; _val.size = (uint32_t)strlen(val) + 1;
    if ( UPS_SUCCESS != ups_db_insert(that->data, 0, &_key, &_val, UPS_OVERWRITE )){
      printf("XScale::Set Error: Key exists but cannot change data.\n");
      info.GetReturnValue().Set(false); return; }
    for(int i=0; i<that->indexCount; i++){
      that->index[i]->del( recordId, obj ); }
  } else {
    _key.flags = UPS_KEY_USER_ALLOC; // alloc new that->data (data)
    _key.data = &recordId; _key.size = sizeof(recordId); _val.data = (void*)val; _val.size = (uint32_t)strlen(val) + 1;
    if ( UPS_SUCCESS != ups_db_insert(that->data, 0, &_key, &_val, 0 )){
      printf("XScale::Set Error: Can't insert data.\n");
      info.GetReturnValue().Set(false); return; }
    // that->keys (key->uid)
    _key.flags = 0;
    _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1; _val.data = &recordId; _val.size = sizeof(recordId);
    if ( UPS_SUCCESS != ups_db_insert(that->keys, 0, &_key, &_val, 0 )){
      printf("XScale::Set Error: Can't insert key.\n");
      info.GetReturnValue().Set(false); return; }}
  free((void*)key);
  free((void*)val);
  for(int i=0; i<that->indexCount; i++){
    that->index[i]->set( recordId, info[1]->ToObject()); }
  info.GetReturnValue().Set(Nan::New<Number>(recordId)); }

NAN_METHOD(XScale::Get) {
  XScale* t = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  if ( !t->open ) { return; }
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
  Local<Value> value = NanJSON.Parse(json).ToLocalChecked();
  info.GetReturnValue().Set(value); }

NAN_METHOD(XScale::Del) {
  XScale* that = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  if ( !that->open ) { return; }
  if ( info.Length() != 1 ){ info.GetReturnValue().Set(false); return; }
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  String::Utf8Value keyAsUtf8(info[0]); const char *key = *keyAsUtf8;
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  if ( UPS_SUCCESS !=  ups_db_find(that->keys, 0, &_key, &_val, 0 )){ info.GetReturnValue().Set(false); return; }
  if ( UPS_SUCCESS !=  ups_db_erase(that->keys, 0, &_key, 0 )){ info.GetReturnValue().Set(false); return; }
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS !=  ups_db_find(that->data, 0, &_key, &_val, 0 )){ info.GetReturnValue().Set(false); return; }
  Local<Object> obj = NanJSON.Parse(String::NewFromUtf8(isolate,(char *)_val.data)).ToLocalChecked()->ToObject();
  for(int i=0; i<that->indexCount; i++){ that->index[i]->del( recordId, obj ); }
  _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS !=  ups_db_erase(that->data, 0, &_key, 0 )){ info.GetReturnValue().Set(false); return; }
  info.GetReturnValue().Set(true); }

NAN_METHOD(XScale::DefineIndex) {
  XScale* that = Nan::ObjectWrap::Unwrap<XScale>(info.Holder());
  if ( !that->open ) { return; }
  if ( info.Length() != 2 || !info[0]->IsString() ){ info.GetReturnValue().Set(false); return; }
  Isolate* isolate = info.GetIsolate();
  const int argc = 3;
  Local<Value> argv[argc] = { info.Holder(), info[0], info[1] };
  Local<Function> cons = Local<Function>::New(isolate, XIndex::constructor);
  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> instance = cons->NewInstance(context, argc, argv).ToLocalChecked();
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
  Nan::SetPrototypeMethod(tpl, "each", Each);
  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("XIndex").ToLocalChecked(), tpl->GetFunction()); }
Nan::Persistent<Function> XIndex::constructor;

XIndex::XIndex(XScale *parent, const char *name, uint32_t flags, Local<Object> This){
  this->queryFlags = UPS_ONLY_DUPLICATES;
  int id = parent->indexCount;
  this->name       = (char*) name;
  this->indexFlags = flags;
  this->parent     = parent;
  this->data       = parent->data;
  ups_parameter_t bool_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT32}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)}, {0,0}};
  ups_parameter_t date_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT64}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)}, {0,0}};
  ups_parameter_t string_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)},
    {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  ups_parameter_t* params = NULL;
  if ( flags == XTYPE_BOOL ) { this->queryFlags = 0; params = &bool_params[0]; }
  else if ( flags == XTYPE_DATE ) params = &date_params[0];
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

inline void XIndex::setBool(uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){
  if ( Subject->Get(IndexKey)->IsFalse() ){ return; }
  ups_status_t s; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {4,&recordId,0};
  _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != (s= ups_db_insert(this->keys, 0, &_key, &_val, 0 ))){
    //printf("hereE %s\n",ups_strerror(s));
  }}

inline void XIndex::setDate(uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){
  ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _val.data = &recordId; _val.size = sizeof(recordId);
  struct timespec spec; clock_gettime(CLOCK_REALTIME, &spec); uint64_t secs = (uint64_t)spec.tv_sec;
  _key.data = (void*)&secs; _key.size = sizeof(uint64_t);
  if ( UPS_SUCCESS != ups_db_insert(this->keys, 0, &_key, &_val, 0 )) return; }

inline void XIndex::setString(uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){
  ups_status_t s; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _val.data = &recordId; _val.size = sizeof(recordId);
  const char *key = COPY_TO_CHAR(Subject->Get(IndexKey)->ToString());
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  if ( UPS_SUCCESS != (s= ups_db_insert(this->keys, 0, &_key, &_val, UPS_DUPLICATE ))) {
    printf("%s %s\n",__FUNCTION__,ups_strerror(s));
    return; }
  free((void*)key); }

inline void XIndex::setStringArray(uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){
  ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _val.data = &recordId; _val.size = sizeof(recordId);
  Local<Array> List = Local<Array>::Cast(Subject->Get(IndexKey));
  int length = List->Length();
  const char *key; Local<Value> item;
  if ( List->IsArray() && length > 0 ) for(int i = 0; i < length; i++){
    if ( !(item = List->Get(i))->IsString() ){ continue; }
    key = COPY_TO_CHAR(item); _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
    ups_db_insert(this->keys, 0, &_key, &_val, UPS_DUPLICATE);
    free((void*)key); }}

inline void XIndex::set(uint32_t recordId, Local<Object> Subject){
  Local<String> IndexKey = String::NewFromUtf8(Isolate::GetCurrent(),this->name);
  if ( !Subject->Has(IndexKey) ) return;
  switch ( this->indexFlags ) {
    case XTYPE_BOOL:         XIndex::setBool(recordId, Subject, IndexKey);        break;
    case XTYPE_DATE:         XIndex::setDate(recordId, Subject, IndexKey);        break;
    case XTYPE_STRING:       XIndex::setString(recordId, Subject, IndexKey);      break;
    case XTYPE_STRING_ARRAY: XIndex::setStringArray(recordId, Subject, IndexKey); break; }}

inline void XIndex::delBool(ups_cursor_t *cursor, uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){
  ups_key_t _key = {0,0,0,0}; _key.size = (sizeof(uint32_t)); _key.data = &recordId;
  ups_db_erase(this->keys, 0, &_key, 0 ); }

inline void XIndex::delDate(ups_cursor_t *cursor, uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){}

inline void XIndex::delString(ups_cursor_t *cursor, uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){
  innerDelString(cursor,recordId,COPY_TO_CHAR(Subject->Get(IndexKey))); }

inline void XIndex::delStringArray(ups_cursor_t *cursor, uint32_t recordId, Local<Object> Subject, Local<String> IndexKey){
  Local<Value> Member = Subject->Get(IndexKey); if ( !Member->IsArray() ) return;
  Local<Array> List = Local<Array>::Cast(Member); int length = List->Length();
  if ( length > 0 ) for( int i = 0; i < length; i++) innerDelString(cursor,recordId,COPY_TO_CHAR(List->Get(i))); }

inline void XIndex::innerDelString(ups_cursor_t *cursor, uint32_t recordId, const char* value){
  uint32_t foundId; uint32_t duplicateKeys = 0; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _key.data = (void*)value;  _key.size = (uint32_t) strlen(value) + 1;
  if ( UPS_SUCCESS != ups_cursor_find(cursor, &_key, &_val, 0)                  ) return;
  if ( UPS_SUCCESS != ups_cursor_get_duplicate_count(cursor, &duplicateKeys, 0) ) return;
  for(uint32_t i=0;i<duplicateKeys;i++){
    if ( ( foundId = *(uint32_t *) _val.data ) == recordId ){ ups_cursor_erase (cursor,0); break; }
    if ( UPS_SUCCESS != ups_cursor_move (cursor, &_key, &_val, UPS_CURSOR_NEXT + UPS_ONLY_DUPLICATES) ){ break; }}
  free ((void*)value); }

inline void XIndex::del(uint32_t recordId, Local<Object> Subject){
  ups_cursor_t *cursor; Local<String> IndexKey = String::NewFromUtf8(Isolate::GetCurrent(),this->name);
  if ( !Subject->Has(IndexKey) ) return;
  if ( UPS_SUCCESS != ups_cursor_create(&cursor, this->keys, 0, 0) ){ return; }
  switch ( this->indexFlags ) {
    case XTYPE_BOOL:         delBool(cursor,recordId,Subject,IndexKey);   break;
    case XTYPE_DATE:         delDate(cursor,recordId,Subject,IndexKey);   break;
    case XTYPE_STRING:       delString(cursor,recordId,Subject,IndexKey); break;
    case XTYPE_STRING_ARRAY: delStringArray(cursor,recordId,Subject,IndexKey); }
  ups_cursor_close(cursor); }

/*
░░          ░▓▓░                         ▒██████▓░   ▒██████▒   ▓████░     ▓█████▓▒░
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
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  ups_status_t s;
  this->parent = parent;
  this->keys = parent->keys;
  this->data = parent->data;
  this->queryFlags = parent->queryFlags | extra_flags;
  if ( UPS_SUCCESS != (s= ups_cursor_create(&this->cur, keys, 0, 0 ))){ return; }
  // First Request
  if ( key == NULL ) {
    this->queryFlags = 0;
    _key.data = (void*)NULL; _key.size = 0;
    if ( UPS_SUCCESS != ( s=ups_cursor_move(this->cur, &_key, &_val, UPS_CURSOR_FIRST ))){
      // printf("!moved[%s]: [bool]\n", ups_strerror(s));
      return; }
    // printf("got: %i %s\n",_key.size,(char*)_key.data);
    this->key         = NULL;
    this->current_key = strndup( (char*)_key.data, _key.size - 1 );
  } else if ( parent->indexFlags == XTYPE_BOOL ){
    this->queryFlags = 0;
    if ( UPS_SUCCESS != ( s=ups_cursor_move(this->cur, &_key, &_val, UPS_CURSOR_NEXT ))){
      // printf("!moved[%s]: [bool]\n", ups_strerror(s));
      return; }
    this->key         = strndup( (char*)_key.data, _key.size );
    this->current_key = strndup( (char*)_key.data, _key.size );
  } else {
    this->length = strlen(key); this->key = strndup(key,this->length);
    _key.data = (void*)this->key; _key.size = this->length + 1;
    if ( UPS_SUCCESS != ( s=ups_cursor_find(this->cur, &_key, &_val, 0 ))){ return; }
    this->current_key = strndup( (char*)_key.data, _key.size - 1 ); }
  // Get Data
  recordId = *(uint32_t*) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != (s= ups_db_find(data,0, &_key, &_val, 0 ))){
    // printf("NEW_CURSee %s\n",ups_strerror(s));
    return; }
  this->current = Json::parse((char *)_val.data)->ToObject();
  this->open = true; }
XCursor::~XCursor(){ this->close(); }

void XCursor::close(){
  if ( !this->open ) return;
  ups_cursor_close( this->cur );
  if ( this->key != NULL ) free( (void*) this->key );
  if ( this->current_key != NULL ) free( this->current_key );
  this->open = false; }

NAN_METHOD(XCursor::New){
  const char *query;
  if ( (!info.IsConstructCall()) || (info.Length() != 2) ){ info.GetReturnValue().Set(false); return; }
  Local<Object> Parent = info[0]->ToObject();
  Local<Object> This = info.This();
  if ( info[1]->IsString() ){
    query = COPY_TO_CHAR(info[1]); }
  else query = NULL;
  XTable* parent = ObjectWrap::Unwrap<XTable>(Parent);
  XCursor* obj = new XCursor(parent, query, 0);
  if ( obj->open == false ){ delete obj; return; }
  obj->Wrap(info.This());
  This->Set(Nan::New("current").ToLocalChecked(),obj->current);
  This->Set(Nan::New("key").ToLocalChecked(),Nan::New(obj->current_key).ToLocalChecked());
  info.GetReturnValue().Set(info.This()); }

NAN_METHOD(XCursor::Close) {
  XCursor* that = Nan::ObjectWrap::Unwrap<XCursor>(info.Holder());
  that->close(); info.GetReturnValue().Set(true); }

inline void XCursor::invalidate(const Nan::FunctionCallbackInfo<v8::Value>& info,Local<Object> This){
  printf("INVALIDATE\n");
  This->Set(Nan::New("current").ToLocalChecked(),Nan::New(false));
  This->Set(Nan::New("key").ToLocalChecked(),Nan::New(false));
  info.GetReturnValue().Set(false); }

inline bool XCursor::move(Local<Object> That, const Nan::FunctionCallbackInfo<v8::Value>& info, uint32_t flags){
  XCursor* that = Nan::ObjectWrap::Unwrap<XCursor>(That);
  ups_status_t s; uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  if ( !that->open ) {
    printf("not open\n");
    that->invalidate(info,That); return false; }
  again:
  if ( UPS_SUCCESS != (s= ups_cursor_move (that->cur, &_key, &_val, flags | that->queryFlags ))){
    That->Set(Nan::New("current").ToLocalChecked(),Nan::Undefined());
    return false; }
  that->current_key = (char*) realloc( (void*)that->current_key, _key.size - 1 );
  strncpy( that->current_key, (char*)_key.data, _key.size - 1 );
  // Get Data
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != (s= ups_db_find(that->data, 0, &_key, &_val, 0 ))){
    // printf("next::again[%s]: %s\n", that->key,ups_strerror(s));
    goto again; }
  that->current = Json::parse((char*)_val.data)->ToObject();
  That->Set(Nan::New("current").ToLocalChecked(),that->current);
  That->Set(Nan::New("key").ToLocalChecked(),Nan::New(that->current_key).ToLocalChecked());
  return true; }

NAN_METHOD(XCursor::Next){  info.GetReturnValue().Set( move(info.Holder(), info, UPS_CURSOR_NEXT )); }
NAN_METHOD(XCursor::Prev){  info.GetReturnValue().Set( move(info.Holder(), info, UPS_CURSOR_PREVIOUS )); }
NAN_METHOD(XCursor::First){ info.GetReturnValue().Set( move(info.Holder(), info, UPS_CURSOR_FIRST )); }
NAN_METHOD(XCursor::Last){  info.GetReturnValue().Set( move(info.Holder(), info, UPS_CURSOR_LAST )); }
