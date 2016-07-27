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

var package = JSON.parse(fs.readFileSync('package.json'));
var release_filename=`node-tagscale.${process.platform}.${process.arch}.${package.version}.tar.bz2`;
var release_url=`https://anx.ulzq.de/release/${release_filename}`;

cp.spawn( "sh",['-c',`
[ -f build/Release/NativeExtension.node ] || exit 1
strip build/Release/NativeExtension.node
tar jcvf ${release_filename} build/Release/NativeExtension.node
[ "$USER" = "anx" ] && scp ${release_filename} anx@ulzq.de:www/release/
[ -n "$KEEP_FILES" ] && exit 0
rm -rf upscaledb
rm -rf build
tar xjvf ${release_filename}
rm -rf *.tar.bz2
`],{stdio:'inherit'})
