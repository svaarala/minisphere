/**
 *  miniRT/tiles CommonJS module
 *  a JavaScript tileset engine for Sphere 2.0
 *  (c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
    throw new TypeError("script must be loaded using require()");
}

var binary = require('./binary');

var rtsSchema =
[
	{ id: 'signature', type: 'fstring', size: 4, regex: "\\.rts" },
	{ id: 'version', type: 'uintLE', size: 2, range: [ 1, 1 ] },
	{ id: 'numTiles', type: 'uintLE', size: 2 },
	{ id: 'tileWidth', type: 'uintLE', size: 2 },
	{ id: 'tileHeight', type: 'uintLE', size: 2 },
	{ id: 'bpp', type: 'uintLE', size: 2, values: [ 32 ] },
	{ id: 'compressed', type: 'bool', values: [ false ] },
	{ id: 'hasObstructions', type: 'bool' },
	{ type: 'reserved', size: 240 },
	{ id: 'images', type: 'array', count: '@numTiles', subtype: { type: 'image', width: '@tileWidth', height: '@tileHeight' } },
	{ id: 'tiles', type: 'array', count: '@numTiles', subtype: { type: 'object', schema: [
		{ type: 'reserved', size: 1 },
		{ id: 'animated', type: 'bool' },
		{ id: 'nextTileIndex', type: 'uintLE', size: 2 },
		{ id: 'delay', type: 'uintLE', size: 2 },
		{ type: 'reserved', size: 1 },
		{ id: 'obstructionType', type: 'uintLE', size: 1 },
		{ id: 'numSegments', type: 'uintLE', size: 2 },
		{ id: 'nameLength', type: 'uintLE', size: 2 },
		{ id: 'terraformed', type: 'bool' },
		{ type: 'reserved', size: 19 },
		{ id: 'name', type: 'fstring', size: '@nameLength' },
		{ id: 'obsmap', type: 'switch', field: 'obstructionType', cases: [
			{ value: 0, type: 'reserved', size: 0 },
			{ value: 2, type: 'array', count: '@numSegments', subtype: { type: 'object', schema: [
				{ id: 'x1', type: 'uintLE', size: 2 },
				{ id: 'y1', type: 'uintLE', size: 2 },
				{ id: 'x2', type: 'uintLE', size: 2 },
				{ id: 'y2', type: 'uintLE', size: 2 },
			]}},
		]},
	]}},
];

module.exports = (function() {
	return {
		TileSet: TileSet,
	};

	function TileSet(fileName)
	{
		var rts = binary.load(fileName, rtsSchema);
		
		var atlasSize = Math.ceil(Math.sqrt(rts.numTiles));
		var atlas = new Surface(rts.tileWidth * atlasSize, rts.tileHeight * atlasSize);
		for (var i = 0; i < rts.numTiles; ++i) {
			var x = (i % atlasSize) * rts.tileWidth;
			var y = Math.floor(i / atlasSize) * rts.tileHeight;
			atlas.blitSurface(rts.images[i].createSurface(), x, y);
		}
		atlas = new Image(atlas);
		
		var tiles = [];
		for (var i = 0; i < rts.numTiles; ++i) {
			tiles.push({
				tileIndex:     i,
				nextTileIndex: rts.tiles[i].nextTileIndex,
				framesLeft:    rts.tiles[i].delay,
			});
		}

		this.atlas = atlas;
		this.tileName = tileName;
		this.update = update;
		this.uv = uv;

		function uv(tileIndex)
		{
			var width = rts.tileWidth * atlasSize;
			var height = rts.tileHeight * atlasSize;
			var u1 = (tileIndex % atlasSize) * rts.tileWidth;
			var v1 = Math.floor(tileIndex / atlasSize) * rts.tileHeight;
			var u2 = u1 + rts.tileWidth;
			var v2 = v1 + rts.tileHeight;
			return [
				{ u: u1, v: v1 },
				{ u: u2, v: v1 },
				{ u: u2, v: v2 },
				{ u: u1, v: v2 },
			]
		}
		
		function tileName(tileIndex)
		{
			if (tileIndex < 0 || tileIndex >= rts.numTiles)
				throw new RangeError("tile index is out of bounds");
			return rts.tiles[tiles[tileIndex].tileIndex].name;
		}

		function update()
		{
			for (var i = tiles.length - 1; i <= 0; --i) {
				if (--tiles[i].framesLeft == 0) {
					var newIndex = tiles[i].nextTileIndex;
					tiles[i].tileIndex = newIndex;
					tiles[i].nextTileIndex = rts.tiles[newIndex].nextTileIndex;
					tiles[i].framesLeft = rts.tiles[newIndex].delay;
				}
			}
		}
	}
})();
