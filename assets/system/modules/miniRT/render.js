/**
 *	miniRT/render CommonJS module
 *	tools for pre-rendering common RPG UI elements
 *	(c) 2015-2016 Fat Cerberus
**/

if (typeof exports === 'undefined')
{
	throw new TypeError("script must be loaded with require()");
}

const gfx = require('gfx');

var render =
module.exports = (function()
{
	return {
		TextRender: TextRender,
	};

	function TextRender(text, options)
	{
		options = options != null ? options : {};

		var font = 'font' in options ? options.font : GetSystemFont();
		var color = 'color' in options ? options.color : new Color(255, 255, 255, 255);
		var shadow = 'shadow' in options ? options.shadow : 0;

		var shadowColor = new Color(0, 0, 0, color.alpha);
		var width = font.getStringWidth(text) + Math.abs(shadow);
		var height = font.height + Math.abs(shadow);
		var surface = new Surface(width, height);
		var lastColorMask = font.getColorMask();
		var offsetX = 0;
		var offsetY = 0;
		if (shadow > 0) {
			font.setColorMask(shadowColor);
			surface.drawText(font, shadow, shadow, text);
			font.setColorMask(color);
			surface.drawText(font, 0, 0, text);
		}
		else if (shadow < 0) {
			font.setColorMask(shadowColor);
			surface.drawText(font, 0, 0, text);
			font.setColorMask(color);
			surface.drawText(font, -shadow, -shadow, text);
			offsetX = shadow;
			offsetY = shadow;
		}
		else {
			font.setColorMask(color);
			surface.drawText(font, 0, 0, text);
		}
		font.setColorMask(lastColorMask);
		var image = surface.createImage();

		this.draw = function draw(x, y)
		{
			image.blit(x + offsetX, y + offsetY);
		}
	}
})();
