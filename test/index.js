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

var obj1 = {tag:['news','test','hash'],body:'test',fancy:true};
var obj2 = {tag:['hash'],body:'test2',fancy:"other"};
var obj3 = {tag:['hash','other'],body:'test3'};
var obj4 = {tag:[],body:'test4',fancy:false};

var COUNT = 1000, i = 0, keys, rk, rks, vals;
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

var tags = null, index = null;

function stress_diff(t,c){ setImmediate( function(){
  console.log('      @\x1b[35m'+c+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / c, 'ms/rec', diff, 'ms total');
})}

describe('ts.XScale', function() {
  it('open database', function() {
    fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
    tags = new ts.XScale('test.db');
    assert.notStrictEqual(tags,false);
  });
  it('insert a string', function() {
    assert.equal(tags.set("test","hello world"),true);
    assert.equal(tags.get("test"),"hello world");
  });
  it('insert a plain object', function() {
    assert.equal(tags.set("test",{}),true);
  });
  it('return an equal plain object', function() {
    assert.deepEqual(tags.get("test"),{})
  });
  it('insert an object', function() {
    assert.equal(tags.set("test",obj1),true);
  });
  it('return an equal object', function() {
    assert.deepEqual(tags.get("test"),obj1)
  });
  it('reopen', function() {
    assert.equal(tags.close(),true); tags = new ts.XScale('test.db');
    assert.equal(tags.close(),true); tags = new ts.XScale('test.db');
    assert.equal(tags.close(),true); tags = new ts.XScale('test.db');
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
    assert.equal(typeof tags.set("test",obj1) == 'number',true); assert.deepEqual(tags.get("test"),obj1);
    assert.equal(typeof tags.set("test",obj2) == 'number',true); assert.deepEqual(tags.get("test"),obj2);
    assert.equal(typeof tags.set("test",obj1) == 'number',true); assert.deepEqual(tags.get("test"),obj1);
  });
  it('survive a little (write) stress test', function(done) {
    this.timeout(5000); var i, COUNT = 10000, t = Date.now();
    for (i = 0; i < COUNT; i++) tags.set(keys[i], vals[i]);
    stress_diff(t,COUNT); done();
  });
  it('survive a little (get) stress test', function(done) {
    this.timeout(5000); var i, COUNT = 10000, t = Date.now();
    for (i = 0; i < COUNT; i++) tags.get(keys[i]);
    stress_diff(t,COUNT); done();
  });
  it('survive a little (del) stress test', function(done) {
    this.timeout(5000); var i, COUNT = 10000, t = Date.now();
    for (i = 0; i < COUNT; i++) tags.del(keys[i]);
    stress_diff(t,COUNT); done();
  });
  it('open a cursor', function() {
    assert.equal    ( tags.set("test",obj1)  > -1, true      ); // Insert an object we want to find.
    assert.equal    ( tags.set("test1",obj2) > -1, true      ); // Insert another object we want to find.
    assert.notEqual ( c = tags.find('test'),       false     ); // Moment of truth
    assert.notEqual ( c.current,                   undefined ); // Should be defined, i mean - we just inseted it ;)
    assert.equal    ( c.next(),                    true      ); // Any next item, we made sure ther is one.
  });
  it('close database', function() {
   assert.equal     ( tags.close(),                true      ); // Close the database and be done with it.
  });
});

var cursor = null, a1,a2,a3;

describe('ts.XIndex<BOOL>', function() {
  it('prepare database', function() {
    fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
    tags = new ts.XScale('test.db');
    assert.notStrictEqual( index = tags.defineIndex('fancy',ts.BOOL), false);
    tags.set('testBools1',obj1);
    tags.set('testBools2',obj2);
    tags.set('testBools4',obj3);
    tags.set('testBools3',obj4);
  });
  it('open a cursor', function() {
    cursor = tags.fancy.find(null);
    assert.notStrictEqual(cursor,false);
  });
  it('close that cursor', function() {
    assert.notStrictEqual(cursor.close(),false);
  });
  it('find tags and walk on them (1) - find()', function() {
    a = tags.fancy.find(null); a1 = a.current;
    assert.equal(a1.body,'test');
    assert.notEqual(a1.fancy,undefined);
  });
  it('find tags and walk on them (2) - next()', function() {
    assert.strictEqual(a.next(),true);
    assert.notEqual(a2 = a.current, undefined);
    assert.notDeepEqual(a1.body,a2.body);
    assert.notDeepEqual(a1.fancy,a2.fancy);
  });
  it('find tags and walk on them (3) - end of list', function() {
    assert.deepEqual( a.next(),  false);
    assert.deepEqual( a.close(), true);
  });
  it('should be removed', function() {
    tags.del('testBools1');
    c = tags.fancy.find(null);
    assert.equal(c.current.body,"test2");
    c.close();
  });
  it('multiple cursors', function() {
    tags.set('testBools1',obj1);
    a = tags.fancy.find(null); assert.notEqual(a.current, undefined)
    b = tags.fancy.find(null); assert.notEqual(b.current, undefined); b.next()
    assert.notEqual(b.current.fancy, a.current.fancy);
    a.close(); b.close()
  });
  it('close database', function() {
    assert.equal(tags.close(),true);
  });
});

describe('ts.XIndex<STRING>', function() {
  it('prepare database', function() {
    fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
    tags = new ts.XScale('test.db');
    assert.notStrictEqual( index = tags.defineIndex('body',ts.STRING), false);
    tags.set('testStrings1',obj1);
    tags.set('testStrings2',obj1);
    tags.set('testStrings3',obj3);
  });
  it('open a cursor', function() {
    cursor = tags.body.find('test');
    assert.notStrictEqual(cursor,false);
  });
  it('close that cursor', function() {
    assert.notStrictEqual(cursor.close(),false);
  });
  it('find tags and walk on them (1) - find()', function() {
    a = tags.body.find('test'); a1 = a.current;
    assert.notEqual(a1.tag.indexOf('hash'),-1);
  });
  it('find tags and walk on them (2) - next()', function() {
    assert.strictEqual(a.next(),true);
    assert.notEqual(a2 = a.current,undefined);
    assert.notEqual(a2.tag.indexOf('hash'),-1);
    assert.deepEqual(a1.tag,a2.tag);
    assert.deepEqual(a1.body,a2.body);
  });
  it('find tags and walk on them (4) - end of list', function() {
    assert.deepEqual(a.next(),false);
    assert.deepEqual(a.close(),true);
  });
  it('dont find non-existing tags', function() {
    assert.equal(tags.body.find('foo'),false)
  });
  it('should be removed', function() {
    tags.set('testStrings',{body:'uniqueString'});
    assert.notEqual(tags.body.find('uniqueString'),false);
    tags.del('testStrings');
    assert.equal(tags.get('testTags4'),false);
    assert.equal(tags.body.find('uniqueString'),false);
  });
  it('multiple cursors', function() {
    a = tags.body.find('test');  assert.notEqual(a.current, undefined)
    b = tags.body.find('test3'); assert.notEqual(b.current, undefined)
    assert.notEqual(b.current.body, a.current.body);
    a.close(); b.close()
  });
  it('close database', function() {
    assert.equal(tags.close(),true);
  });
});

describe('ts.XIndex<STRING_ARRAY>', function() {
  it('prepare database', function() {
    fs.existsSync('./test.db') && fs.unlinkSync('./test.db');
    tags = new ts.XScale('test.db');
    index = tags.defineIndex('tag',ts.STRING_ARRAY);
    tags.set('testTags1',obj1);
    tags.set('testTags2',obj2);
    tags.set('testTags3',obj3);
  });
  it('open a cursor', function() {
    cursor = tags.tag.find('hash');
    assert.notStrictEqual(cursor,false);
  });
  it('close that cursor', function() {
    assert.strictEqual(cursor.close(),true);
  });
  it('find tags and walk on them (1) - find()', function() {
    a = tags.tag.find('hash'); a1 = a.current;
    assert.notEqual(a1.tag.indexOf('hash'),-1);
  });
  it('find tags and walk on them (2) - next()', function() {
    assert.strictEqual(a.next(),true);
    assert.notEqual(a2 = a.current,undefined);
    assert.notEqual(a2.tag.indexOf('hash'),-1);
    assert.notDeepEqual(a1.tag,a2.tag);
    assert.notDeepEqual(a1.body,a2.body);
  });
  it('find tags and walk on them (3) - next()', function() {
    assert.strictEqual(a.next(),true);
    assert.notEqual(a3 = a.current,undefined);
    assert.notEqual(a3.tag.indexOf('hash'),-1);
    assert.notDeepEqual(a2.tag,a3.tag);
    assert.notDeepEqual(a2.body,a3.body);
  });
  it('find tags and walk on them (4) - end of list', function() {
    assert.deepEqual(a.next(),false);
    assert.deepEqual(a.close(),true);
  });
  it('dont find non-existing tags', function() {
    assert.equal(tags.tag.find('foo'),false)
  });
  it('should be removed', function() {
    tags.set('testStrings4',{tag:['uniqueTag']});
    assert.notEqual(tags.tag.find('uniqueTag'),false);
    tags.del('testStrings4');
    assert.equal(tags.get('testTags4'),false);
    assert.equal(tags.tag.find('uniqueTag'),false);
  });
  it('multiple cursors', function() {
    a = tags.tag.find('hash'); assert.notEqual(a.current.tag.indexOf('hash'),-1)
    b = tags.tag.find('news'); assert.notEqual(b.current.tag.indexOf('news'),-1)
    a.close(); b.close()
  });
  it('close database', function() {
    assert.equal(tags.close(),true);
  });
});

return


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
    var i, j, ref, t = Date.now(); COUNT = 10;
    for (i = j = 0, ref = COUNT; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
      table.set(keys[i], vals[i]);
    console.log('      @\x1b[35m'+COUNT+'\x1b[36mrecs\x1b[0m', (diff=Date.now()-t) / COUNT, 'ms/rec', diff, 'ms total');
  });
  it('survive a little (get) stress test', function(done) {
    var i, j, ref, t = Date.now();
    for (i = j = 0, ref = COUNT = 10; 0 <= ref ? j < ref : j > ref; i = 0 <= ref ? ++j : --j)
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

var table1, table2, table3, table4, table5;
describe('ts.Joint', function() {
  it('Open several XScales and Tables', function() {
    fs.existsSync('./test.db')  && fs.unlinkSync('./test.db');
    fs.existsSync('./test1.db') && fs.unlinkSync('./test1.db');
    fs.existsSync('./test2.db') && fs.unlinkSync('./test2.db');
    fs.existsSync('./test3.db') && fs.unlinkSync('./test3.db');
    fs.existsSync('./test4.db') && fs.unlinkSync('./test4.db');
    assert.notStrictEqual(table1 = new ts.Table('test.db'),false);
    assert.notStrictEqual(table2 = new ts.Table('test1.db'),false);
    assert.notStrictEqual(table3 = new ts.Table('test2.db'),false);
    assert.notStrictEqual(table4 = new ts.Table('test3.db'),false);
    assert.notStrictEqual(table5 = new ts.Table('test4.db'),false);
  });
  it('Add records and flush them to disk', function() {
    var sizeBefore = fs.statSync(table5.path).size;
    assert.strictEqual(table5.set('test123'),true);
    assert.strictEqual(ts.flushAll(),true);
    assert.strictEqual(fs.statSync(table5.path).size > sizeBefore, true);
  });
  it('Close some of them', function() {
    assert.notStrictEqual(table1.close(),false);
    assert.notStrictEqual(table2.close(),false);
    assert.notStrictEqual(table3.close(),false);
    assert.notStrictEqual(table4.close(),false);
  });
  it('Close them all', function() {
    fs.existsSync('./test.db')  && fs.unlinkSync('./test.db');
    fs.existsSync('./test1.db') && fs.unlinkSync('./test1.db');
    fs.existsSync('./test2.db') && fs.unlinkSync('./test2.db');
    fs.existsSync('./test3.db') && fs.unlinkSync('./test3.db');
    fs.existsSync('./test4.db') && fs.unlinkSync('./test4.db');
  });
});
