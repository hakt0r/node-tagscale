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
var https = require('https');

var package = JSON.parse(fs.readFileSync('package.json'));
var release_filename=`node-tagscale.${process.platform}.${process.arch}.${package.version}.tar.bz2`;
var release_url=`https://anx.ulzq.de/release/${release_filename}`;

console.log(`Downloading ${release_url}`);
var download = function(url, dest, cb) {
  var file = fs.createWriteStream(dest);
  var request = https.get(url, function(response) {
    response.pipe(file);
    console.log(`Unpacking ${release_filename}`);
    file.on('finish', function() { file.close(cb); });
}); }

download(release_url, release_filename, function(){
  cp.spawn( "tar",['xjvf',release_filename],{stdio:'inherit'});
});
