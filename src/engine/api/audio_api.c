#include "minisphere.h"
#include "api/audio_api.h"

#include "api.h"
#include "audio.h"

static duk_ret_t js_new_Mixer                  (duk_context* ctx);
static duk_ret_t js_Mixer_finalize             (duk_context* ctx);
static duk_ret_t js_Mixer_get_volume           (duk_context* ctx);
static duk_ret_t js_Mixer_set_volume           (duk_context* ctx);
static duk_ret_t js_new_Sound                  (duk_context* ctx);
static duk_ret_t js_Sound_finalize             (duk_context* ctx);
static duk_ret_t js_Sound_get_length           (duk_context* ctx);
static duk_ret_t js_Sound_get_mixer            (duk_context* ctx);
static duk_ret_t js_Sound_get_pan              (duk_context* ctx);
static duk_ret_t js_Sound_set_pan              (duk_context* ctx);
static duk_ret_t js_Sound_get_pitch            (duk_context* ctx);
static duk_ret_t js_Sound_set_pitch            (duk_context* ctx);
static duk_ret_t js_Sound_get_playing          (duk_context* ctx);
static duk_ret_t js_Sound_get_position         (duk_context* ctx);
static duk_ret_t js_Sound_set_position         (duk_context* ctx);
static duk_ret_t js_Sound_get_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_set_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_get_seekable         (duk_context* ctx);
static duk_ret_t js_Sound_get_volume           (duk_context* ctx);
static duk_ret_t js_Sound_set_volume           (duk_context* ctx);
static duk_ret_t js_Sound_pause                (duk_context* ctx);
static duk_ret_t js_Sound_play                 (duk_context* ctx);
static duk_ret_t js_Sound_reset                (duk_context* ctx);
static duk_ret_t js_Sound_stop                 (duk_context* ctx);
static duk_ret_t js_new_SoundStream            (duk_context* ctx);
static duk_ret_t js_SoundStream_finalize       (duk_context* ctx);
static duk_ret_t js_SoundStream_get_bufferSize (duk_context* ctx);
static duk_ret_t js_SoundStream_get_mixer      (duk_context* ctx);
static duk_ret_t js_SoundStream_buffer         (duk_context* ctx);
static duk_ret_t js_SoundStream_play           (duk_context* ctx);
static duk_ret_t js_SoundStream_pause          (duk_context* ctx);
static duk_ret_t js_SoundStream_stop           (duk_context* ctx);

duk_ret_t
dukopen_audio(duk_context* ctx)
{
	initialize_audio();

	register_api_type(g_duk, "Mixer", js_Mixer_finalize);
	register_api_prop(g_duk, "Mixer", "volume", js_Mixer_get_volume, js_Mixer_set_volume);
	register_api_type(g_duk, "Sound", js_Sound_finalize);
	register_api_prop(g_duk, "Sound", "length", js_Sound_get_length, NULL);
	register_api_prop(g_duk, "Sound", "mixer", js_Sound_get_mixer, NULL);
	register_api_prop(g_duk, "Sound", "pan", js_Sound_get_pan, js_Sound_set_pan);
	register_api_prop(g_duk, "Sound", "pitch", js_Sound_get_pitch, js_Sound_set_pitch);
	register_api_prop(g_duk, "Sound", "playing", js_Sound_get_playing, NULL);
	register_api_prop(g_duk, "Sound", "position", js_Sound_get_position, js_Sound_set_position);
	register_api_prop(g_duk, "Sound", "repeat", js_Sound_get_repeat, js_Sound_set_repeat);
	register_api_prop(g_duk, "Sound", "seekable", js_Sound_get_seekable, NULL);
	register_api_prop(g_duk, "Sound", "volume", js_Sound_get_volume, js_Sound_set_volume);
	register_api_method(g_duk, "Sound", "pause", js_Sound_pause);
	register_api_method(g_duk, "Sound", "play", js_Sound_play);
	register_api_method(g_duk, "Sound", "reset", js_Sound_reset);
	register_api_method(g_duk, "Sound", "stop", js_Sound_stop);
	register_api_type(g_duk, "SoundStream", js_SoundStream_finalize);
	register_api_prop(g_duk, "SoundStream", "bufferSize", js_SoundStream_get_bufferSize, NULL);
	register_api_prop(g_duk, "SoundStream", "mixer", js_SoundStream_get_mixer, NULL);
	register_api_method(g_duk, "SoundStream", "buffer", js_SoundStream_buffer);
	register_api_method(g_duk, "SoundStream", "pause", js_SoundStream_pause);
	register_api_method(g_duk, "SoundStream", "play", js_SoundStream_play);
	register_api_method(g_duk, "SoundStream", "stop", js_SoundStream_stop);

	duk_function_list_entry functions[] = {
		{ "Mixer",       js_new_Mixer,       DUK_VARARGS },
		{ "Sound",       js_new_Sound,       DUK_VARARGS },
		{ "SoundStream", js_new_SoundStream, DUK_VARARGS },
		{ NULL, NULL, 0 }
	};
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);
	return 1;
}

duk_ret_t
dukclose_audio(duk_context* ctx)
{
	shutdown_audio();
	return 0;
}

static duk_ret_t
js_new_Mixer(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int freq = duk_require_int(ctx, 0);
	int bits = duk_require_int(ctx, 1);
	int channels = n_args >= 3 ? duk_require_int(ctx, 2) : 2;

	mixer_t* mixer;

	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid bit depth for mixer (%i)", bits);
	if (channels < 1 || channels > 7)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid channel count for mixer (%i)", channels);
	if (!(mixer = mixer_new(freq, bits, channels)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create %i-bit %ich voice", bits, channels);
	duk_push_sphere_obj(ctx, "Mixer", mixer);
	return 1;
}

static duk_ret_t
js_Mixer_finalize(duk_context* ctx)
{
	mixer_t* mixer;

	mixer = duk_require_sphere_obj(ctx, 0, "Mixer");
	mixer_free(mixer);
	return 0;
}

static duk_ret_t
js_Mixer_get_volume(duk_context* ctx)
{
	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_sphere_obj(ctx, -1, "Mixer");
	duk_pop(ctx);
	duk_push_number(ctx, mixer_get_gain(mixer));
	return 1;
}

static duk_ret_t
js_Mixer_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);

	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_sphere_obj(ctx, -1, "Mixer");
	duk_pop(ctx);
	mixer_set_gain(mixer, volume);
	return 0;
}

static duk_ret_t
js_new_Sound(duk_context* ctx)
{
	const char* filename;
	sound_t*    sound;

	filename = duk_require_path(ctx, 0, NULL, false);

	if (!(sound = sound_new(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load sound `%s`", filename);
	duk_push_sphere_obj(ctx, "Sound", sound);
	return 1;
}

static duk_ret_t
js_Sound_finalize(duk_context* ctx)
{
	sound_t* sound;

	sound = duk_require_sphere_obj(ctx, 0, "Sound");
	sound_free(sound);
	return 0;
}

static duk_ret_t
js_Sound_get_length(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_number(ctx, sound_len(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_mixer(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_sphere_obj(ctx, "Mixer", mixer_ref(sound_mixer(sound)));
	return 1;
}

static duk_ret_t
js_Sound_get_pan(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_number(ctx, sound_pan(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_pan(duk_context* ctx)
{
	double   new_pan;
	sound_t* sound;

	new_pan = duk_require_number(ctx, 0);

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_set_pan(sound, new_pan);
	return 0;
}

static duk_ret_t
js_Sound_get_pitch(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_number(ctx, sound_pitch(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_pitch(duk_context* ctx)
{
	float new_pitch = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_set_pitch(sound, new_pitch);
	return 0;
}

static duk_ret_t
js_Sound_get_playing(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_boolean(ctx, sound_playing(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_position(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_number(ctx, sound_tell(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_position(duk_context* ctx)
{
	double new_pos = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_seek(sound, new_pos);
	return 0;
}

static duk_ret_t
js_Sound_get_repeat(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_boolean(ctx, sound_repeat(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_repeat(duk_context* ctx)
{
	bool     repeat;
	sound_t* sound;

	repeat = duk_require_boolean(ctx, 0);

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_set_repeat(sound, repeat);
	return 0;
}

static duk_ret_t
js_Sound_get_seekable(duk_context* ctx)
{
	duk_push_true(ctx);
	return 1;
}

static duk_ret_t
js_Sound_get_volume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_push_number(ctx, sound_gain(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_set_gain(sound, volume);
	return 0;
}

static duk_ret_t
js_Sound_pause(duk_context* ctx)
{
	bool     paused;
	sound_t* sound;

	paused = duk_require_boolean(ctx, 0);

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_pause(sound, paused);
	return 0;
}

static duk_ret_t
js_Sound_play(duk_context* ctx)
{
	mixer_t* mixer;
	sound_t* sound;

	mixer = duk_require_sphere_obj(ctx, 0, "Mixer");

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_play(sound, mixer);
	return 0;
}

static duk_ret_t
js_Sound_reset(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_seek(sound, 0);
	return 0;
}

static duk_ret_t
js_Sound_stop(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	sound_stop(sound);
	return 0;
}

static duk_ret_t
js_new_SoundStream(duk_context* ctx)
{
	// new SoundStream(frequency[, bits[, channels]]);
	// Arguments:
	//     frequency: Audio frequency in Hz. (default: 22050)
	//     bits:      Bit depth. (default: 8)
	//     channels:  Number of independent channels. (default: 1)

	stream_t* stream;
	int       argc;
	int       frequency;
	int       bits;
	int       channels;

	argc = duk_get_top(ctx);
	frequency = argc >= 1 ? duk_require_int(ctx, 0) : 22050;
	bits = argc >= 2 ? duk_require_int(ctx, 1) : 8;
	channels = argc >= 3 ? duk_require_int(ctx, 1) : 1;
	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SoundStream(): invalid bit depth (%i)", bits);
	if (!(stream = stream_new(frequency, bits, channels)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SoundStream(): stream creation failed");
	duk_push_sphere_obj(ctx, "SoundStream", stream);
	return 1;
}

static duk_ret_t
js_SoundStream_finalize(duk_context* ctx)
{
	stream_t* stream;

	stream = duk_require_sphere_obj(ctx, 0, "SoundStream");
	stream_free(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_get_bufferSize(duk_context* ctx)
{
	stream_t*    stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_push_number(ctx, stream_bytes_left(stream));
	return 1;
}

static duk_ret_t
js_SoundStream_get_mixer(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	duk_push_sphere_obj(ctx, "Mixer", mixer_ref(stream_mixer(stream)));
	return 1;
}

static duk_ret_t
js_SoundStream_buffer(duk_context* ctx)
{
	// SoundStream:buffer(data);
	// Arguments:
	//     data: An ArrayBuffer or TypedArray containing the audio data
	//           to feed into the stream buffer.

	const void* data;
	duk_size_t  size;
	stream_t*   stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	data = duk_require_buffer_data(ctx, 0, &size);
	stream_feed(stream, data, size);
	return 0;
}

static duk_ret_t
js_SoundStream_pause(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	stream_pause(stream, true);
	return 0;
}

static duk_ret_t
js_SoundStream_play(duk_context* ctx)
{
	mixer_t*  mixer;
	stream_t* stream;

	mixer = duk_require_sphere_obj(ctx, 0, "Mixer");

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	stream_play(stream, mixer);
	return 0;
}

static duk_ret_t
js_SoundStream_stop(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	stream_stop(stream);
	return 0;
}
