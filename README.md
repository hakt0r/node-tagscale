# tagscale

node**JS** extension utilizing [upscale**db**](https://upscaledb.com/) to store JSON-documents including a tag-index.

Can be used a common Key-value-store.

This is a synchronous API, no threading - nothing.

Also, consider this *experimental, loosely tested, slow and sloppy*, test it well before you use it.
And of course - share your patches!!!11elf

## Installing

```ShellSession
$ npm i --save git+https://github.com/hakt0r/node-tagscale
```

## Using

```JavaScript
var ts = require('tagscale');

// Simple key-value store like leveldb, etc.
if ( table = new ts.XScale('somefile.db') ){
  table.set('test1',"Hello World!");
  console.log(table.get('test1')); // "Hello World!"
  table.set('test1',"Hello Universe!");
  console.log(table.get('test1')); // "Hello Universe!"
  table.del('test1');
  console.log(table.get('test1')); // false
  table.close(); }

// Tag-indexed database
if ( db = new ts.XScale('somefile.db') ){
  db.defineIndex('tag',ts.STRING_ARRAY);
  db.set('test1',{{tag:"news","tagscale"},body:'Hello World!'});
  db.set('test2',{{tag:"news","hashtags"},body:'Hello Universe!'});
  db.set('test3',{{tag:"news","forall!!"},body:'Hello Code!'});
  console.log(db.get('test1')); // {{tag:"news","tagscale"},body:'Hello World!'}
  db.del('test3');
  console.log(db.get('test3')); // false
  if ( news = db.tag.find('news') ){
    console.log(news.current); // {{tag:"news","tagscale"},body:'Hello World!'}
    news.next();
    console.log(news.current); // {{tag:"news","hashtags"},body:'Hello Universe!'}
    news.prev();
    console.log(news.current); // {{tag:"news","tagscale"},body:'Hello World!'}
  db.close(); }}
```
## API

#### *class* **XScale**
  - ***constructor***(path)
    - *string* **path** - full or relative path to database file
  - ***close***() - flush and close database and all it's indexes
  - ***set***(key,value)
    - *string* **key** - primary key (as in unique)
    - *object* **value** - the associated object (must be an object)
    - return (*number* or false);
  - ***get***(key)
    - *string* **key** - primary key (as in unique)
    - return (*object* or false);
  - ***del***(key)
    - *string* **key** - primary key (as in unique)
    - return *boolean*;
  - ***find***(key)
    - *string* **key** - primary key (as in unique)
    - return (*XCursor* or false);
  - ***defineIndex***(name,type)
    - *string* **name** - Name of the index and -OFC- the field to be indexed.
    - *number* **type** - Type of index you desire
      - 1: ts.**BOOL** - Records with ( key && key != false )
      - 1: ts.**DATE** - Attach a timestamp (automatic)
      - 2: ts.**STRING** - A String whose value should be indexed
      - 3: ts.**STRING_ARRAY** - An Array of strings - like tags ;>
    - return (*XIndex* or false);

#### *class* **XIndex**
- ***find***(key)
  - *string* **key** - the key to look up (not require fr ts.BOOL)
  - return (*XCursor* or false);

#### *class* **XCursor**
- ***length*** *number* - the number of keys initially matched
- ***current*** *object* - the current object
- ***close***()
  - return *boolean*
- ***next***()
  - return *boolean*
- ***prev***()
  - return *boolean*
- ***first***()
  - return *boolean*
- ***last***()
  - return *boolean*

## Building

To compile the extension for the first time, run:

```ShellSession

# first get node-tagscale
$ git clone https://github.com/hakt0r/node-tagscale --depth=1
$ cd node-tagscale

# Keep upscaledb after building - (saves alot of time ;)
$ touch KEEP_FILES

# Install from source only
$ FROM_SOURCE=yes npm i

# Try installing from repo first, then from source
$ npm i
```

All subsequent builds *only* need `npm run build`

### Dependencies (for Debian)

```ShellSession
# Don't forget
$ sudo apt install build-essential nodejs npm node-gyp

# Required dependencies
$ sudo apt install libgoogle-perftools-dev libboost-filesystem-dev libboost-thread-dev libboost-asio-dev libboost-dev

# Optionals
$ sudo apt install ccache dietlibc libdb-dev protobuf-compiler libprotobuf-dev
```

## Testing

A very basic test suite is included. Feel free to have a glance.

```ShellSession

$ npm run test

  building [...]

  ts.XScale
    ✓ open database

  [...]

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
