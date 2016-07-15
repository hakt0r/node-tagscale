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

"use strict";

var ts = require('bindings')('NativeExtension');

class TagStore {
  constructor(path){
    this.path = path;
    if ( false === this.open() ) return false; }
  open(){ return this.id = ts.tags_open(this.path); }
  close(){          return ts.tags_close(this.id); }
  set(key,value){   return ts.tags_set(this.id,key,value); }
  get(key){         return ts.tags_get(this.id,key); }
  del(key){         return ts.tags_del(this.id,key); }
  byTag(key,flags){
    var c = new Cursor(this,key,flags);
    return c.id === false ? false : c; }}

class Table {
  constructor(path){
    this.path = path;
    if ( false === this.open() ) return false; }
  open(){ return this.id = ts.table_open(this.path); }
  close(){          return ts.table_close(this.id); }
  set(key,value){   return ts.table_set(this.id,key,value); }
  get(key){         return ts.table_get(this.id,key); }
  del(key){         return ts.table_del(this.id,key); }}

class Cursor {
  constructor(db,key){
    this.db = db;
    this.DB = db.id;
    this.key = key;
    var res = ts.find( this.DB, this.key, ( rec, uid ) => {
      this.current = rec; this.id = uid; })
    if ( res == false ) this.id = res; }
  first(){ return this.current = ts.next(this.DB, this.id, 1); }
  last(){  return this.current = ts.next(this.DB, this.id, 2); }
  next(){  return this.current = ts.next(this.DB, this.id, 4); }
  prev(){  return this.current = ts.next(this.DB, this.id, 8); }
  close(){ this.current = null; return ts.next(this.DB, this.id, 0); }}

ts.Table    = Table;
ts.Cursor   = Cursor;
ts.TagStore = TagStore;

module.exports = ts;
