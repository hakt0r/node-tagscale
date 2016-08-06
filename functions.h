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

#ifndef TAGSCALE_GRAB_H
#define TAGSCALE_GRAB_H

#include <nan.h>
#include <node.h>
#include <stdio.h>
#include "unistd.h"
#include <time.h>
#include <ups/upscaledb.h>

using namespace std;
using namespace v8;
using namespace Nan;

typedef class XScale XScale;
typedef class XIndex XIndex;
typedef class XCursor XCursor;

static const int TABLE_MAX = 1024;
static XScale *TABLE[TABLE_MAX];
inline int ALLOC_TABLE_ID(XScale* table){
  for (int i = 0; i < TABLE_MAX; i++ ) if ( TABLE[i] == NULL )
    { TABLE[i] = table; return i; };
  return -1; }

class Json {
  public:
    static void Init();
    static inline const char*  stringify (Local<Value> value, uint32_t *valLen);
    static inline Local<Value> parse     (const char* data);
  private:
    static Nan::Persistent<Object>   _JSON_;
    static Nan::Persistent<Function> _STRINGIFY_; };

class XBase : public Nan::ObjectWrap {
public:
 ups_db_t *keys;
 ups_db_t *data; };

class XScale : public XBase {
 public:
  ups_env_t *env;
  int indexCount = 0;
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> constructor;
  inline void close(void);
 private:
  explicit XScale(const char* path);
  ~XScale();
  static NAN_METHOD(New);
  static NAN_METHOD(Close);
  static NAN_METHOD(Set);
  static NAN_METHOD(Get);
  static NAN_METHOD(Del);
  static NAN_METHOD(DefineIndex);
  static NAN_METHOD(Find);
  XIndex *index[TABLE_MAX];
  char *path = NULL;
  bool open = false;
  int id;};

class XIndex : public XBase {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> constructor;
  inline void set(uint32_t recordId, Local<Object> Subject);
  inline void del(uint32_t recordId, Local<Object> Subject);
  XScale *parent;
 private:
  explicit XIndex(XScale *parent, const char* name, uint32_t flags, Local<Object> This);
  ~XIndex();
  inline void close(void);
  static NAN_METHOD(New);
  static NAN_METHOD(Close);
  static NAN_METHOD(Set);
  static NAN_METHOD(Get);
  static NAN_METHOD(Del);
  static NAN_METHOD(Find);
  uint32_t flags;
  char *name = NULL;
  bool open = false; };

class XCursor : public XBase {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> constructor;
  uint32_t flags;
  char *key;
  int length;
 private:
  explicit XCursor(ups_db_t *primary, ups_db_t *db, const char* key, uint32_t flags);
  ~XCursor();
  inline void close(void);
  static NAN_METHOD(New);
  static NAN_METHOD(Close);
  static NAN_METHOD(Next);
  static NAN_METHOD(Prev);
  static NAN_METHOD(First);
  static NAN_METHOD(Last);
  static inline void move(const Nan::FunctionCallbackInfo<v8::Value>& info, uint32_t flags);
  Local<Object> current;
  ups_cursor_t *cur;
  bool open = false; };

void tagscale_init(void);
void tagscale_flushAll(void);
void tagscale_closeAll(void);

NAN_METHOD(upb_flushAll);
NAN_METHOD(upb_closeAll);

#endif
