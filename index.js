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

"use strict";

module.exports = exports = require('bindings')('NativeExtension');

exports.XSCALE       = 1
exports.KEYVAL       = 2
exports.BOOL         = 4
exports.DATE         = 8
exports.STRING       = 16
exports.STRING_ARRAY = 32

exports.XScale.create = function(path){
  const fs = require('fs');
  if ( fs.existsSync(path) ) fs.unlinkSync(path);
  return new exports.XScale(path);
}

exports.TagScale = class TagScale extends exports.XScale {
  constructor(path){
    super(path);
    this.defineIndex('tag',3);
}}

process.on( 'exit', function(){ exports.closeAll() } )
