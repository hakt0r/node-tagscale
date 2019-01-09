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

var fs = require('fs');
var cp = require('child_process');
var path = require('path');

var LD  = process.env.LD  || 'ld';
var CC  = process.env.CC  || 'gcc';
var CXX = process.env.CXX || 'g++';
var CPP = process.env.CPP || 'cpp';

cp.spawn( "sh",['-c',`
[ -n "$KEEP_FILES" ] && {
  touch KEEP_FILES; export FROM_SOURCE=yes; }
ccache=$(which ccache 2>/dev/null);
diet=$(which diet 2>/dev/null);
if ! echo "$CC" | grep -q "$ccache"
then
  export CC="$ccache  $diet ${CC}  -fPIC";
  export LD="$ccache  $diet ${LD}  -fPIC";
  export CXX="$ccache $diet ${CXX} -fPIC";
  export CPP="$ccache $diet ${CPP}";
  export ARFLAGS=cr;
fi
proc="-j$(nproc||echo 2)";

[ -d node_modules/mocha ] || npm i
[ -f ./dest/lib/libupscaledb.a ] && node-gyp configure && exit 0;

[ -d ./upscaledb ] ||
  git clone --depth=1 https://github.com/cruppstahl/upscaledb;

cd upscaledb;
  mkdir -p dest;

  [ -f ./configure ] || bash ./bootstrap.sh;

  [ -f ./Makefile  ] || ./configure \
    --disable-shared --enable-static-boost --with-pic=static \
    --prefix=${path.join(__dirname,'upscaledb','dest')} \
    --disable-java --disable-encryption --disable-remote

  [ -f ./src/libupscaledb.la ] || {
    make --trace -C 3rdparty $proc;
    make --trace -C src      $proc; }

  [ -f ./dest/lib/libupscaledb.a  ] ||
    make --trace -C src install $proc;

cd ..;
node-gyp configure;
`],{stdio:'inherit'})
