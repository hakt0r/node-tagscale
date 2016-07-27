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
#define DBNAME_DATE 3
#define DBNAME_TAG  4

#define FATAL_QUERY(f,st,msg) st=f; if(st!=UPS_SUCCESS){Nan::Error(msg);\
  info.GetReturnValue().Set(false);\
  printf("\x1b[35;43m %s \x1b[32;41m %s \x1b[0m\n",msg,ups_strerror(st));\
  return;}

#define FATAL_QUERY_NOERROR(f,st,msg) st=f; if(st!=UPS_SUCCESS){Nan::Error(msg);\
  info.GetReturnValue().Set(false);\
  return;}

static inline char *TO_CHAR(Handle<Value> val) {
  String::Utf8Value utf8(val->ToString());
  int len = utf8.length() + 1;
  char *str = (char *) calloc(sizeof(char), len);
  strncpy(str, *utf8, len);
  return str; }

static int string_compare(ups_db_t *db, const uint8_t *lhs, uint32_t lhs_length, const uint8_t *rhs, uint32_t rhs_length) {
  int s = strncmp((const char *)lhs, (const char *)rhs, lhs_length < rhs_length ? lhs_length : rhs_length);
  if (s < 0) return -1;
  if (s > 0) return +1;
  return 0; }

/* TAGSTORE HANDLING */

#define RESOLVE_TAGSTORE()\
  int __ID__ = info[0]->Int32Value(); \
  if ( (__ID__ < 0) or (__ID__ > TAGS_MAX) ){ info.GetReturnValue().Set(false); return; } \
  tagstoreHandle* t = TAGS[__ID__];

#define FAIL_LOG_NULL(f,st,msg) st=f; if(st!=UPS_SUCCESS){Nan::Error(msg);\
  printf("\x1b[35;43m %s \x1b[32;41m %s \x1b[0m\n",msg,ups_strerror(st)); return NULL; }

static tagstoreHandle* new_tagstore(const char* path){
  ups_status_t STATUS; bool opened = false;
  int i, uid = -1; for (i=0;i<TAGS_MAX;i++) if ( TAGS[i] == NULL ) { uid = i; break; }; if ( uid == -1 ) { return NULL; }
  tagstoreHandle* t = TAGS[uid] = (tagstoreHandle*)malloc(sizeof(tagstoreHandle)); t->id = uid;
  ups_parameter_t uid_params[]  = { {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT32}, {0,0} };
  ups_parameter_t date_params[] = { {UPS_PARAM_KEY_TYPE, UPS_TYPE_UINT64}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)}, {0,0}};
  ups_parameter_t key_params[]  = { {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)}, {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  ups_parameter_t tag_params[]  = { {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_RECORD_SIZE, sizeof(uint32_t)}, {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  if ( access( path, F_OK ) != -1 ) if( UPS_SUCCESS == ups_env_open(&t->env,path,0,NULL) ){
    FAIL_LOG_NULL(ups_env_open_db(t->env, &t->byKey,  DBNAME_KEYS, 0,0), STATUS, "table_bykey_open");
    FAIL_LOG_NULL(ups_env_open_db(t->env, &t->byUID,  DBNAME_UID,  0,0), STATUS, "table_byuid_open");
    FAIL_LOG_NULL(ups_env_open_db(t->env, &t->byDate, DBNAME_DATE, 0,0), STATUS, "table_bydate_open");
    FAIL_LOG_NULL(ups_env_open_db(t->env, &t->byTag,  DBNAME_TAG,  0,0), STATUS, "table_bytag_open");
    opened = true; }
  if ( !opened ){
    FAIL_LOG_NULL(ups_env_create(&t->env, path, 0, 0664, 0),STATUS,"table_env_create");
    FAIL_LOG_NULL(ups_env_create_db(t->env, &t->byKey,  DBNAME_KEYS, 0,                         &key_params[0]),  STATUS, "table_bykey_create");
    FAIL_LOG_NULL(ups_env_create_db(t->env, &t->byUID,  DBNAME_UID,  UPS_RECORD_NUMBER32,       &uid_params[0]),  STATUS, "table_byuid_create");
    FAIL_LOG_NULL(ups_env_create_db(t->env, &t->byDate, DBNAME_DATE, UPS_ENABLE_DUPLICATE_KEYS, &date_params[0]), STATUS, "table_bydate_create");
    FAIL_LOG_NULL(ups_env_create_db(t->env, &t->byTag,  DBNAME_TAG,  UPS_ENABLE_DUPLICATE_KEYS, &tag_params[0]),  STATUS, "table_bytag_create");
    opened = true; }
  t->path = strndup(path,strlen(path));
  if ( !opened ) { free(t); return NULL; };
  return t; }

static void free_tagstore(tagstoreHandle* t){
  ups_env_close(t->env, UPS_AUTO_CLEANUP);
  TAGS[t->id] = (tagstoreHandle*)NULL;
  free((void*)t->path);
  free((void*)t); }

static inline bool tags_remove(Isolate *isolate, tagstoreHandle *t, uint32_t _uid, Local<Object> obj){
  ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  uint32_t rec_uid; ups_cursor_t *cursor;
  if ( UPS_SUCCESS != ups_cursor_create(&cursor, t->byTag, 0, 0) ){ return false; }
  Local<Array>  tags = Local<Array>::Cast(obj->Get(String::NewFromUtf8(isolate,"tag")));
  const char *k; int length = tags->Length();; Local<Value> item;
  if ( tags->IsArray() && length > 0 ) for(int i = 0; i < length; i++){
    if ( !(item = tags->Get(i))->IsString() ){ continue; }
    k = TO_CHAR(item); _key.data = (void*)k;  _key.size = (uint32_t)strlen(k) + 1;
    if ( UPS_SUCCESS != ups_cursor_find(cursor, &_key, &_val, 0) ) { free((void*)k); continue; }
    rec_uid = *(uint32_t *) _val.data;      if ( rec_uid != _uid ) { free((void*)k); continue; }
    ups_cursor_erase (cursor,0);                                     free((void*)k); }
  return true; }

NAN_METHOD(upb_tags_open) {
  if ( info.Length() != 1 ){ info.GetReturnValue().Set(false); return; }
  String::Utf8Value pathAsUtf8(info[0]); const char * path = *pathAsUtf8;
  tagstoreHandle *t = new_tagstore(path);
  if ( t == NULL ){ info.GetReturnValue().Set(false); return; }
  info.GetReturnValue().Set(Nan::New<Number>(t->id)); }

NAN_METHOD(upb_tags_close) {
  if ( info.Length() != 1 || !info[0]->IsNumber() ){ info.GetReturnValue().Set(false); return; }
  RESOLVE_TAGSTORE();
  free_tagstore(t);
  info.GetReturnValue().Set(true); }

NAN_METHOD(upb_tags_set) {
  int argc = info.Length();
  if ( argc != 3 || !info[1]->IsString() || !info[2]->IsObject() ){ info.GetReturnValue().Set(false); return; }
  RESOLVE_TAGSTORE();
  ups_status_t STATUS; uint32_t _uid; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  String::Utf8Value keyAsUtf8(info[1]); const char *key = *keyAsUtf8;
  // JSON STRINGIFY
  Local<Object> value = info[2]->ToObject();
  Local<Object> json = isolate->GetCurrentContext()->Global()->Get(String::NewFromUtf8(isolate,"JSON"))->ToObject();
  Local<Function> stringify = json->Get(String::NewFromUtf8(isolate, "stringify")).As<Function>();
  Local<Value> args[] = { value };
  Local<Value> result = stringify->Call(json, 1, args);
  String::Utf8Value const __val(result); const char *val = *__val;
  // CHECK_EXISTS (key->uid)
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  _val.data = &_uid;      _val.size = sizeof(_uid);
  if ( UPS_SUCCESS == ups_db_find(t->byKey, 0, &_key, &_val, 0) ){
    _uid = *(uint32_t *) _val.data; _key.data = &_uid; _key.size = sizeof(_uid);
    FATAL_QUERY(ups_db_find(t->byUID, 0, &_key, &_val, 0),STATUS,"table_set_update_get_data");
    Local<String> json  = String::NewFromUtf8(isolate,(char *)_val.data);
    Local<Object>  obj = JSON::Parse(json)->ToObject();
    if ( !tags_remove(isolate,t,_uid,obj) ){ info.GetReturnValue().Set(false);; return; }
    // update t->byUID (data)
    _key.data = &_uid;      _key.size = sizeof(_uid);
    _val.data = (void*)val; _val.size = (uint32_t)strlen(val) + 1;
    FATAL_QUERY(ups_db_insert(t->byUID, 0, &_key, &_val, UPS_OVERWRITE),STATUS,key);
  } else {
    // alloc new t->byUID (data)
    _key.flags = UPS_KEY_USER_ALLOC;
    _key.data = &_uid;      _key.size = sizeof(_uid);
    _val.data = (void*)val; _val.size = (uint32_t)strlen(val) + 1;
    FATAL_QUERY(ups_db_insert(t->byUID, 0, &_key, &_val, 0),STATUS,key);
    _key.flags = 0;
    // t->byKey (key->uid)
    _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
    _val.data = &_uid;      _val.size = sizeof(_uid);
    FATAL_QUERY(ups_db_insert(t->byKey, 0, &_key, &_val, 0),STATUS,key); }
  // t->byDate (date->uid)
  struct timespec spec; clock_gettime(CLOCK_REALTIME, &spec); uint64_t secs = (uint64_t)spec.tv_sec;
  _key.data = (void*)&secs; _key.size = sizeof(uint64_t);
  _val.data = (void*)&_uid; _val.size = sizeof(uint32_t);
  FATAL_QUERY(ups_db_insert(t->byDate, 0, &_key, &_val, UPS_DUPLICATE),STATUS,"table_put_date")
  // t->byTag (data)
  Local<Array> tags = Local<Array>::Cast(value->Get(String::NewFromUtf8(isolate,"tag")));
  _val.data = &_uid; _val.size = sizeof(_uid);
  if ( tags->IsArray() && tags->Length() > 0 ){
    const char *k; int length = tags->Length();; Local<Value> item;
    for(int i = 0; i < length; i++){
      if ( !(item = tags->Get(i))->IsString() ){ continue; }
      k = TO_CHAR(item); _key.data = (void*)k; _key.size = (uint32_t)strlen(k) + 1;
      STATUS = ups_db_insert(t->byTag, 0, &_key, &_val, UPS_DUPLICATE);
      free((void*)k);
      FATAL_QUERY(STATUS,STATUS,"table_put_tag") }}
  info.GetReturnValue().Set(true); }

NAN_METHOD(upb_tags_get) {
  if ( info.Length() != 2 ){ info.GetReturnValue().Set(false); return; }
  RESOLVE_TAGSTORE();
  ups_status_t STATUS; uint32_t _uid; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  String::Utf8Value keyAsUtf8(info[1]); const char *key = *keyAsUtf8;
  _key.data = (void*)key; _key.size = (uint32_t) strlen(key) + 1;
  FATAL_QUERY_NOERROR(ups_db_find(t->byKey, 0, &_key, &_val, 0),STATUS,"table_get_uid")
  _uid = *(uint32_t *) _val.data; _key.data = &_uid; _key.size = sizeof(_uid);
  FATAL_QUERY(ups_db_find(t->byUID, 0, &_key, &_val, 0),STATUS,"table_get_data")
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  Local<String> json = String::NewFromUtf8(isolate,(char *)_val.data);
  Local<Value> value = JSON::Parse(json);
  info.GetReturnValue().Set(value); }

NAN_METHOD(upb_tags_del) {
  if ( info.Length() != 2 ){ info.GetReturnValue().Set(false); return; }
  RESOLVE_TAGSTORE();
  ups_status_t STATUS; uint32_t _uid; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  String::Utf8Value keyAsUtf8(info[1]); const char *key = *keyAsUtf8;
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  FATAL_QUERY_NOERROR(ups_db_find(t->byKey, 0, &_key, &_val, 0),STATUS,"table_del_get_uid")
  FATAL_QUERY_NOERROR(ups_db_erase(t->byKey, 0, &_key, 0)      ,STATUS,"table_del_key")
  _uid = *(uint32_t *) _val.data; _key.data = &_uid; _key.size = sizeof(_uid);
  FATAL_QUERY_NOERROR(ups_db_find(t->byUID, 0, &_key, &_val, 0),STATUS,"table_del_get_uid")
  // delete tags if necessary
  Local<String> json = String::NewFromUtf8(isolate,(char *)_val.data);
  Local<Value> value = JSON::Parse(json);
  if ( !tags_remove(isolate,t,_uid,value->ToObject()) ){ info.GetReturnValue().Set(false);; return; }
  // delete data
  _key.data = &_uid; _key.size = sizeof(_uid);
  FATAL_QUERY_NOERROR(ups_db_erase(t->byUID, 0, &_key, 0),STATUS,"table_del_val")
  info.GetReturnValue().Set(true); }

/* CURSOR HANDLING */

static cursorHandle* new_cursor(ups_db_t* DB, const char* key){
  int i, uid = -1;
  for (i=0;i<CURSOR_MAX;i++) if ( CURSOR[i] == NULL ) { uid = i; break; }; if ( uid == -1 ) { return NULL; }
  cursorHandle* t = CURSOR[uid] = (cursorHandle*)malloc(sizeof(cursorHandle)); t->id = uid;
  if ( UPS_SUCCESS != ups_cursor_create(&t->cur, DB, 0, 0) ){
    free(t); return NULL; }
  t->length = strlen(key);
  t->key = strndup(key,t->length);
  return t; }

static void free_cursor(cursorHandle* t){
  ups_cursor_close(t->cur);
  CURSOR[t->id] = (cursorHandle*)NULL;
  free((void*)t->key);
  free((void*)t); }

NAN_METHOD(upb_find){
  if ( info.Length() != 3 or !info[2]->IsFunction() ){ info.GetReturnValue().Set(false); return; }
  RESOLVE_TAGSTORE();
  uint32_t _uid; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  String::Utf8Value keyAsUtf8(info[1]); const char *key = *keyAsUtf8;
  cursorHandle* task = new_cursor(t->byTag,key);
  if ( task == NULL ){
    info.GetReturnValue().Set(false); return; }
  _key.data = (void*)task->key; _key.size = task->length + 1;
  if ( UPS_SUCCESS != ups_cursor_find(task->cur, &_key, &_val, 0) ){
    free_cursor(task); info.GetReturnValue().Set(false); return; }
  if ( 0 != strcmp(key,(const char*)_key.data) ){
    free_cursor(task); info.GetReturnValue().Set(false); return; }
  _uid = *(uint32_t *) _val.data; _key.data = &_uid; _key.size = sizeof(_uid);
  if ( UPS_SUCCESS != ups_db_find(t->byUID,0, &_key, &_val, 0) ){
    free_cursor(task); info.GetReturnValue().Set(false); return; }
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  Local<String> json = String::NewFromUtf8(isolate,(char *)_val.data);
  Local<Value> value = JSON::Parse(json);
  Local<Value> args[] = { value, Nan::New<Number>((uint64_t)task->id) };
  Local<Function> callbackHandle = info[2].As<Function>();
  Nan::MakeCallback(Nan::GetCurrentContext()->Global(), callbackHandle, 2, args);
  info.GetReturnValue().Set(Nan::New<Number>(_uid)); }

NAN_METHOD(upb_next){
  if ( info.Length() != 3 or !info[0]->IsNumber() or !info[1]->IsNumber() ){
    info.GetReturnValue().Set(false); return; }
  RESOLVE_TAGSTORE();
  int __CID__ = info[1]->Int32Value(); if ( __CID__ < 0 || __CID__ > CURSOR_MAX ){ info.GetReturnValue().Set(false); return; }
  cursorHandle* cursor = CURSOR[__CID__];
  int dir = info[2]->Int32Value();
  if ( cursor == NULL ){
    info.GetReturnValue().Set(false); return; }
  if ( dir == 0 ){
    free_cursor(cursor); info.GetReturnValue().Set(true); return; }
  uint32_t _uid; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  again: if ( UPS_SUCCESS != ups_cursor_move (cursor->cur, &_key, &_val, dir + UPS_ONLY_DUPLICATES) ){
    info.GetReturnValue().Set(false); return; }
  _uid = *(uint32_t *) _val.data; _key.data = &_uid; _key.size = sizeof(_uid);
  if ( UPS_SUCCESS != ups_db_find(t->byUID, 0, &_key, &_val, 0) ) goto again;
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  Local<String> json = String::NewFromUtf8(isolate,(char *)_val.data);
  Local<Value> value = JSON::Parse(json);
  info.GetReturnValue().Set(value); }

/* TABLE HANDLING */

#define TABLE_FREE_AND_FAIL { free(t); return NULL; }

#define RESOLVE_TABLE()\
  int __ID__ = info[0]->Int32Value(); \
  if ( (__ID__ < 0) or (__ID__ > TABLE_MAX) ){ info.GetReturnValue().Set(false); return; } \
  tableHandle* t = TABLE[__ID__];

static tableHandle* new_table(const char* path){
  ups_parameter_t key_params[]  = { {UPS_PARAM_KEY_TYPE, UPS_TYPE_CUSTOM}, {UPS_PARAM_CUSTOM_COMPARE_NAME, (uint64_t)"string"}, {0,0}};
  bool opened = false;
  int i, uid = -1; for (i=0;i<TABLE_MAX;i++) if ( TABLE[i] == NULL ) { uid = i; break; }; if ( uid == -1 ) { return NULL; }
  tableHandle* t = TABLE[uid] = (tableHandle*)malloc(sizeof(tableHandle)); t->id = uid;
  if ( access( path, F_OK ) != -1 ) if( UPS_SUCCESS == ups_env_open(&t->env,path,0,NULL) ){
    if ( UPS_SUCCESS != ups_env_open_db(t->env, &t->db, DBNAME_KEYS, 0,0) ) TABLE_FREE_AND_FAIL;
    opened = true; }
  if ( !opened ){
    if ( UPS_SUCCESS != ups_env_create(&t->env, path, 0, 0664, 0) ) TABLE_FREE_AND_FAIL;
    if ( UPS_SUCCESS != ups_env_create_db(t->env, &t->db, DBNAME_KEYS, 0, &key_params[0]) ) TABLE_FREE_AND_FAIL;
    opened = true; }
  if ( !opened ) TABLE_FREE_AND_FAIL;
  t->path = strndup(path,strlen(path));
  return t; }

static void free_table(tableHandle* t){
  ups_env_close(t->env, UPS_AUTO_CLEANUP);
  TABLE[t->id] = (tableHandle*)NULL;
  free((void*)t->path);
  free((void*)t); }

NAN_METHOD(upb_table_open){
  if ( info.Length() != 1 || !info[0]->IsString() ){ info.GetReturnValue().Set(false); return; }
  String::Utf8Value pathAsUtf8(info[0]); const char * path = *pathAsUtf8;
  tableHandle *t = new_table(path);
  info.GetReturnValue().Set(Nan::New<Number>(t->id)); }

NAN_METHOD(upb_table_close){
  if ( info.Length() != 1 || !info[0]->IsNumber() ){ info.GetReturnValue().Set(false); return; }
  RESOLVE_TABLE();
  free_table(t);
  info.GetReturnValue().Set(true); }

NAN_METHOD(upb_table_set){
  if ( info.Length() != 3 || !info[0]->IsNumber() || !info[1]->IsString() ){
    info.GetReturnValue().Set(false); return; }
  RESOLVE_TABLE();
  ups_status_t STATUS; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  String::Utf8Value keyAsUtf8(info[1]); const char *key = *keyAsUtf8;
  // JSON STRINGIFY
  Local<Object> json = isolate->GetCurrentContext()->Global()->Get(String::NewFromUtf8(isolate,"JSON"))->ToObject();
  Local<Function> stringify = json->Get(String::NewFromUtf8(isolate, "stringify")).As<Function>();
  Local<Value> args[] = { info[2] };
  Local<Value> result = stringify->Call(json, 1, args);
  String::Utf8Value const __val(result); const char *val = *__val;
  // SET DATA
  _key.data = (void*)key; _key.size = (uint32_t)strlen(key) + 1;
  _val.data = (void*)val; _val.size = (uint32_t)strlen(val) + 1;
  FATAL_QUERY(ups_db_insert(t->db, 0, &_key, &_val, UPS_OVERWRITE),STATUS,key);
  info.GetReturnValue().Set(true); }

NAN_METHOD(upb_table_get){
  if ( info.Length() != 2 || !info[0]->IsNumber() || !info[1]->IsString()){
    info.GetReturnValue().Set(false); return; }
  RESOLVE_TABLE();
  ups_status_t STATUS; ups_key_t _key = {0,0,0,0}; ups_record_t _val = {0,0,0};
  String::Utf8Value keyAsUtf8(info[1]); const char *key = *keyAsUtf8;
  _key.data = (void*)key; _key.size = (uint32_t) strlen(key) + 1;
  FATAL_QUERY_NOERROR(ups_db_find(t->db, 0, &_key, &_val, 0),STATUS,"table_get_data")
  Isolate *isolate = Isolate::GetCurrent(); v8::HandleScope scope( isolate );
  Local<String> json = String::NewFromUtf8(isolate,(char *)_val.data);
  Local<Value> value = JSON::Parse(json);
  info.GetReturnValue().Set(value); }

NAN_METHOD(upb_table_del){
  if ( info.Length() != 2 || !info[0]->IsNumber() || !info[1]->IsString()){
    info.GetReturnValue().Set(false); return; }
  RESOLVE_TABLE();
  ups_status_t STATUS; ups_key_t _key = {0,0,0,0};
  String::Utf8Value keyAsUtf8(info[1]); const char *key = *keyAsUtf8;
  _key.data = (void*)key; _key.size = (uint32_t) strlen(key) + 1;
  FATAL_QUERY_NOERROR(ups_db_erase(t->db, 0, &_key, 0),STATUS,"table_del_key")
  info.GetReturnValue().Set(true); }

NAN_METHOD(upb_flushAll){ tagscale_flushAll(); }
NAN_METHOD(upb_closeAll){ tagscale_closeAll(); }

void tagscale_init(void){
  ups_register_compare("string",string_compare);
  for(int i = 0;i<TAGS_MAX;i++)   TAGS[i] = NULL;
  for(int i = 0;i<TABLE_MAX;i++)  TABLE[i] = NULL;
  for(int i = 0;i<CURSOR_MAX;i++) CURSOR[i] = NULL; }

void tagscale_flushAll(void){
  for(int i = 0;i<TAGS_MAX;i++)  ups_env_flush(TAGS[i]->env, 0);
  for(int i = 0;i<TABLE_MAX;i++) ups_env_flush(TABLE[i]->env,0); }

void tagscale_closeAll(void){
  for(int i = 0;i<CURSOR_MAX;i++) if ( CURSOR[i] != NULL ){ free_cursor(CURSOR[i]); }
  for(int i = 0;i<TABLE_MAX;i++)  if ( TABLE[i] != NULL ){  free_table(TABLE[i]); }
  for(int i = 0;i<TAGS_MAX;i++)   if ( TAGS[i] != NULL ){   free_tagstore(TAGS[i]); }}
