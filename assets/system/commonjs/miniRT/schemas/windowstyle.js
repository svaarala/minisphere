/**
 *  Sphere 1.x windowstyle format schema CommonJS module
 *  a schema for reading Sphere 1.x tileset files using miniRT/binary
 *  (c) 2015-2016 Fat Cerberus
**/

module.exports =
[
	{ id: 'signature', type: 'fstring', size: 4, regex: "\\.rws" },
	{ id: 'version', type: 'uintLE', size: 2, range: [ 1, 2 ] },
	{ id: 'edgeWidth', type: 'uintLE', size: 1 },
	{ id: 'backStyle', type: 'uintLE', size: 1 },
	{ id: 'edgeColors', type: 'array', count: 4, subtype: { type: 'color' } },
	{ id: 'edgeOffsets', type: 'array', count: 4, subtype: { type: 'uintLE', size: 1 } },
	{ type: 'reserved', size: 36 },
	{ id: 'data', type: 'switch', field: 'version', cases: [
		{ value: 1, type: 'object', schema: [
			{ id: 'images', type: 'array', count: 9, subtype: { type: 'image', width: '@edgeWidth', height: '@edgeWidth' } },
		]},
		{ value: 2, type: 'object', schema: [
			{ id: 'bitmaps', type: 'array', count: 9, subtype: { type: 'object', schema: [
				{ id: 'width', type: 'uintLE', size: 2 },
				{ id: 'height', type: 'uintLE', size: 2 },
				{ id: 'image', type: 'image', width: "@width", height: "@height" },
			]}},
		]},
	]},
];
