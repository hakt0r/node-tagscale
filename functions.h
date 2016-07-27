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

static const int TAGS_MAX   = 64;
static const int TABLE_MAX  = 64;
static const int CURSOR_MAX = 128;

struct tagstoreHandle{
  int id; char *path; ups_env_t *env; ups_db_t *byKey; ups_db_t *byUID; ups_db_t *byDate; ups_db_t *byTag; };

struct tableHandle{
  int id; ups_env_t* env; ups_db_t* db; char * path; };

struct cursorHandle{
  int id; ups_cursor_t* cur; char * key; int length; };

static tagstoreHandle *TAGS[TAGS_MAX];
static tableHandle    *TABLE[TABLE_MAX];
static cursorHandle   *CURSOR[CURSOR_MAX];

void tagscale_init(void);
void tagscale_flushAll(void);
void tagscale_closeAll(void);

NAN_METHOD(upb_table_open);
NAN_METHOD(upb_table_close);
NAN_METHOD(upb_table_set);
NAN_METHOD(upb_table_get);
NAN_METHOD(upb_table_del);

NAN_METHOD(upb_tags_open);
NAN_METHOD(upb_tags_close);
NAN_METHOD(upb_tags_set);
NAN_METHOD(upb_tags_get);
NAN_METHOD(upb_tags_del);

NAN_METHOD(upb_find);
NAN_METHOD(upb_next);

NAN_METHOD(upb_flushAll);
NAN_METHOD(upb_closeAll);

#endif
