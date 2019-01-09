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

#ifndef XSCALE_H
#define XSCALE_H

// #include <uv.h>
// #include <stdio.h>
// #include <time.h>
#include <napi.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <ups/upscaledb.h>

#define DBNAME_KEYS 1
#define DBNAME_UID  2
#define DBNAME_MORE 3

#define XTYPE_XSCALE        1
#define XTYPE_KEYVAL        2
#define XTYPE_BOOL          4
#define XTYPE_DATE          8
#define XTYPE_STRING        16
#define XTYPE_STRING_ARRAY  32

typedef class XScale  xscale;
typedef class XTable  xtable;
typedef class XIndex  xindex;
typedef class XCursor xcursor;

static const int     TABLE_MAX = 1024;
static xscale* TABLE[TABLE_MAX];

inline int  ALLOC_TABLE_ID(xscale* table);
inline void debug_flags       (uint32_t flags);
inline void tagscale_flushAll (void);
inline void tagscale_closeAll (void);
       void tagscale_init     (void);

class Tools {
public:
  static void Init(Napi::Env env, Napi::Object exports);
  static inline const char*  stringify (Napi::Env env, Napi::Value value, uint32_t *valLen);
  static inline std::string  stringify (Napi::Env env, Napi::Value value);
  static inline Napi::Value  parse     (Napi::Env env, const char* data);
  static Napi::FunctionReference Parse;
  static Napi::FunctionReference Stringify;
  static Napi::FunctionReference Log; };

class XBase {
public:
  uint32_t queryFlags;
  ups_db_t *keys;
  ups_db_t *data; };

class XCursor : public Napi::ObjectWrap<XCursor>, public XBase {
public:
  static Napi::FunctionReference  constructor;
  static Napi::Object        Init (Napi::Env env, Napi::Object exports);
                          XCursor (const Napi::CallbackInfo& info);
                         ~XCursor ();
  inline bool                move (const Napi::CallbackInfo& info, uint32_t flags);
  inline void               close (void);
                            char* key;
                            char* current_key;
                              int length;
private:
  Napi::Value                 New (const Napi::CallbackInfo& info);
  Napi::Value               Close (const Napi::CallbackInfo& info);
  Napi::Value                Next (const Napi::CallbackInfo& info);
  Napi::Value                Prev (const Napi::CallbackInfo& info);
  Napi::Value               First (const Napi::CallbackInfo& info);
  Napi::Value                Last (const Napi::CallbackInfo& info);
  inline void          invalidate (const Napi::CallbackInfo& info);
                          xtable* parent;
                    ups_cursor_t* cur;
                         uint32_t recordId;
                         uint32_t firstRecordId;
                             bool open = false; };

class XTable : public XBase {
public:
  uint32_t indexFlags;
  Napi::Value Find (const Napi::CallbackInfo& info);
  Napi::Value Each (const Napi::CallbackInfo& info); };

class XScale : public Napi::ObjectWrap<XScale>, public XTable {
public:
  static Napi::Object       Init (Napi::Env env, Napi::Object exports);
                          XScale (const Napi::CallbackInfo& info);
                         ~XScale ();
  static Napi::Value    flushAll (const Napi::CallbackInfo& info);
  static Napi::Value    closeAll (const Napi::CallbackInfo& info);
  inline void              close (void);
                      ups_env_t* env;
                             int indexCount = 0;
private:
  static Napi::FunctionReference constructor;
  Napi::Value              Close (const Napi::CallbackInfo& info);
  Napi::Value                Set (const Napi::CallbackInfo& info);
  Napi::Value                Get (const Napi::CallbackInfo& info);
  Napi::Value                Del (const Napi::CallbackInfo& info);
  Napi::Value        DefineIndex (const Napi::CallbackInfo& info);
                         xindex* index[TABLE_MAX];
                           char* path = NULL;
                            bool open = false;
                             int id; };

class XIndex : public Napi::ObjectWrap<XIndex>, public XTable {
public:
  static Napi::FunctionReference constructor;
  static Napi::Object        Init (Napi::Env env, Napi::Object exports);
  static Napi::Object NewInstance (Napi::Env env, xscale* parent, const char* name, uint32_t flags);
                           XIndex (const Napi::CallbackInfo& info);
  void                      setup (xscale* parent, const char* name, uint32_t flags);
                          ~XIndex ();
  inline void                 set (Napi::Env env, uint32_t recordId, Napi::Object Subject);
  inline void             setBool (Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void             setDate (Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void           setString (Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void      setStringArray (Napi::Env env, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void                 del (Napi::Env env, uint32_t recordId, Napi::Object Subject);
  inline void             delBool (ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void             delDate (ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void           delString (Napi::Env env, ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void      delStringArray (Napi::Env env, ups_cursor_t *cursor, uint32_t recordId, Napi::Object Subject, Napi::String IndexKey);
  inline void      innerDelString (ups_cursor_t *cursor, uint32_t recordId, const char* value);
                          xscale* parent;
private:
  Napi::Value                 Get (const Napi::CallbackInfo& info);
  Napi::Value                 Set (const Napi::CallbackInfo& info);
  Napi::Value                 Del (const Napi::CallbackInfo& info);
  Napi::Value               Close (const Napi::CallbackInfo& info);
  inline void               close (void);
                            char* name = NULL;
                             bool open = false; };

#endif
