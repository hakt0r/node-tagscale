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

var ts = require('../');
var assert = require('assert');
var fs = require('fs');
var db = null;

var obj1 = {tag:['news','test','hash'],body:'test'};
var obj2 = {tag:['hash'],body:'test2'};
var obj3 = {tag:['hash'],body:'test3'};

var COUNT = 10000, i = 0, keys, rk, rks, vals;
keys = new Array(COUNT); vals = new Array(COUNT);
fnv = function(s) {
  var h = 0, i = 0; s = s.toString();
  while (i < s.length) {
    h ^= s.charCodeAt(i);
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    i++; }
  return (h >>> 0).toString(36); }
rk = function() { return fnv((Math.random() * 10000000).toString()) }
while (i < COUNT) {
  keys[i] = rk();
  vals[i] = {
    tag: [rk(), rk(), rk(), 'news'],
    body: rk() };
  i++; }

var tags = null;

describe('ts.TagStore', function() {
  it('open database', function() {
    fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
    tags = new ts.TagStore('test.db')
    assert.notStrictEqual(tags,false);
  });
  it('insert an object', function() {
    assert.equal(tags.set("test",obj1),true);
  });
  it('return an equal object', function() {
    assert.deepEqual(tags.get("test"),obj1)
  });
  it('reopen', function() {
    assert.equal(tags.close(),true);
    assert.notStrictEqual(tags.open(),false);
  });
  it('record persistence', function() {
    assert.deepEqual(tags.get("test"),obj1)
  });
  it('remove the object', function() {
    assert.equal(tags.del("test"),true);
  });
  it('return nothing for that key now', function() {
    assert.equal(tags.get("test"),false);
  });
  it('update an object', function() {
    assert.equal(tags.set("test",obj1),true)
    assert.deepEqual(tags.get("test"),obj1)
    assert.equal(tags.set("test",obj2),true)
    assert.deepEqual(tags.get("test"),obj2)
    assert.equal(tags.set("test",obj1),true)
    assert.deepEqual(tags.get("test"),obj1)
  });
  it('survive a little (write) stress test', function() {
    var i, j, ref, t = Date.now();
    for (i = j = 0, ref = COUNT; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
      tags.set(keys[i], vals[i]);
    console.log('      @\x1b[35m'+COUNT+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / COUNT, 'ms/rec', diff, 'ms total');
  });
  it('survive a little (get) stress test', function(done) {
    var i, j, ref, t = Date.now();
    for (i = j = 0, ref = COUNT = 10000; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
      tags.get(keys[i]);
    console.log('      @\x1b[35m'+COUNT+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / COUNT, 'ms/rec', diff, 'ms total');
    done();
  });
  it('survive a little (del) stress test', function(done) {
    var i, j, ref, t = Date.now();
    for (i = j = 0, ref = COUNT = 2000; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
      tags.del(keys[i]);
    console.log('      @\x1b[35m'+COUNT+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / COUNT, 'ms/rec', diff, 'ms total');
    done();
  });
  it('close database', function() {
   assert.equal(tags.close(),true);
  });
});

var cursor = null;
describe('ts.TagStore.byTag', function() {
  it('prepare database', function() {
    fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
    assert.notStrictEqual(tags.open(),false);
    tags.set('testTags1',obj1);
    tags.set('testTags2',obj2);
    tags.set('testTags3',obj3);
  });
  it('open a cursor', function() {
    cursor = tags.byTag('hash');
    assert.notStrictEqual(cursor,false);
  });
  it('close that cursor', function() {
    assert.notStrictEqual(cursor.close(),false);
  });
  it('find tags and walk on them', function() {
    a = tags.byTag('hash'); a1 = a.current;
    assert.notEqual(a1.tag.indexOf('hash'),-1);
    a.next(); a2 = a.current;
    assert.notEqual(a2.tag.indexOf('hash'),-1);
    assert.notDeepEqual(a1,a2);
    a.next(); a3 = a.current;
    assert.notEqual(a3.tag.indexOf('hash'),-1);
    assert.notDeepEqual(a2,a3);
    a.next();
    assert.deepEqual(a.current,false);
    a.close()
  });
  it('dont find non-existing tags', function() {
    assert.equal(tags.byTag('foo'),false)
  });
  it('should be removed', function() {
    tags.set('testTags4',{tag:['uniqueTag']});
    c = tags.byTag('uniqueTag'); c.close();
    tags.del('testTags4');
    assert.equal(c = tags.byTag('uniqueTag'),false);
  });
  it('multiple cursors', function() {
    a = tags.byTag('hash'); assert.notEqual(a.current.tag.indexOf('hash'),-1)
    b = tags.byTag('news'); assert.notEqual(b.current.tag.indexOf('news'),-1)
    a.close(); b.close()
  });
  it('close database', function() {
    assert.equal(tags.close(),true);
  });
});

var table = null;
describe('ts.Table', function() {
  it('open database', function() {
    fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
    assert.notStrictEqual(table = new ts.Table('test.db'),false);
  });
  it('close database', function() {
   assert.equal(table.close(),true);
  });
  it('re-open database', function() {
    assert.notStrictEqual(table.open(),false);
  });
  it('set value', function() {
    assert.notStrictEqual(table.set('test1',obj1),false);
  });
  it('get value', function() {
    assert.deepEqual(table.get('test1'),obj1);
  });
  it('value type: object', function() {
    assert.notStrictEqual(table.set('test-object',obj1),false);
    assert.deepEqual(table.get('test-object'),obj1);
  });
  it('value type: string', function() {
    assert.notStrictEqual(table.set('test-string','Hello World!'),false);
    assert.strictEqual(table.get('test-string'),'Hello World!');
  });
  it('value type: number', function() {
    assert.notStrictEqual(table.set('test-number',1337),false);
    assert.strictEqual(table.get('test-number'),1337);
  });
  it('value type: boolean', function() {
    assert.notStrictEqual(table.set('test-bool',true),false);
    assert.strictEqual(table.get('test-bool'),true);
    assert.notStrictEqual(table.set('test-bool',false),false);
    assert.strictEqual(table.get('test-bool'),false);
  });
  it('delete value', function() {
    assert.strictEqual(table.del('test1'),true);
    assert.strictEqual(table.get('test1'),false);
  });
  it('persistence', function() {
    assert.notStrictEqual(table.set('test1',obj1),false);
    assert.equal(table.close(),true);
    assert.notStrictEqual(table.open(),false);
    assert.deepEqual(table.get('test1'),obj1);
  });
  it('replace value', function() {
    assert.notStrictEqual(table.set('test1',obj2),false);
    assert.deepEqual(table.get('test1'),obj2);
  });
  it('survive a little (write) stress test', function() {
    var i, j, ref, t = Date.now(); COUNT = 10000;
    for (i = j = 0, ref = COUNT; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
      table.set(keys[i], vals[i]);
    console.log('      @\x1b[35m'+COUNT+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / COUNT, 'ms/rec', diff, 'ms total');
  });
  it('survive a little (get) stress test', function(done) {
    var i, j, ref, t = Date.now();
    for (i = j = 0, ref = COUNT = 10000; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
      table.get(keys[i]);
    console.log('      @\x1b[35m'+COUNT+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / COUNT, 'ms/rec', diff, 'ms total');
    done();
  });
  it('survive a little (del) stress test', function(done) {
    var i, j, ref, t = Date.now();
    for (i = j = 0, ref = COUNT; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
      table.del(keys[i]);
    console.log('      @\x1b[35m'+COUNT+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / COUNT, 'ms/rec', diff, 'ms total');
    done();
  });
  it('close database', function() {
   assert.equal(table.close(),true);
   fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
  });
});

var table1, table2, table3, table4;
describe('ts.Joint', function() {
  it('Open several TagStores and Tables', function() {
    fs.existsSync('./test.db')  && fs.unlinkSync('./test.db');
    fs.existsSync('./test1.db') && fs.unlinkSync('./test1.db');
    fs.existsSync('./test2.db') && fs.unlinkSync('./test2.db');
    fs.existsSync('./test3.db') && fs.unlinkSync('./test3.db');
    assert.notStrictEqual(table1 = new ts.Table('test.db'),false);
    assert.notStrictEqual(table2 = new ts.Table('test1.db'),false);
    assert.notStrictEqual(table3 = new ts.Table('test2.db'),false);
    assert.notStrictEqual(table4 = new ts.Table('test3.db'),false);
  });
  it('Close them all', function() {
    assert.notStrictEqual(table1.close(),false);
    assert.notStrictEqual(table2.close(),false);
    assert.notStrictEqual(table3.close(),false);
    assert.notStrictEqual(table4.close(),false);
    fs.existsSync('./test.db')  && fs.unlinkSync('./test.db');
    fs.existsSync('./test1.db') && fs.unlinkSync('./test1.db');
    fs.existsSync('./test2.db') && fs.unlinkSync('./test2.db');
    fs.existsSync('./test3.db') && fs.unlinkSync('./test3.db');
  });
});
