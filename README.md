# tagscale

node**JS** extension utilizing [upscale**db**](https://upscaledb.com/) to store JSON-documents including a tag-index.
Includes a common Key-value-store.

This is a synchronous API, no threading - nothing.

Also, consider this *experimental, loosely tested, slow and sloppy*, test it well before you use it.
And of course - share your patches!!!11elf

## Installing

```ShellSession
$ npm i git+https://github.com/hakt0r/node-tagscale
```

## Using

```JavaScript
var ts = require('tagscale');

// Simple key-value store like leveldb, etc.
if ( table = new ts.Table('somefile.db') ){
  table.set('test1',"Hello World!");
  console.log(table.get('test1')); // "Hello World!"
  table.set('test1',"Hello Universe!");
  console.log(table.get('test1')); // "Hello Universe!"
  table.del('test1');
  console.log(table.get('test1')); // false
  table.close(); }

// Tag-indexed database
if ( tags = new ts.TagStore('somefile.db') ){
  tags.set('test1',{{tag:"news","tagscale"},body:'Hello World!'});
  tags.set('test2',{{tag:"news","hashtags"},body:'Hello Universe!'});
  tags.set('test3',{{tag:"news","forall!!"},body:'Hello Code!'});
  console.log(tags.get('test1')); // {{tag:"news","tagscale"},body:'Hello World!'}
  tags.del('test3');
  console.log(tags.get('test3')); // false
  if ( news = tags.byTag('news') ){
    console.log(news.current); // {{tag:"news","tagscale"},body:'Hello World!'}
    news.next();
    console.log(news.current); // {{tag:"news","hashtags"},body:'Hello Universe!'}
    news.prev();
    console.log(news.current); // {{tag:"news","tagscale"},body:'Hello World!'}
  tags.close(); }}
```

## Building

To compile the extension for the first time, run:

```ShellSession
$ npm i
```

All subsequent builds *only* need `npm run build`

### Dependencies

  - node**JS** 4+
  - node-gyp
  - c/c++ compiler suite
  - ccache (optional)
  - dietlibc (optional)

Compiles upscaledb ad-hoc if no library is found on the system;
  the Author names the following (debian) packages as dependencies.
  - libgoogle-perftools-dev
  - libboost-filesystem-dev
  - libboost-thread-dev
  - libboost-dev
  - libuv ( >1.0.0 - from debian testing seems to suffice, in my experience )

Not needed from my side and optional:
  - libdb-dev
  - protobuf-compiler
  - libprotobuf-dev

## Testing

A very basic test suite is included. Feel free to have a glance.

```ShellSession
$ npm run test

  building [...]

  ts.TagStore
    ✓ open database
    ✓ insert an object
    ✓ return an equal object
    ✓ reopen
    ✓ record persistence
    ✓ remove the object
    ✓ return nothing for that key now
    ✓ update an object
      @10000recs 0.0746 ms/rec 746 ms total
    ✓ survive a little (write) stress test (749ms)
      @10000recs 0.0141 ms/rec 141 ms total
    ✓ survive a little (get) stress test (145ms)
      @2000recs 0.5935 ms/rec 1187 ms total
    ✓ survive a little (del) stress test (1188ms)
    ✓ close database (92ms)

  ts.TagStore.byTag
    ✓ prepare database
    ✓ open a cursor
    ✓ close that cursor
    ✓ find tags and walk on them
    ✓ dont find non-existing tags
    ✓ should be removed
    ✓ multiple cursors
    ✓ close database

  ts.Table
    ✓ open database
    ✓ close database
    ✓ re-open database
    ✓ set value
    ✓ get value
    ✓ value type: object
    ✓ value type: string
    ✓ value type: number
    ✓ value type: boolean
    ✓ delete value
    ✓ persistence
      @10000recs 0.0246 ms/rec 246 ms total
    ✓ survive a little (write) stress test (246ms)
      @10000recs 0.0125 ms/rec 125 ms total
    ✓ survive a little (get) stress test (125ms)
      @10000recs 0.0044 ms/rec 44 ms total
    ✓ survive a little (del) stress test (44ms)
    ✓ close database

  31 passing (3s)
```

## License

tag**scale**

  nodeJS key-value-store with secondary indices (tags) based on upscaledb

  Copyright &copy; 2016 Sebastian Glaser

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see &lt;http://www.gnu.org/licenses/&gt;.
