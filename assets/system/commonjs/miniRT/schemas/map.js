/**
 *  Sphere 1.x map format schema CommonJS module
 *  a schema for reading Sphere 1.x map files using miniRT/binary
 *  (c) 2015-2016 Fat Cerberus
**/

// some important notes:
//     - as Sphere 1.x was written for x86 systems, all RMP files in the wild
//       can safely be assumed to use little endian byte order for numbers.
//     - the entities section of an RMP map file is a sticking point as it interleaves
//       both person and trigger entities.  while both types have identical headers, the
//       data section varies depending on the type of entity.

module.exports =
[
	{ id: 'signature', type: 'fstring', size: 4, regex: "\\.rmp" },
	{ id: 'version', type: 'uintLE', size: 2, range: [ 1, 1 ] },
	{ id: 'type', type: 'uintLE', size: 1 },
	{ id: 'numLayers', type: 'uintLE', size: 1 },
	{ type: 'reserved', size: 1 },  // reserved
	{ id: 'numEntities', type: 'uintLE', size: 2 },
	{ id: 'startX', type: 'uintLE', size: 2 },
	{ id: 'startY', type: 'uintLE', size: 2 },
	{ id: 'startLayer', type: 'uintLE', size: 1 },
	{ id: 'startDir', type: 'uintLE', size: 1 },
	{ id: 'numStrings', type: 'uintLE', size: 2 },
	{ id: 'numZones', type: 'uintLE', size: 2 },
	{ id: 'repeating', type: 'bool' },
	{ type: 'reserved', size: 234 },  // reserved
	{ id: 'strings', type: 'array', count: '@numStrings', subtype: { type: 'pstringLE', size: 2 } },
	{ id: 'layers', type: 'array', count: '@numLayers', subtype: { type: 'object', schema: {
		{ id: 'width', type: 'uintLE', size: 2 },
		{ id: 'height', type: 'uintLE', size: 2 },
		{ id: 'flags', type: 'uintLE', size: 2 },
		{ id: 'parallaxX', type: 'floatLE' },
		{ id: 'parallaxY', type: 'floatLE' },
		{ id: 'scrollX', type: 'floatLE' },
		{ id: 'scrollY', type: 'floatLE' },
		{ id: 'numSegments', type: 'uintLE', size: 4 },
		{ id: 'reflective', type: 'bool' },
		{ type: 'reserved', size: 3 },
		{ id: 'name', type: 'pstringLE', size: 2 },
		{ id: 'tilemap', type: 'array', count: '@height', subtype: { type: 'array', count: '@width', subtype: { type: 'uintLE', size: 2 } } },
		{ id: 'segments', type: 'array', count: '@numSegments', subtype: { type: 'object', schema: {
			{ id: 'x1', type: 'uintLE', size: 4 },
			{ id: 'y1', type: 'uintLE', size: 4 },
			{ id: 'x2', type: 'uintLE', size: 4 },
			{ id: 'y2', type: 'uintLE', size: 4 },
		}}},
	}}},
	{ id: 'entities', type: 'array', count: '@numEntities', subtype: { type: 'object', schema: {
		{ id: 'x', type: 'uintLE', size: 2 },
		{ id: 'y', type: 'uintLE', size: 2 },
		{ id: 'layer', type: 'uintLE', size: 2 },
		{ id: 'type', type: 'uintLE', size: 2 },
		{ type: 'reserved', size: 8 },
		{ id: 'data', type: 'switch', field: 'type', cases: [
			{ value: 1, type: 'object', schema: {
				{ id: 'name', type: 'pstringLE', size: 2 },
				{ id: 'spriteset', type: 'pstringLE', size: 2 },
				{ id: 'numScripts', type: 'uintLE', size: 2 },
				{ id: 'scripts', type: 'array', count: '@numScripts', subtype: { type: 'pstringLE', size: 2 } },
				{ type: 'reserved', size: 16 },
			}},
			{ value: 2, type: 'object', schema: {
				{ id: 'script', type: 'pstringLE', size: 2 },
			}},
		]},
	}}},
	{ id: 'zones', type: 'array', count: '@numZones', subtype: { type: 'object', schema: {
		{ id: 'x1', type: 'uintLE', size: 2 },
		{ id: 'y1', type: 'uintLE', size: 2 },
		{ id: 'x2', type: 'uintLE', size: 2 },
		{ id: 'y2', type: 'uintLE', size: 2 },
		{ id: 'layer', type: 'uintLE', size: 2 },
		{ id: 'numSteps', type: 'uintLE', size: 2 },
		{ type: 'reserved', size: 4 },
		{ id: 'script', type: 'pstringLE', size: 2 },
	}}},
];
