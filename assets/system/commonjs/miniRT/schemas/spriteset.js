/**
 *  Sphere 1.x tileset format schema CommonJS module
 *  a schema for reading Sphere 1.x tileset files using miniRT/binary
 *  (c) 2015-2016 Fat Cerberus
**/

module.exports =
[
	{ id: 'signature', type: 'fstring', size: 4, regex: "\\.rss" },
	{ id: 'version', type: 'uintLE', size: 2, range: [ 1, 3 ] },
	{ id: 'numImages', type: 'uintLE', size: 2 },
	{ id: 'frameWidth', type: 'uintLE', size: 2 },
	{ id: 'frameHeight', type: 'uintLE', size: 2 },
	{ id: 'numPoses', type: 'uintLE', size: 2 },
	{ id: 'base', type: 'object', schema: [
		{ id: 'x1', type: 'uintLE', size: 2 },
		{ id: 'y1', type: 'uintLE', size: 2 },
		{ id: 'x2', type: 'uintLE', size: 2 },
		{ id: 'y2', type: 'uintLE', size: 2 },
	]},
	{ type: 'reserved', size: 106 },
	{ id: 'data', type: 'switch', field: 'version', cases: [
		{ value: 1, type: 'object', schema: [
			{ id: 'images', type: 'array', count: 64, subtype: { type: 'image', width: '@frameWidth', height: '@frameHeight' } },
		]},
		{ value: 2, type: 'object', schema: [
			{ id: 'poses', type: 'array', count: '@numPoses', subtype: { type: 'object', schema: [
				{ id: 'numFrames', type: 'uintLE', size: 2 },
				{ type: 'reserved', size: 62 },
				{ id: 'frames', type: 'array', count: '@numFrames', subtype: { type: 'object', schema: [
					{ id: 'width', type: 'uintLE', size: 2 },
					{ id: 'height', type: 'uintLE', size: 2 },
					{ id: 'delay', type: 'uintLE', size: 2 },
					{ type: 'reserved', size: 26 },
					{ id: 'image', type: 'image', width: '@width', height: '@height' },
				]}},
			]}},
		]},
		{ value: 3, type: 'object', schema: [
			{ id: 'images', type: 'array', count: '@numImages', subtype: { type: 'image', width: '@frameWidth', height: '@frameHeight' } },
			{ id: 'poses', type: 'array', count: '@numPoses', subtype: { type: 'object', schema: [
				{ id: 'numFrames', type: 'uintLE', size: 2 },
				{ type: 'reserved', size: 6 },
				{ id: 'name', type: 'pstringLE', size: 2 },
				{ id: 'frames', type: 'array', count: '@numFrames', subtype: { type: 'object', schema: [
					{ id: 'imageIndex', type: 'uintLE', size: 2 },
					{ id: 'delay', type: 'uintLE', size: 2 },
					{ type: 'reserved', size: 4 },
				]}},
			]}},
		]},
	]},
];
