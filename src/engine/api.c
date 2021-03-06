#include "minisphere.h"
#include "animation.h"
#include "async.h"
#include "audialis.h"
#include "bytearray.h"
#include "color.h"
#include "debugger.h"
#include "file.h"
#include "font.h"
#include "galileo.h"
#include "image.h"
#include "input.h"
#include "logger.h"
#include "map_engine.h"
#include "rng.h"
#include "shader.h"
#include "sockets.h"
#include "spriteset.h"
#include "surface.h"
#include "transpiler.h"
#include "windowstyle.h"

#include "api.h"

#define SPHERE_API_VERSION 2.0
#define SPHERE_API_LEVEL   1

static const char* const SPHERE_EXTENSIONS[] =
{
	"sphere-legacy-api",
	"sphere-obj-constructors",
	"sphere-obj-props",
	"sphere-async-api",
	"sphere-audialis",
	"sphere-coffeescript",
	"sphere-commonjs",
	"sphere-frameskip-api",
	"sphere-galileo",
	"sphere-galileo-shaders",
	"sphere-map-engine",
	"sphere-new-sockets",
	"sphere-rng-object",
	"sphere-s2gm",
	"sphere-spherefs",
	"sphere-typescript",
};

static duk_ret_t js_GetVersion           (duk_context* ctx);
static duk_ret_t js_GetVersionString     (duk_context* ctx);
static duk_ret_t js_GetExtensions        (duk_context* ctx);
static duk_ret_t js_RequireSystemScript  (duk_context* ctx);
static duk_ret_t js_RequireScript        (duk_context* ctx);
static duk_ret_t js_EvaluateSystemScript (duk_context* ctx);
static duk_ret_t js_EvaluateScript       (duk_context* ctx);
static duk_ret_t js_IsSkippedFrame       (duk_context* ctx);
static duk_ret_t js_GetFrameRate         (duk_context* ctx);
static duk_ret_t js_GetGameManifest      (duk_context* ctx);
static duk_ret_t js_GetGameList          (duk_context* ctx);
static duk_ret_t js_GetMaxFrameSkips     (duk_context* ctx);
static duk_ret_t js_GetScreenHeight      (duk_context* ctx);
static duk_ret_t js_GetScreenWidth       (duk_context* ctx);
static duk_ret_t js_GetSeconds           (duk_context* ctx);
static duk_ret_t js_GetTime              (duk_context* ctx);
static duk_ret_t js_SetFrameRate         (duk_context* ctx);
static duk_ret_t js_SetMaxFrameSkips     (duk_context* ctx);
static duk_ret_t js_SetScreenSize        (duk_context* ctx);
static duk_ret_t js_Abort                (duk_context* ctx);
static duk_ret_t js_Alert                (duk_context* ctx);
static duk_ret_t js_Assert               (duk_context* ctx);
static duk_ret_t js_CreateStringFromCode (duk_context* ctx);
static duk_ret_t js_DebugPrint           (duk_context* ctx);
static duk_ret_t js_Delay                (duk_context* ctx);
static duk_ret_t js_DoEvents             (duk_context* ctx);
static duk_ret_t js_ExecuteGame          (duk_context* ctx);
static duk_ret_t js_Exit                 (duk_context* ctx);
static duk_ret_t js_FlipScreen           (duk_context* ctx);
static duk_ret_t js_GarbageCollect       (duk_context* ctx);
static duk_ret_t js_Print                (duk_context* ctx);
static duk_ret_t js_RestartGame          (duk_context* ctx);
static duk_ret_t js_UnskipFrame          (duk_context* ctx);

static duk_ret_t duk_mod_search (duk_context* ctx);

static vector_t*  s_extensions;
static void*      s_print_ptr;
static lstring_t* s_user_agent;

void
initialize_api(duk_context* ctx)
{
	int num_extensions;

	int i;

	console_log(1, "initializing Sphere API");

	s_user_agent = lstr_newf("v%.1f (%s %s)", SPHERE_API_VERSION, PRODUCT_NAME, VERSION_NAME);
	console_log(1, "    API %s", lstr_cstr(s_user_agent));

	// register API extensions
	s_extensions = vector_new(sizeof(char*));
	num_extensions = sizeof SPHERE_EXTENSIONS / sizeof SPHERE_EXTENSIONS[0];
	for (i = 0; i < num_extensions; ++i) {
		console_log(1, "    %s", SPHERE_EXTENSIONS[i]);
		api_register_extension(SPHERE_EXTENSIONS[i]);
	}

	// register the 'global' global object alias (like Node.js!).
	// this provides direct access to the global object from any scope.
	duk_push_global_object(ctx);
	duk_push_string(ctx, "global");
	duk_push_global_object(ctx);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);

	// inject __defineGetter__/__defineSetter__ polyfills
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineGetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { get: func, configurable: true }); } });");
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineSetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { set: func, configurable: true }); } });");

	// save built-in print() in case a script overwrites it
	duk_get_global_string(ctx, "print");
	s_print_ptr = duk_get_heapptr(ctx, -1);
	duk_pop(ctx);

	// set up RequireScript() inclusion tracking table
	duk_push_global_stash(ctx);
	duk_push_object(ctx); duk_put_prop_string(ctx, -2, "RequireScript");
	duk_pop(ctx);

	// stash an object to hold prototypes for built-in objects
	duk_push_global_stash(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "prototypes");
	duk_pop(ctx);

	// register module search callback
	duk_get_global_string(ctx, "Duktape");
	duk_push_c_function(ctx, duk_mod_search, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "modSearch");
	duk_pop(ctx);

	// register core API functions
	api_register_method(ctx, NULL, "GetVersion", js_GetVersion);
	api_register_method(ctx, NULL, "GetVersionString", js_GetVersionString);
	api_register_method(ctx, NULL, "GetExtensions", js_GetExtensions);
	api_register_method(ctx, NULL, "EvaluateScript", js_EvaluateScript);
	api_register_method(ctx, NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	api_register_method(ctx, NULL, "RequireScript", js_RequireScript);
	api_register_method(ctx, NULL, "RequireSystemScript", js_RequireSystemScript);
	api_register_method(ctx, NULL, "IsSkippedFrame", js_IsSkippedFrame);
	api_register_method(ctx, NULL, "GetFrameRate", js_GetFrameRate);
	api_register_method(ctx, NULL, "GetGameManifest", js_GetGameManifest);
	api_register_method(ctx, NULL, "GetGameList", js_GetGameList);
	api_register_method(ctx, NULL, "GetMaxFrameSkips", js_GetMaxFrameSkips);
	api_register_method(ctx, NULL, "GetScreenHeight", js_GetScreenHeight);
	api_register_method(ctx, NULL, "GetScreenWidth", js_GetScreenWidth);
	api_register_method(ctx, NULL, "GetSeconds", js_GetSeconds);
	api_register_method(ctx, NULL, "GetTime", js_GetTime);
	api_register_method(ctx, NULL, "SetFrameRate", js_SetFrameRate);
	api_register_method(ctx, NULL, "SetMaxFrameSkips", js_SetMaxFrameSkips);
	api_register_method(ctx, NULL, "SetScreenSize", js_SetScreenSize);
	api_register_method(ctx, NULL, "Abort", js_Abort);
	api_register_method(ctx, NULL, "Alert", js_Alert);
	api_register_method(ctx, NULL, "Assert", js_Assert);
	api_register_method(ctx, NULL, "CreateStringFromCode", js_CreateStringFromCode);
	api_register_method(ctx, NULL, "DebugPrint", js_DebugPrint);
	api_register_method(ctx, NULL, "Delay", js_Delay);
	api_register_method(ctx, NULL, "DoEvents", js_DoEvents);
	api_register_method(ctx, NULL, "Exit", js_Exit);
	api_register_method(ctx, NULL, "ExecuteGame", js_ExecuteGame);
	api_register_method(ctx, NULL, "FlipScreen", js_FlipScreen);
	api_register_method(ctx, NULL, "GarbageCollect", js_GarbageCollect);
	api_register_method(ctx, NULL, "Print", js_Print);
	api_register_method(ctx, NULL, "RestartGame", js_RestartGame);
	api_register_method(ctx, NULL, "UnskipFrame", js_UnskipFrame);

	// initialize subsystem APIs
	init_animation_api();
	init_async_api();
	init_audialis_api();
	init_bytearray_api();
	init_color_api();
	init_file_api();
	init_font_api(g_duk);
	init_galileo_api();
	init_image_api(g_duk);
	init_input_api();
	init_logging_api();
	init_map_engine_api(g_duk);
	init_rng_api();
	init_screen_api();
	init_shader_api();
	init_sockets_api();
	init_spriteset_api(g_duk);
	init_surface_api();
	init_windowstyle_api();
}

void
shutdown_api(void)
{
	console_log(1, "shutting down Sphere API");

	lstr_free(s_user_agent);
}

bool
api_have_extension(const char* name)
{
	iter_t iter;
	char* *i_name;

	iter = vector_enum(s_extensions);
	while (i_name = vector_next(&iter))
		if (strcmp(*i_name, name) == 0) return true;
	return false;
}

int
api_level(void)
{
	return SPHERE_API_LEVEL;
}

void
api_register_const(duk_context* ctx, const char* name, double value)
{
	duk_push_global_object(ctx);
	duk_push_string(ctx, name); duk_push_number(ctx, value);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);
}

void
api_register_ctor(duk_context* ctx, const char* name, duk_c_function fn, duk_c_function finalizer)
{
	duk_push_global_object(ctx);
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, name);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);

	// create a prototype. Duktape won't assign one for us.
	duk_push_object(ctx);
	duk_push_string(ctx, name);
	duk_put_prop_string(ctx, -2, "\xFF" "ctor");
	if (finalizer != NULL) {
		duk_push_c_function(ctx, finalizer, DUK_VARARGS);
		duk_put_prop_string(ctx, -2, "\xFF" "dtor");
	}

	// save the prototype in the prototype stash. for full compatibility with
	// Sphere 1.5, we have to allow native objects to be created through the
	// legacy APIs even if the corresponding constructor is overwritten or shadowed.
	// for that to work, the prototype must remain accessible.
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "prototypes");
	duk_dup(ctx, -3);
	duk_put_prop_string(ctx, -2, name);
	duk_pop_2(ctx);

	// attach prototype to constructor
	duk_push_string(ctx, "prototype");
	duk_insert(ctx, -2);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_WRITABLE);
	duk_push_string(ctx, name);
	duk_insert(ctx, -2);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);
}

bool
api_register_extension(const char* designation)
{
	char* string;

	if (!(string = strdup(designation))) return false;
	if (!vector_push(s_extensions, &string))
		return false;
	return true;
}

void
api_register_function(duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!duk_get_prop_string(ctx, -1, namespace_name)) {
			duk_pop(ctx);
			duk_push_string(ctx, namespace_name);
			duk_push_object(ctx);
			duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
				| DUK_DEFPROP_SET_WRITABLE
				| DUK_DEFPROP_SET_CONFIGURABLE);
			duk_get_prop_string(ctx, -1, namespace_name);
		}
	}

	duk_push_string(ctx, name);
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, name);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	if (namespace_name != NULL)
		duk_pop(ctx);
	duk_pop(ctx);
}

void
api_register_method(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);
	if (ctor_name != NULL) {
		// load the prototype from the prototype stash
		duk_push_global_stash(ctx);
		duk_get_prop_string(ctx, -1, "prototypes");
		duk_get_prop_string(ctx, -1, ctor_name);
	}

	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, name);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);

	// for a defprop, Duktape expects the key to be pushed first, then the value; however, since
	// we have the value (the function being registered) on the stack already by this point, we
	// need to shuffle things around to make everything work.
	duk_push_string(ctx, name);
	duk_insert(ctx, -2);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);

	if (ctor_name != NULL)
		duk_pop_3(ctx);
	duk_pop(ctx);
}

void
api_register_prop(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter)
{
	duk_uint_t flags;
	int        obj_index;

	duk_push_global_object(ctx);
	if (ctor_name != NULL) {
		duk_push_global_stash(ctx);
		duk_get_prop_string(ctx, -1, "prototypes");
		duk_get_prop_string(ctx, -1, ctor_name);
	}
	obj_index = duk_normalize_index(ctx, -1);
	duk_push_string(ctx, name);
	flags = DUK_DEFPROP_SET_CONFIGURABLE;
	if (getter != NULL) {
		duk_push_c_function(ctx, getter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_GETTER;
	}
	if (setter != NULL) {
		duk_push_c_function(ctx, setter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_SETTER;
	}
	duk_def_prop(g_duk, obj_index, flags);
	if (ctor_name != NULL)
		duk_pop_3(ctx);
	duk_pop(ctx);
}

noreturn
duk_error_ni(duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...)
{
	va_list ap;
	char*   filename;
	int     line_number;

	// get filename and line number from Duktape call stack
	duk_get_global_string(g_duk, "Duktape");
	duk_get_prop_string(g_duk, -1, "act");
	duk_push_int(g_duk, -2 + blame_offset);
	duk_call(g_duk, 1);
	if (!duk_is_object(g_duk, -1)) {
		duk_pop(g_duk);
		duk_get_prop_string(g_duk, -1, "act");
		duk_push_int(g_duk, -2);
		duk_call(g_duk, 1);
	}
	duk_get_prop_string(g_duk, -1, "lineNumber");
	duk_get_prop_string(g_duk, -2, "function");
	duk_get_prop_string(g_duk, -1, "fileName");
	filename = strdup(duk_safe_to_string(g_duk, -1));
	line_number = duk_to_int(g_duk, -3);
	duk_pop_n(g_duk, 5);

	// construct an Error object
	va_start(ap, fmt);
	duk_push_error_object_va(ctx, err_code, fmt, ap);
	va_end(ap);
	duk_push_string(ctx, filename);
	duk_put_prop_string(ctx, -2, "fileName");
	duk_push_int(ctx, line_number);
	duk_put_prop_string(ctx, -2, "lineNumber");
	free(filename);
	
	duk_throw(ctx);
}

duk_bool_t
duk_is_sphere_obj(duk_context* ctx, duk_idx_t index, const char* ctor_name)
{
	const char* obj_ctor_name;
	duk_bool_t  result;

	index = duk_require_normalize_index(ctx, index);
	if (!duk_is_object_coercible(ctx, index))
		return 0;

	duk_get_prop_string(ctx, index, "\xFF" "ctor");
	obj_ctor_name = duk_safe_to_string(ctx, -1);
	result = strcmp(obj_ctor_name, ctor_name) == 0;
	duk_pop(ctx);
	return result;
}

void
duk_push_sphere_obj(duk_context* ctx, const char* ctor_name, void* udata)
{
	duk_idx_t obj_index;

	duk_push_object(ctx);
	obj_index = duk_normalize_index(ctx, -1);
	duk_push_pointer(ctx, udata);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "prototypes");
	duk_get_prop_string(ctx, -1, ctor_name);
	if (duk_get_prop_string(ctx, -1, "\xFF" "dtor")) {
		duk_set_finalizer(ctx, obj_index);
	}
	else
		duk_pop(ctx);
	duk_set_prototype(ctx, obj_index);
	duk_pop_2(ctx);
}

void*
duk_require_sphere_obj(duk_context* ctx, duk_idx_t index, const char* ctor_name)
{
	void* udata;

	index = duk_require_normalize_index(ctx, index);
	if (!duk_is_sphere_obj(ctx, index, ctor_name))
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "expected a %s object", ctor_name);
	duk_get_prop_string(ctx, index, "\xFF" "udata");
	udata = duk_get_pointer(ctx, -1);
	duk_pop(ctx);
	return udata;
}

static duk_ret_t
duk_mod_search(duk_context* ctx)
{
	vector_t*   filenames;
	lstring_t*  filename;
	size_t      len;
	const char* name;
	char*       slurp;
	lstring_t*  source_text;

	iter_t     iter;
	lstring_t* *p;

	name = duk_get_string(ctx, 0);
	if (name[0] == '~')
		duk_error_ni(ctx, -2, DUK_ERR_TYPE_ERROR, "SphereFS prefix not allowed in module ID");

	filenames = vector_new(sizeof(lstring_t*));
	filename = lstr_newf("lib/%s.js", name); vector_push(filenames, &filename);
	filename = lstr_newf("lib/%s.ts", name); vector_push(filenames, &filename);
	filename = lstr_newf("lib/%s.coffee", name); vector_push(filenames, &filename);
	filename = lstr_newf("#/modules/%s.js", name); vector_push(filenames, &filename);
	filename = lstr_newf("#/modules/%s.ts", name); vector_push(filenames, &filename);
	filename = lstr_newf("#/modules/%s.coffee", name); vector_push(filenames, &filename);
	filename = NULL;
	iter = vector_enum(filenames);
	while (p = vector_next(&iter)) {
		if (filename == NULL && sfs_fexist(g_fs, lstr_cstr(*p), NULL))
			filename = lstr_dup(*p);
		lstr_free(*p);
	}
	vector_free(filenames);
	if (filename == NULL)
		duk_error_ni(ctx, -2, DUK_ERR_REFERENCE_ERROR, "CommonJS module `%s` not found", name);
	console_log(2, "initializing JS module `%s` as `%s`", name, lstr_cstr(filename));
	if (!(slurp = sfs_fslurp(g_fs, lstr_cstr(filename), NULL, &len)))
		duk_error_ni(ctx, -2, DUK_ERR_ERROR, "unable to read script `%s`", lstr_cstr(filename));
	source_text = lstr_from_buf(slurp, len);
	free(slurp);
	if (!transpile_to_js(&source_text, lstr_cstr(filename))) {
		lstr_free(source_text);
		duk_throw(ctx);
	}
	duk_push_lstring_t(ctx, filename);
	duk_put_prop_string(ctx, 3, "filename");
	duk_push_lstring_t(ctx, source_text);
	lstr_free(source_text);
	lstr_free(filename);
	return 1;
}

static duk_ret_t
js_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, SPHERE_API_VERSION);
	return 1;
}

static duk_ret_t
js_GetVersionString(duk_context* ctx)
{
	duk_push_string(ctx, lstr_cstr(s_user_agent));
	return 1;
}

static duk_ret_t
js_GetExtensions(duk_context* ctx)
{
	char**  i_string;

	iter_t iter;
	int    i;

	duk_push_array(ctx);
	iter = vector_enum(s_extensions); i = 0;
	while (i_string = vector_next(&iter)) {
		duk_push_string(ctx, *i_string);
		duk_put_prop_index(ctx, -2, i++);
	}
	return 1;
}

static duk_ret_t
js_EvaluateScript(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, "scripts", true);
	if (!sfs_fexist(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateScript(): file `%s` not found", filename);
	if (!evaluate_script(filename))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_EvaluateSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	char path[SPHERE_PATH_MAX];

	sprintf(path, "scripts/lib/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		sprintf(path, "#/scripts/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateSystemScript(): system script `%s` not found", filename);
	if (!evaluate_script(path))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_RequireScript(duk_context* ctx)
{
	const char* filename;
	bool        is_required;

	filename = duk_require_path(ctx, 0, "scripts", true);
	if (!sfs_fexist(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireScript(): file `%s` not found", filename);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, filename);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, filename);
		if (!evaluate_script(filename))
			duk_throw(ctx);
	}
	duk_pop_3(ctx);
	return 0;
}

static duk_ret_t
js_RequireSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	bool is_required;
	char path[SPHERE_PATH_MAX];

	sprintf(path, "scripts/lib/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		sprintf(path, "#/scripts/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireSystemScript(): system script `%s` not found", filename);

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, path);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, path);
		if (!evaluate_script(path))
			duk_throw(ctx);
	}
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_IsSkippedFrame(duk_context* ctx)
{
	duk_push_boolean(ctx, screen_is_skipframe(g_screen));
	return 1;
}

static duk_ret_t
js_GetFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, g_framerate);
	return 1;
}

static duk_ret_t
js_GetGameManifest(duk_context* ctx)
{
	duk_push_lstring_t(ctx, get_game_manifest(g_fs));
	duk_json_decode(ctx, -1);
	duk_push_string(ctx, path_cstr(get_game_path(g_fs)));
	duk_put_prop_string(ctx, -2, "directory");
	return 1;
}

static duk_ret_t
js_GetGameList(duk_context* ctx)
{
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_FS_ENTRY* fse;
	path_t*           path = NULL;
	path_t*           paths[2];
	sandbox_t*        sandbox;

	int i, j = 0;

	// build search paths
	paths[0] = path_rebase(path_new("games/"), enginepath());
	paths[1] = path_rebase(path_new("minisphere/games/"), homepath());

	// search for supported games
	duk_push_array(ctx);
	for (i = sizeof paths / sizeof(path_t*) - 1; i >= 0; --i) {
		fse = al_create_fs_entry(path_cstr(paths[i]));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
				path = path_new(al_get_fs_entry_name(file_info));
				if (sandbox = new_sandbox(path_cstr(path))) {
					duk_push_lstring_t(ctx, get_game_manifest(sandbox));
					duk_json_decode(ctx, -1);
					duk_push_string(ctx, path_cstr(path));
					duk_put_prop_string(ctx, -2, "directory");
					duk_put_prop_index(ctx, -2, j++);
					free_sandbox(sandbox);
				}
				path_free(path);
			}
		}
		al_destroy_fs_entry(fse);
		path_free(paths[i]);
	}
	return 1;
}

static duk_ret_t
js_GetMaxFrameSkips(duk_context* ctx)
{
	duk_push_int(ctx, screen_get_frameskip(g_screen));
	return 1;
}

static duk_ret_t
js_GetScreenHeight(duk_context* ctx)
{
	duk_push_int(ctx, g_res_y);
	return 1;
}

static duk_ret_t
js_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, g_res_x);
	return 1;
}

static duk_ret_t
js_GetSeconds(duk_context* ctx)
{
	duk_push_number(ctx, al_get_time());
	return 1;
}

static duk_ret_t
js_GetTime(duk_context* ctx)
{
	duk_push_number(ctx, floor(al_get_time() * 1000));
	return 1;
}

static duk_ret_t
js_SetFrameRate(duk_context* ctx)
{
	int framerate = duk_require_int(ctx, 0);

	if (framerate < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetFrameRate(): framerate must be positive (got: %d)", framerate);
	g_framerate = framerate;
	return 0;
}

static duk_ret_t
js_SetMaxFrameSkips(duk_context* ctx)
{
	int max_skips = duk_require_int(ctx, 0);

	if (max_skips < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetMaxFrameSkips(): value cannot be negative (%d)", max_skips);
	screen_set_frameskip(g_screen, max_skips);
	return 0;
}

static duk_ret_t
js_SetScreenSize(duk_context* ctx)
{
	int  res_width;
	int  res_height;

	res_width = duk_require_int(ctx, 0);
	res_height = duk_require_int(ctx, 1);

	if (res_width < 0 || res_height < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetScreenSize(): dimensions cannot be negative (got X: %d, Y: %d)",
			res_width, res_height);
	screen_resize(g_screen, res_width, res_height);
	return 0;
}

static duk_ret_t
js_Alert(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* text = n_args >= 1 && !duk_is_null_or_undefined(ctx, 0)
		? duk_to_string(ctx, 0) : "It's 8:12... do you know where the pig is?\n\nIt's...\n\n\n\n\n\n\nBEHIND YOU! *MUNCH*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	const char* caller_info;
	const char* filename;
	int         line_number;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Alert(): stack offset must be negative");

	// get filename and line number of Alert() call
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3 + stack_offset); duk_call(ctx, 1);
	if (!duk_is_object(ctx, -1)) {
		duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3); duk_call(ctx, 1);
	}
	duk_remove(ctx, -2);
	duk_get_prop_string(ctx, -1, "lineNumber"); line_number = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "function");
	duk_get_prop_string(ctx, -1, "fileName"); filename = duk_get_string(ctx, -1); duk_pop(ctx);
	duk_pop_2(ctx);

	// show the message
	screen_show_mouse(g_screen, true);
	caller_info =
		duk_push_sprintf(ctx, "%s (line %i)", filename, line_number),
		duk_get_string(ctx, -1);
	al_show_native_message_box(screen_display(g_screen), "Alert from Sphere game", caller_info, text, NULL, 0x0);
	screen_show_mouse(g_screen, false);

	return 0;
}

static duk_ret_t
js_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* message = n_args >= 1 ? duk_to_string(ctx, 0) : "Some type of weird pig just ate your game!\n\n\n\n\n\n\n\n...and you*munch*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Abort(): stack offset must be negative");
	duk_error_ni(ctx, -1 + stack_offset, DUK_ERR_ERROR, "%s", message);
}

static duk_ret_t
js_Assert(duk_context* ctx)
{
	const char* filename;
	int         line_number;
	const char* message;
	int         num_args;
	bool        result;
	int         stack_offset;
	lstring_t*  text;

	num_args = duk_get_top(ctx);
	result = duk_to_boolean(ctx, 0);
	message = duk_require_string(ctx, 1);
	stack_offset = num_args >= 3 ? duk_require_int(ctx, 2)
		: 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Assert(): stack offset must be negative");

	if (!result) {
		// get the offending script and line number from the call stack
		duk_push_global_object(ctx);
		duk_get_prop_string(ctx, -1, "Duktape");
		duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3 + stack_offset); duk_call(ctx, 1);
		if (!duk_is_object(ctx, -1)) {
			duk_pop(ctx);
			duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3); duk_call(ctx, 1);
		}
		duk_remove(ctx, -2);
		duk_get_prop_string(ctx, -1, "lineNumber"); line_number = duk_get_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "function");
		duk_get_prop_string(ctx, -1, "fileName"); filename = duk_get_string(ctx, -1); duk_pop(ctx);
		duk_pop_2(ctx);
		fprintf(stderr, "ASSERT: `%s:%i` : %s\n", filename, line_number, message);

		// if an assertion fails in a game being debugged:
		//   - the user may choose to ignore it, in which case execution continues.  this is useful
		//     in some debugging scenarios.
		//   - if the user chooses not to continue, a prompt breakpoint will be triggered, turning
		//     over control to the attached debugger.
		if (is_debugger_attached()) {
			text = lstr_newf("%s (line: %i)\n%s\n\nYou can ignore the error, or pause execution, turning over control to the attached debugger.  If you choose to debug, execution will pause at the statement following the failed Assert().\n\nIgnore the error and continue?", filename, line_number, message);
			if (!al_show_native_message_box(screen_display(g_screen), "Script Error", "Assertion failed!",
				lstr_cstr(text), NULL, ALLEGRO_MESSAGEBOX_WARN | ALLEGRO_MESSAGEBOX_YES_NO))
			{
				duk_debugger_pause(ctx);
			}
			lstr_free(text);
		}
	}
	duk_dup(ctx, 0);
	return 1;
}

static duk_ret_t
js_CreateStringFromCode(duk_context* ctx)
{
	int code = duk_require_int(ctx, 0);

	char cstr[2];

	if (code < 0 || code > 255)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CreateStringFromCode(): character code is out of ASCII range (%i)", code);
	cstr[0] = (char)code; cstr[1] = '\0';
	duk_push_string(ctx, cstr);
	return 1;
}

static duk_ret_t
js_DebugPrint(duk_context* ctx)
{
	int num_items;

	num_items = duk_get_top(ctx);

	// separate printed values with a space
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);

	// tack on a newline and concatenate the values
	duk_push_string(ctx, "\n");
	duk_join(ctx, num_items + 1);

	debug_print(duk_get_string(ctx, -1));
	return 0;
}

static duk_ret_t
js_Delay(duk_context* ctx)
{
	double millisecs = floor(duk_require_number(ctx, 0));

	if (millisecs < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Delay(): delay must be positive (got: %.0f)", millisecs);
	delay(millisecs / 1000);
	return 0;
}

static duk_ret_t
js_DoEvents(duk_context* ctx)
{
	do_events();
	duk_push_boolean(ctx, true);
	return 1;
}

static duk_ret_t
js_ExecuteGame(duk_context* ctx)
{
	path_t*     games_path;
	const char* filename;

	filename = duk_require_string(ctx, 0);

	// store the old game path so we can relaunch when the chained game exits
	g_last_game_path = path_dup(get_game_path(g_fs));

	// if the passed-in path is relative, resolve it relative to <engine>/games.
	// this is done for compatibility with Sphere 1.x.
	g_game_path = path_new(filename);
	games_path = path_rebase(path_new("games/"), enginepath());
	path_rebase(g_game_path, games_path);
	path_free(games_path);

	restart_engine();
}

static duk_ret_t
js_Exit(duk_context* ctx)
{
	exit_game(false);
}

static duk_ret_t
js_FlipScreen(duk_context* ctx)
{
	screen_flip(g_screen, g_framerate);
	return 0;
}

static duk_ret_t
js_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

static duk_ret_t
js_Print(duk_context* ctx)
{
	int num_items;

	num_items = duk_get_top(ctx);
	duk_push_heapptr(ctx, s_print_ptr);
	duk_insert(ctx, 0);
	duk_call(ctx, num_items);
	return 0;
}

static duk_ret_t
js_RestartGame(duk_context* ctx)
{
	restart_engine();
}

static duk_ret_t
js_UnskipFrame(duk_context* ctx)
{
	screen_unskip_frame(g_screen);
	return 0;
}
