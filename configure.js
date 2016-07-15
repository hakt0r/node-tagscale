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

var fs = require('fs');
var cp = require('child_process');
var path = require('path');

cp.spawn( "sh",['-c',"\
  [ -f ./libupscaledb.so.0 ] && node-gyp configure && exit 0;                             \
  [ -f /usr/local/lib/upscaledb.so.0 ] && {                                               \
     ln -s /usr/local/lib/upscaledb* ./;                                                  \
     ln -s /usr/local/include        ./; };                                               \
  [ -f /usr/lib/upscaledb.so.0 ] && {                                                     \
     ln -s /usr/lib/upscaledb* ./;                                                        \
     ln -s /usr/include        ./; };                                                     \
  [ -f ./libupscaledb.so.0 ] && node-gyp configure && exit 0;                             \
  [ -d ./upscaledb ] || git clone --depth=1 https://github.com/cruppstahl/upscaledb;      \
  cd upscaledb; mkdir -p dest;                                                            \
  [ -f ./configure ] || bash ./bootstrap.sh;                                              \
  [ -f ./Makefile  ] || ./configure --prefix="+path.join(__dirname,'upscaledb','dest')+"; \
  [ -f ./src/libupscaledb.la ] || make -j$(nproc||echo 2);                                \
  [ -f ./dest/lib/libupscaledb.so ] || make install;                                      \
  [ -d ../include ] || cp -r ./include ../;                                               \
  [ -f ../libupscaledb.so ] ||                                                            \
    cp $(realpath ./dest/lib/libupscaledb.so) ../libupscaledb.so;                         \
  [ -f ../libupscaledb.so.0 ] || ln -s libupscaledb.so ../libupscaledb.so.0;              \
  cd ..; node-gyp configure;                                                              \
  [ -f ../libupscaledb.so.0 ] && rm -rf upscaledb                                         \
"],{stdio:'inherit'})
