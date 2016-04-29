/**
 *	Sphere 1.x compatibility shim for Sphere 2.0
 *	bridges the Sphere 1.x (Vanilla) and 2.0 (Pegasus) APIs
 *	(c) 2016 Fat Cerberus
**/

'use strict';
const audio  = require('audio');
const debug  = require('debug');
const engine = require('engine');
const fs     = require('fs');
const net    = require('net');
const gfx    = require('gfx');

global.GetScreenHeight = function() { return gfx.screen.height; }
global.GetScreenWidth = function() { return gfx.screen.width; }
global.CreateColor = gfx.Color;
global.FlipScreen = gfx.flip;
global.ApplyColorMask = function(color) {
	Rectangle(0, 0, gfx.screen.width, gfx.screen.height, color);
};
global.Rectangle = function(x, y, width, height, color) {
	new gfx.Shape([
		{ x: x, y: y, color: color },
		{ x: x + width, y: y, color: color },
		{ x: x, y: y + height, color: color },
		{ x: x + width, y: y + height, color: color },
	]).draw();
}

var mixer = new audio.Mixer(44100, 16, 2);
global.LoadSound = function LoadSound(fileName) {
	if (fileName[0] != '~')
		fileName = 'sounds/' + fileName;
	var sound = new audio.Sound(fileName);
	return {
		toString: function() { return '[object sound]'; },
		isPlaying: function() { return sound.playing; },
		isSeekable: function() { return true; },
		getLength: function() { return sound.length * 1000000; },
		getPan: function() { return sound.pan * 255; },
		getPitch: function() { return sound.pitch; },
		getPosition: function() { return sound.position * 1000000; },
		getRepeat: function() { return sound.repeat; },
		getVolume: function() { return sound.volume * 255; },
		setPan: function(value) { sound.pan = value / 255; },
		setPitch: function(value) { sound.pitch = value; },
		setPosition: function(value) { sound.position = value / 1000000; },
		setRepeat: function(value) { sound.repeat = !!value; },
		setVolume: function(value) { sound.volume = value / 255; },
		play: function(repeat) {
			if (repeat === void null) {
				sound.pause(false);
			}
			else {
				sound.repeat = !!repeat;
				sound.play(mixer);
			}
		},
		pause: function() {
			sound.pause(true);
		},
		reset: function() {
			sound.position = 0.0;
		},
		stop: function() {
			sound.stop();
		},
	};
};
