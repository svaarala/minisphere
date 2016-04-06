/**
 *  Sphere 1.x tileset format schema CommonJS module
 *  a schema for reading Sphere 1.x tileset files using miniRT/binary
 *  (c) 2015-2016 Fat Cerberus
**/

module.exports =
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
		{ id: 'nextTile', type: 'uintLE', size: 2 },
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
