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

module.exports = exports = require('bindings')('NativeExtension');

exports.DATE = 1
exports.STRING = 2
exports.STRING_ARRAY = 3

exports.TagScale = class TagScale extends exports.XScale {
  constructor(path){
    super(path);
    this.defineIndex('tag',3);
}}
