/*

  * c) 2016-2019 Sebastian Glaser <anx@ulzq.de>

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

/*
  ████████  ██████   ██████  ██      ███████
     ██    ██    ██ ██    ██ ██      ██
     ██    ██    ██ ██    ██ ██      ███████
     ██    ██    ██ ██    ██ ██           ██
     ██     ██████   ██████  ███████ ███████
*/

inline int ALLOC_TABLE_ID(XScale* table){
  for (int i = 0; i < TABLE_MAX; i++ ) if ( TABLE[i] == NULL )
    { TABLE[i] = table; return i; };
  return -1; }

inline void debug_flags(uint32_t flags){
  if ( flags & UPS_CURSOR_FIRST    ) printf("UPS_CURSOR_FIRST\n");
  if ( flags & UPS_CURSOR_LAST     ) printf("UPS_CURSOR_LAST\n");
  if ( flags & UPS_CURSOR_NEXT     ) printf("UPS_CURSOR_NEXT\n");
  if ( flags & UPS_CURSOR_PREVIOUS ) printf("UPS_CURSOR_PREVIOUS\n");
  if ( flags & UPS_SKIP_DUPLICATES ) printf("UPS_SKIP_DUPLICATES\n");
  if ( flags & UPS_ONLY_DUPLICATES ) printf("UPS_ONLY_DUPLICATES\n");
  if ( flags & UPS_FIND_EQ_MATCH   ) printf("UPS_FIND_EQ_MATCH\n");
  if ( flags & UPS_FIND_LT_MATCH   ) printf("UPS_FIND_LT_MATCH\n");
  if ( flags & UPS_FIND_GT_MATCH   ) printf("UPS_FIND_GT_MATCH\n"); }

static inline char* COPY_TO_CHAR(Napi::Env env, Napi::Value val) {
  size_t length = 0;
  napi_get_value_string_utf8(env,val,NULL,0     ,&length);
  char* utf8 = (char*) malloc(length+1);
  napi_get_value_string_utf8(env,val,utf8,length+1,&length);
  return utf8; }

static int string_compare (
  ups_db_t *db, const uint8_t *lhs, uint32_t lhs_length, const uint8_t *rhs, uint32_t rhs_length
) {
  int s = strncmp((const char *)lhs, (const char *)rhs, lhs_length < rhs_length ? lhs_length : rhs_length);
  if (s < 0) {return -1;} if (s > 0) {return +1;} return 0; }

void tagscale_init(void){
  ups_register_compare("string",string_compare); }

inline void tagscale_flushAll(void){
  for(int i = 0;i<TABLE_MAX;i++) if ( TABLE[i] != NULL ) ups_env_flush(TABLE[i]->env,0); }

inline void tagscale_closeAll(void){
  for(int i = 0;i<TABLE_MAX;i++) if ( TABLE[i] != NULL ) TABLE[i]->close(); }

Napi::FunctionReference Tools::Parse;
Napi::FunctionReference Tools::Stringify;
Napi::FunctionReference Tools::Log;

void Tools::Init(Napi::Env env, Napi::Object exports){
  Napi::Object JSON    = env.Global().Get("JSON").As<Napi::Object>();
  Napi::Object Console = env.Global().Get("console").As<Napi::Object>();
  Tools::Parse     = Napi::Persistent( JSON.Get("parse").As<Napi::Function>() );
  Tools::Stringify = Napi::Persistent( JSON.Get("stringify").As<Napi::Function>() );
  Tools::Log       = Napi::Persistent( Console.Get("log").As<Napi::Function>() );
}

inline std::string Tools::stringify(Napi::Env env, Napi::Value value) {
  return Stringify.Call({value}).As<Napi::String>().Utf8Value(); }

inline const char* Tools::stringify(Napi::Env env, Napi::Value value, uint32_t *valLen) {
  Napi::String resultStr = Stringify.Call({value}).As<Napi::String>();
  return resultStr.Utf8Value().c_str(); }

inline Napi::Value Tools::parse (Napi::Env env, const char* data){
  return Parse.Call({Napi::Value::From(env,data)}); }

/*
  ██   ██ ███████  ██████  █████  ██      ███████
   ██ ██  ██      ██      ██   ██ ██      ██
    ███   ███████ ██      ███████ ██      █████
   ██ ██       ██ ██      ██   ██ ██      ██
  ██   ██ ███████  ██████ ██   ██ ███████ ███████
*/

Napi::FunctionReference XScale::constructor;

Napi::Object XScale::Init(Napi::Env env, Napi::Object exports) {
  exports.Set("flushAll", Napi::Function::New(env, XScale::flushAll));
  exports.Set("closeAll", Napi::Function::New(env, XScale::closeAll));
  Napi::HandleScope scope(env);
  Napi::Function func = DefineClass(env, "XScale", {
    InstanceMethod(      "close", &XScale::Close),
    InstanceMethod(        "set", &XScale::Set),
    InstanceMethod(        "get", &XScale::Get),
    InstanceMethod(        "del", &XScale::Del),
    InstanceMethod(       "find", &XScale::Find),
    InstanceMethod(       "each", &XScale::Each),
    InstanceMethod("defineIndex", &XScale::DefineIndex)
  });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("XScale", func);
  return exports;
}

XScale::XScale(const Napi::CallbackInfo& info) : Napi::ObjectWrap<XScale>(info)  {
  int length = info.Length();
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  if ( length != 1        ) { Napi::TypeError::New(env, "Reqiures one argument of String").ThrowAsJavaScriptException(); }
  if ( !info[0].IsString()) { Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException(); }
  this->path = strdup(info[0].As<Napi::String>().Utf8Value().c_str());
  ups_parameter_t uid_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT32}, {0,0} };
  ups_parameter_t key_params[] = {
    {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)},
    {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  this->queryFlags = UPS_FIND_NEAR_MATCH | UPS_SKIP_DUPLICATES;
  uint32_t flags = UPS_AUTO_RECOVERY;
  int result = UPS_FILE_NOT_FOUND;
  struct stat buffer;
  if ( 0 == stat(this->path,&buffer) ){
                                  result =                      ups_env_open   (&this->env, this->path, flags, NULL ); }
  if      ( UPS_FILE_NOT_FOUND == result ) { if( UPS_SUCCESS != ups_env_create (&this->env, this->path, flags, 0664, 0 )) { free(this->path); return; } }
  else if ( UPS_NEED_RECOVERY  == result ) { if( UPS_SUCCESS != ups_env_open   (&this->env, this->path, flags, 0       )) { free(this->path); return; } }
  else if ( UPS_SUCCESS != result ) { free(this->path); return; }
  if( UPS_SUCCESS != ups_env_open_db   ( this->env, &this->keys, DBNAME_KEYS, 0,0 ))
  if( UPS_SUCCESS != ups_env_create_db ( this->env, &this->keys, DBNAME_KEYS, 0, &key_params[0]) ) { free(this->path); return; }
  if( UPS_SUCCESS != ups_env_open_db   ( this->env, &this->data, DBNAME_UID,  0,0))
  if( UPS_SUCCESS != ups_env_create_db ( this->env, &this->data, DBNAME_UID,  UPS_RECORD_NUMBER32, &uid_params[0]) ) { free(this->path); return; }
  this->id = ALLOC_TABLE_ID(this);
  this->open = true;
  info.This().As<Napi::Object>().Set("path",Napi::Value::From(env,this->path));
  //- printf("keys: %llx\n",keys);
}

inline void XScale::close(void){
  if ( this->open ){
    ups_env_close(this->env, UPS_AUTO_CLEANUP);
      this->open = false; }}

Napi::Value XScale::Close(const Napi::CallbackInfo& info) {
  if ( !this->open ) { return Napi::Value::From(info.Env(),true); }
  this->close();
  return Napi::Value::From(info.Env(),true); }

Napi::Value XScale::flushAll (const Napi::CallbackInfo& info) {
  tagscale_flushAll(); return Napi::Value::From(info.Env(),true); }

Napi::Value XScale::closeAll (const Napi::CallbackInfo& info) {
  tagscale_closeAll(); return Napi::Value::From(info.Env(),true); }

Napi::Value XScale::Set(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  if ( !this->open ) { Napi::TypeError::New(env,"XScale db not open").ThrowAsJavaScriptException(); return env.Null(); }
  if ( info.Length() != 2 or !info[0].IsString() ){
    printf("XScale::Set Error: invalid arguments count=%i firstIsString=%i.\n",(int)info.Length(),(int)info[0].IsString() );
    return Napi::Value::From(env,false); }
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  std::string key = info[0].As<Napi::String>().Utf8Value();
  std::string val = Tools::stringify(env,info[1]);
  uint32_t valLen = val.size();
  // CHECK_EXISTS (key->uid)
  _key.data = (void*)key.c_str();
  _key.size = (uint32_t) valLen + 1;
  _val.data = &recordId;
  _val.size = sizeof(recordId);
  if ( UPS_SUCCESS == ups_db_find(this->keys, 0, &_key, &_val, 0) ){
    recordId = *(uint32_t *) _val.data;
    _key.data = &recordId;
    _key.size = sizeof(recordId);
    if ( UPS_SUCCESS != ups_db_find(this->data, 0, &_key, &_val, 0 )){
      printf("XScale::Set Error: Key exists but has no data.\n");
      return Napi::Value::From(env,false); }
    Napi::Object obj = Tools::parse(env,(char *)_val.data).As<Napi::Object>();
    // update this->data (data)
    _key.data = &recordId;
    _key.size = sizeof(recordId);
    _val.data = (void*)val.c_str();
    _val.size = (uint32_t) valLen + 1;
    if ( UPS_SUCCESS != ups_db_insert(this->data, 0, &_key, &_val, UPS_OVERWRITE )){
      printf("XScale::Set Error: Key exists but cannot change data.\n");
      return Napi::Value::From(env,false); }
    for(int i=0; i<this->indexCount; i++){
      this->index[i]->del(env, recordId, obj ); }
  } else {
    _key.flags = UPS_KEY_USER_ALLOC; // alloc new this->data (data)
    _key.data = &recordId;
    _key.size = sizeof(recordId);
    _val.data = (void*)val.c_str();
    _val.size = (uint32_t) valLen + 1;
    if ( UPS_SUCCESS != ups_db_insert(this->data, 0, &_key, &_val, 0 )){
      printf("XScale::Set Error: Can't insert data.\n");
      return Napi::Value::From(env,false); }
    // this->keys (key->uid)
    _key.flags = 0;
    _key.data = (void*)key.c_str();
    _key.size = (uint32_t) key.size() + 1;
    _val.data = &recordId;
    _val.size = sizeof(recordId);
    if ( UPS_SUCCESS != ups_db_insert(this->keys, 0, &_key, &_val, 0 )){
      printf("XScale::Set Error: Can't insert key.\n");
      return Napi::Value::From(env,false); }}
  for(int i=0; i<this->indexCount; i++){
    this->index[i]->set(env,recordId, info[1].As<Napi::Object>()); }
  return Napi::Number::New(env,recordId); }

Napi::Value XScale::Get(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if ( !this->open ) { return Napi::Value::From(env,false); }
  if ( info.Length() != 1 ){ return Napi::Value::From(env,false); }
  ups_status_t STATUS; uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  const char *key = info[0].As<Napi::String>().Utf8Value().c_str();
  _key.data = (void*)key; _key.size = (uint32_t) strlen(key) + 1;
  if ( UPS_SUCCESS != ( STATUS = ups_db_find(this->keys, 0, &_key, &_val, 0 ))){
    // printf("XScale get %s: %s\n",key,ups_strerror(STATUS));
    return Napi::Value::From(env,false); }
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != ( STATUS = ups_db_find(this->data, 0, &_key, &_val, 0 ))){
    // printf("XScale get-data %s[%i]: %s\n",key,recordId,ups_strerror(STATUS));
    return Napi::Value::From(env,false); }
  Napi::Value value = Tools::parse(env,(char *)_val.data);
  return value; }

Napi::Value XScale::Del(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if ( !this->open ) { return Napi::Value::From(env,false); }
  if ( info.Length() != 1 ){ return Napi::Value::From(env,false); }
  uint32_t recordId; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  const char *key = info[0].As<Napi::String>().Utf8Value().c_str();
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  if ( UPS_SUCCESS !=  ups_db_find(this->keys, 0, &_key, &_val, 0 )){ return Napi::Value::From(env,false); }
  if ( UPS_SUCCESS !=  ups_db_erase(this->keys, 0, &_key, 0 )){ return Napi::Value::From(env,false); }
  recordId = *(uint32_t *) _val.data; _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS !=  ups_db_find(this->data, 0, &_key, &_val, 0 )){ return Napi::Value::From(env,false); }
  Napi::Object obj = Tools::parse(env,(char *)_val.data).As<Napi::Object>();
  for(int i=0; i<this->indexCount; i++){ this->index[i]->del(env, recordId, obj ); }
  _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS !=  ups_db_erase(this->data, 0, &_key, 0 )){ return Napi::Value::From(env,false); }
  return Napi::Value::From(env,true); }

Napi::Value XScale::DefineIndex(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if ( !this->open         ){ return Napi::Value::From(env,false); }
  if ( info.Length() != 2  ){ return Napi::Value::From(env,false); }
  if ( !info[0].IsString() ){ return Napi::Value::From(env,false); }
  if ( !info[1].IsNumber() ){ return Napi::Value::From(env,false); }
  //- printf("defineIndex %s\n",info[0].As<Napi::String>().Utf8Value().c_str());
  Napi::Object Index = XIndex::constructor.New({info.This().As<Napi::Value>(),info[0],info[1]});
  this->index[this->indexCount++] = Napi::ObjectWrap<XIndex>::Unwrap(Index);
  return Napi::Value::From(env,true); }

XScale::~XScale(){
  TABLE[this->id] = NULL;
  this->close();
  free(this->path); }

/*
  ██   ██ ████████  █████  ██████  ██      ███████
   ██ ██     ██    ██   ██ ██   ██ ██      ██
    ███      ██    ███████ ██████  ██      █████
   ██ ██     ██    ██   ██ ██   ██ ██      ██
  ██   ██    ██    ██   ██ ██████  ███████ ███████
*/

Napi::Value XTable::Find(const Napi::CallbackInfo& info){
  Napi::Env   env = info.Env();
  size_t     argc = info.Length();
  if ( argc != 1 ){
    return Napi::Value::From(env,false); }
  Napi::Object cursor = XCursor::constructor.New({
    info.This().As<Napi::Value>(), info[0], Napi::Value::From(env,0)});
  if ( ! cursor.Has("current") ){
    return Napi::Value::From(env,false); }
  return cursor;
}

Napi::Value XTable::Each(const Napi::CallbackInfo& info){
  Napi::Env env = info.Env();
  if ( info.Length() != 2 ){ return Napi::Value::From(env,false); }
  if ( !info[1].IsFunction() ){
    printf("XTable::Each Error[argv#2] Not a function.\n");
    return Napi::Value::From(env,false); }
  Napi::Object cursor = XCursor::constructor.New({
    info.This().As<Napi::Value>(), info[0], Napi::Value::From(env,0)});
  if ( ! cursor.Has("current") ){
    return Napi::Value::From(env,false); }
  // Callback Loop
  Napi::Function func = info[1].As<Napi::Function>();
  XCursor*          c = Napi::ObjectWrap<XCursor>::Unwrap(cursor);
  more:
  Napi::Value key     = cursor.Get("key");
  Napi::Value current = cursor.Get("current");
  Napi::Value result  = func.Call(env.Global(),{cursor,current,key});
  if ( ! result.IsEmpty() ) { if ( ! result.As<Napi::Boolean>().Value() ) { goto cleanup; }}
  if ( c->move(info, UPS_CURSOR_NEXT ) ) goto more;
  cleanup:
  c->close();
  return Napi::Value::From(env,true);
}

/*
  ██   ██ ██ ███    ██ ██████  ███████ ██   ██
   ██ ██  ██ ████   ██ ██   ██ ██       ██ ██
    ███   ██ ██ ██  ██ ██   ██ █████     ███
   ██ ██  ██ ██  ██ ██ ██   ██ ██       ██ ██
  ██   ██ ██ ██   ████ ██████  ███████ ██   ██
*/

Napi::FunctionReference XIndex::constructor;

Napi::Object XIndex::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);
  Napi::Function func = DefineClass(env, "XIndex", {
    InstanceMethod("close", &XIndex::Close),
    InstanceMethod("find",  &XIndex::Find),
    InstanceMethod("each",  &XIndex::Each)
  });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("XIndex", func);
  return exports;
}

XIndex::XIndex(const Napi::CallbackInfo& info) : Napi::ObjectWrap<XIndex>(info){
  if ( (!info.IsConstructCall()) || (info.Length() != 3) ){ return; }
  Napi::Object     This = info.This().As<Napi::Object>();
  Napi::Object   Parent = info[0].As<Napi::Object>();
  Napi::String IndexKey = info[1].As<Napi::String>();
  const char*      name = COPY_TO_CHAR(info.Env(), info[1]);
  uint32_t        flags = info[2].As<Napi::Number>().Uint32Value();
  XScale*        parent = Napi::ObjectWrap<XScale>::Unwrap(Parent);
  this->setup(parent,name,flags);
  Parent.Set(IndexKey,This);
}

void XIndex::setup(XScale *parent, const char *name, uint32_t flags){
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
  //- printf("keys: %llx\n",keys);
  this->open = true; }

XIndex::~XIndex(){ this->close(); }

inline void XIndex::close(void){
  if ( this->open ){ free(this->name); this->open = false; }}

Napi::Value XIndex::Close(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  // XIndex* t = info.Holder().Unwrap<XIndex>();
  this->close();
  return Napi::Value::From(env,true); }

inline void XIndex::setBool(Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){
  Napi::Value value = Subject.Get(IndexKey);
  bool istrue       = true;
  if      ( value.IsEmpty()      ){ istrue = false; }
  else if ( value.IsNull()       ){ istrue = false; }
  else if ( value.IsUndefined()  ){ istrue = false; }
  else if ( value.IsBoolean() ){ if ( value.As<Napi::Boolean>().Value() == false ) { istrue = false; }}
  else if ( value.IsNumber()  ){ if ( value.As<Napi::Number>().Uint32Value() == 0 ) { istrue = false; }}
  // -printf("setBool: %i\n",istrue);
  if ( istrue == false ){ return; }
  ups_status_t s; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {4,&recordId,0};
  _key.data = &recordId; _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != (s= ups_db_insert(this->keys, 0, &_key, &_val, 0 ))){
    //printf("hereE %s\n",ups_strerror(s));
  }}

inline void XIndex::setDate(Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){
  ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _val.data = &recordId; _val.size = sizeof(recordId);
  struct timespec spec; clock_gettime(CLOCK_REALTIME, &spec); uint64_t secs = (uint64_t)spec.tv_sec;
  _key.data = (void*)&secs; _key.size = sizeof(uint64_t);
  if ( UPS_SUCCESS != ups_db_insert(this->keys, 0, &_key, &_val, 0 )) return; }

inline void XIndex::setString(Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){
  ups_status_t s; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _val.data = &recordId; _val.size = sizeof(recordId);
  const char *key = COPY_TO_CHAR(env,Subject.Get(IndexKey).As<Napi::String>());
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  if ( UPS_SUCCESS != (s= ups_db_insert(this->keys, 0, &_key, &_val, UPS_DUPLICATE ))) {
    printf("%s %s\n",__FUNCTION__,ups_strerror(s));
    return; }
  free((void*)key); }

inline void XIndex::setStringArray(Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){
  ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _val.data = &recordId; _val.size = sizeof(recordId);
  Napi::Array List = Subject.Get(IndexKey).As<Napi::Array>();
  int length = List.Length();
  const char *key; Napi::Value item;
  if ( List.IsArray() && length > 0 ) for(int i = 0; i < length; i++){
    if ( !(item = List.Get(i)).IsString() ){ continue; }
    key = COPY_TO_CHAR(env,item); _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
    ups_db_insert(this->keys, 0, &_key, &_val, UPS_DUPLICATE);
    free((void*)key); }}

inline void XIndex::set(Napi::Env env, uint32_t recordId, Napi::Object Subject){
  //- printf("setindex %s: %i\n",name,indexFlags);
  Napi::String IndexKey = Napi::Value::From(env,this->name).As<Napi::String>();
  if ( !Subject.Has(IndexKey) ) return;
  switch ( this->indexFlags ) {
    case XTYPE_BOOL:         XIndex::setBool(env,recordId, Subject, IndexKey);        break;
    case XTYPE_DATE:         XIndex::setDate(env,recordId, Subject, IndexKey);        break;
    case XTYPE_STRING:       XIndex::setString(env,recordId, Subject, IndexKey);      break;
    case XTYPE_STRING_ARRAY: XIndex::setStringArray(env,recordId, Subject, IndexKey); break; }}

inline void XIndex::delBool(ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){
  ups_key_t _key = {0,0,0,0}; _key.size = (sizeof(uint32_t)); _key.data = &recordId;
  ups_db_erase(this->keys, 0, &_key, 0 ); }

inline void XIndex::delDate(ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){}

inline void XIndex::delString(Napi::Env env, ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){
  innerDelString(cursor,recordId,COPY_TO_CHAR(env,Subject.Get(IndexKey))); }

inline void XIndex::delStringArray(Napi::Env env, ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey){
  Napi::Value Member = Subject.Get(IndexKey); if ( !Member.IsArray() ) return;
  Napi::Array List = Member.As<Napi::Array>(); int length = List.Length();
  if ( length > 0 ) for( int i = 0; i < length; i++) innerDelString(cursor,recordId,COPY_TO_CHAR(env, List.Get(i))); }

inline void XIndex::innerDelString(ups_cursor_t *cursor, uint32_t recordId, const char* value){
  uint32_t foundId; uint32_t duplicateKeys = 0; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  _key.data = (void*)value;  _key.size = (uint32_t) strlen(value) + 1;
  if ( UPS_SUCCESS != ups_cursor_find(cursor, &_key, &_val, 0)                  ) return;
  if ( UPS_SUCCESS != ups_cursor_get_duplicate_count(cursor, &duplicateKeys, 0) ) return;
  for(uint32_t i=0;i<duplicateKeys;i++){
    if ( ( foundId = *(uint32_t *) _val.data ) == recordId ){ ups_cursor_erase (cursor,0); break; }
    if ( UPS_SUCCESS != ups_cursor_move (cursor, &_key, &_val, UPS_CURSOR_NEXT + UPS_ONLY_DUPLICATES) ){ break; }}
  free ((void*)value); }

inline void XIndex::del(Napi::Env env, uint32_t recordId, Napi::Object Subject){
  ups_cursor_t *cursor; Napi::String IndexKey = Napi::String::New(env,this->name);
  if ( !Subject.Has(IndexKey) ) return;
  if ( UPS_SUCCESS != ups_cursor_create(&cursor, this->keys, 0, 0) ){ return; }
  switch ( this->indexFlags ) {
    case XTYPE_BOOL:         delBool(cursor,recordId,Subject,IndexKey);   break;
    case XTYPE_DATE:         delDate(cursor,recordId,Subject,IndexKey);   break;
    case XTYPE_STRING:       delString(env, cursor,recordId,Subject,IndexKey); break;
    case XTYPE_STRING_ARRAY: delStringArray(env, cursor,recordId,Subject,IndexKey); }
  ups_cursor_close(cursor); }

/*
  ██   ██  ██████ ██    ██ ██████  ███████  ██████  ██████
   ██ ██  ██      ██    ██ ██   ██ ██      ██    ██ ██   ██
    ███   ██      ██    ██ ██████  ███████ ██    ██ ██████
   ██ ██  ██      ██    ██ ██   ██      ██ ██    ██ ██   ██
  ██   ██  ██████  ██████  ██   ██ ███████  ██████  ██   ██
*/

Napi::FunctionReference XCursor::constructor;

Napi::Object XCursor::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);
  Napi::Function func = DefineClass(env, "XCursor", {
    InstanceMethod("close", &XCursor::Close),
    InstanceMethod("next",  &XCursor::Next),
    InstanceMethod("prev",  &XCursor::Prev),
    InstanceMethod("first", &XCursor::First),
    InstanceMethod("last",  &XCursor::Last)
  });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("XCursor", func);
  return exports;
}

XCursor::XCursor (const Napi::CallbackInfo& info) : Napi::ObjectWrap<XCursor>(info){
  std::string  name;
  Napi::Env     env = info.Env();
  Napi::Object This = info.This().As<Napi::Object>();
  // Tools::Log.Call({Napi::Value::From(env,"\x1b[32m$cursor$\x1b[0m"),info[1]});
  ups_status_t    s;
  uint32_t    flags = info[2].As<Napi::Number>().Uint32Value();
  ups_key_t    _key = {0,0,0,0};
  ups_record_t _val = {0,0,0};
  this->parent      = Napi::ObjectWrap<XScale>::Unwrap(info[0].As<Napi::Object>());
  this->keys        = parent->keys;
  this->data        = parent->data;
  this->queryFlags  = parent->queryFlags | flags;
  this->key         = NULL;
  if ( 4 == info[1].Type() ) {
    name = info[1].As<Napi::String>().Utf8Value();
    this->key = (char*)name.c_str(); }
  if ( UPS_SUCCESS != (s= ups_cursor_create(&this->cur, this->keys, 0, 0 ))){
    //- printf("!created[%s]: [bool]\n", ups_strerror(s));
    return; }
  //- printf("curs-keys: %llx, %llx\n",this->keys,key);
  // First Request
  if ( key == NULL ) {
    this->queryFlags = 0;
    _key.data = (void*)NULL;
    _key.size = 0;
    if ( UPS_SUCCESS != ( s=ups_cursor_move(this->cur, &_key, &_val, UPS_CURSOR_FIRST ))){
      //- printf("!moved[%s]: [null]\n", ups_strerror(s));
      return; }
    //- printf("got: %i %s\n",_key.size,(char*)_key.data);
    this->key         = NULL;
    this->current_key = strndup( (char*)_key.data, _key.size - 1 );
  } else if ( parent->indexFlags == XTYPE_BOOL ){
    this->queryFlags = 0;
    if ( UPS_SUCCESS != ( s=ups_cursor_move(this->cur, &_key, &_val, UPS_CURSOR_NEXT ))){
      //- printf("!moved[%s]: [bool]\n", ups_strerror(s));
      return; }
    this->key         = strndup( (char*)_key.data, _key.size );
    this->current_key = strndup( (char*)_key.data, _key.size );
  } else {
    this->length = strlen(key); this->key = strndup(key,this->length);
    _key.data = (void*)this->key; _key.size = this->length + 1;
    if ( UPS_SUCCESS != ( s=ups_cursor_find(this->cur, &_key, &_val, 0 ))){ return; }
    this->current_key = strndup( (char*)_key.data, _key.size - 1 ); }
  // Get Data
  recordId = *(uint32_t*) _val.data;
  _key.data = &recordId;
  _key.size = sizeof(recordId);
  if ( UPS_SUCCESS != (s= ups_db_find(data,0, &_key, &_val, 0 ))){
    //- printf("NEW_CURSee %s\n",ups_strerror(s));
    return; }
  firstRecordId = recordId;
  This.Set("current",Tools::parse(info.Env(),(char *)_val.data).As<Napi::Object>());
  This.Set("key",Napi::Value::From(env, this->current_key));
  //- printf("c$%llx: %s\n",this,current_key);
  // Tools::Log.Call({Napi::Value::From(env,"current$$"),This.Get("current")});
  this->open = true; }

XCursor::~XCursor(){ this->close(); }

Napi::Value XCursor::Close(const Napi::CallbackInfo& info) {
  this->close();
  return Napi::Value::From(info.Env(),true); }

Napi::Value XCursor::Next  (const Napi::CallbackInfo& info){ return Napi::Value::From(info.Env(), move(info, UPS_CURSOR_NEXT )); }
Napi::Value XCursor::Prev  (const Napi::CallbackInfo& info){ return Napi::Value::From(info.Env(), move(info, UPS_CURSOR_PREVIOUS )); }
Napi::Value XCursor::First (const Napi::CallbackInfo& info){ return Napi::Value::From(info.Env(), move(info, UPS_CURSOR_FIRST )); }
Napi::Value XCursor::Last  (const Napi::CallbackInfo& info){ return Napi::Value::From(info.Env(), move(info, UPS_CURSOR_LAST )); }

void XCursor::close(){
  if ( !this->open ) return;
  ups_cursor_close( this->cur );
  if ( this->key != NULL ) free( (void*) this->key );
  if ( this->current_key != NULL ) free( this->current_key );
  this->open = false; }

inline void XCursor::invalidate(const Napi::CallbackInfo& info){
  printf("INVALIDATE\n");
  Napi::Env     env = info.Env();
  Napi::Object This = info.This().As<Napi::Object>();
  This.Set("current", Napi::Value::From(env, false));
  This.Set("key",     Napi::Value::From(env, false)); }

inline bool XCursor::move(const Napi::CallbackInfo& info, uint32_t flags){
  Napi::Env env = info.Env();
  Napi::Object This = info.This().As<Napi::Object>();
  // Tools::Log.Call({Napi::Value::From(env,"current$@"),This.Get("current")});
  ups_status_t    s;
  ups_key_t    _key = {0,0,0,0};
  ups_record_t _val = {0,0,0};
  //- printf("move:: %i\n",flags);
  if ( !this->open ) {
    printf("not open\n");
    this->invalidate(info);
    return false; }
  again:
  if ( UPS_SUCCESS != (s= ups_cursor_move (this->cur, &_key, &_val, flags | this->queryFlags ))){
    //- printf("move::end\n");
    This.Set("current",env.Undefined());
    return false; }
  if ( flags & UPS_CURSOR_PREVIOUS ) if ( recordId == firstRecordId ){
    This.Set("current",env.Undefined());
    return false; }
  this->current_key = (char*) realloc( (void*)this->current_key, _key.size - 1 );
  strncpy( this->current_key, (char*)_key.data, _key.size - 1 );
  // Get Data
  uint32_t newRecordId = *(uint32_t *) _val.data;
  _key.data = &newRecordId;
  _key.size = sizeof(newRecordId);
  if ( UPS_SUCCESS != (s= ups_db_find(this->data, 0, &_key, &_val, 0 ))){
    //- printf("next::again[%s]: %s\n", this->key,ups_strerror(s));
    goto again; }
  if ( flags & UPS_CURSOR_NEXT ) if ( newRecordId == firstRecordId ){
    This.Set("current",env.Undefined());
    return false; }
  This.Set("current",Tools::parse(env,(char*)_val.data).As<Napi::Object>());
  This.Set("key",Napi::Value::From(env, this->current_key));
  //- Tools::Log.Call({Napi::Value::From(env,"current$"),This.Get("current")});
  recordId = newRecordId;
  return true; }
