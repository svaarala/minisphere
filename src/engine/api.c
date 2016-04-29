#include "minisphere.h"
#include "api.h"

#include "async.h"
#include "audio.h"
#include "color.h"
#include "debugger.h"
#include "file.h"
#include "font.h"
#include "primitives.h"
#include "image.h"
#include "input.h"
#include "logger.h"
#include "rng.h"
#include "shader.h"
#include "sockets.h"
#include "transpiler.h"

#include "api/engine_api.h"
#include "api/audio_api.h"
#include "api/debug_api.h"
#include "api/fs_api.h"
#include "api/gfx_api.h"
#include "api/net_api.h"
#include "api/random_api.h"

struct module
{
	char*          name;
	duk_c_function initializer;
	duk_c_function finalizer;
};

static duk_ret_t duk_mod_search (duk_context* ctx);

struct module*    s_modules = NULL;
int               s_num_modules;

void
initialize_api(duk_context* ctx)
{
	console_log(1, "initializing Sphere API");

	// register the 'global' global object alias (like Node.js!).
	// this provides direct access to the global object from any scope.
	duk_push_global_object(ctx);
	duk_push_string(ctx, "global");
	duk_push_global_object(ctx);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);

	// register CommonJS module loader
	duk_get_global_string(g_duk, "Duktape");
	duk_push_c_function(g_duk, duk_mod_search, DUK_VARARGS);
	duk_put_prop_string(g_duk, -2, "modSearch");
	duk_pop(g_duk);

	// stash an object to hold prototypes for built-in objects
	duk_push_global_stash(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "prototypes");
	duk_pop(ctx);

	// register built-in CommonJS modules
	register_api_module("engine", dukopen_engine, dukclose_engine);
	register_api_module("audio", dukopen_audio, dukclose_audio);
	register_api_module("debug", dukopen_debug, dukclose_debug);
	register_api_module("fs", dukopen_fs, dukclose_fs);
	register_api_module("gfx", dukopen_gfx, dukclose_gfx);
	register_api_module("net", dukopen_net, dukclose_net);
	register_api_module("random", dukopen_random, dukclose_random);
}

void
shutdown_api(void)
{
	int i;

	console_log(1, "shutting down Sphere API");

	for (i = 0; i < s_num_modules; ++i)
		free(s_modules[i].name);
	free(s_modules);
}

void
register_api_const(duk_context* ctx, const char* name, double value)
{
	duk_push_global_object(ctx);
	duk_push_string(ctx, name); duk_push_number(ctx, value);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);
}

void
register_api_ctor(duk_context* ctx, const char* name, duk_c_function fn, duk_c_function finalizer)
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

void
register_api_function(duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn)
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
register_api_method(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn)
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
register_api_module(const char* name, duk_c_function initializer, duk_c_function finalizer)
{
	// `initializer` should set up any components required to use the module
	// and leave an object on the top of the stack which will serve as the module's
	// export table.

	struct module* module;

	++s_num_modules;
	s_modules = realloc(s_modules, s_num_modules * sizeof(struct module));
	module = &s_modules[s_num_modules - 1];
	module->name = strdup(name);
	module->initializer = initializer;
	module->finalizer = finalizer;
}

void
register_api_prop(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter)
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

void
register_api_type(duk_context* ctx, const char* name, duk_c_function finalizer)
{
	// construct a prototype for our new type
	duk_push_object(ctx);
	duk_push_string(ctx, name);
	duk_put_prop_string(ctx, -2, "\xFF" "ctor");
	if (finalizer != NULL) {
		duk_push_c_function(ctx, finalizer, DUK_VARARGS);
		duk_put_prop_string(ctx, -2, "\xFF" "dtor");
	}

	// stash the new prototype
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "prototypes");
	duk_dup(ctx, -3);
	duk_put_prop_string(ctx, -2, name);
	duk_pop_3(ctx);
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
	duk_idx_t index;

	duk_push_object(ctx); index = duk_normalize_index(ctx, -1);
	duk_push_pointer(ctx, udata); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "prototypes");
	duk_get_prop_string(ctx, -1, ctor_name);
	if (duk_get_prop_string(ctx, -1, "\xFF" "dtor")) {
		duk_set_finalizer(ctx, index);
	}
	else
		duk_pop(ctx);
	duk_set_prototype(ctx, index);
	duk_pop_2(ctx);
}

void*
duk_require_sphere_obj(duk_context* ctx, duk_idx_t index, const char* ctor_name)
{
	void* udata;

	index = duk_require_normalize_index(ctx, index);
	if (!duk_is_sphere_obj(ctx, index, ctor_name))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "not a Sphere %s", ctor_name);
	duk_get_prop_string(ctx, index, "\xFF" "udata"); udata = duk_get_pointer(ctx, -1); duk_pop(ctx);
	return udata;
}

static duk_ret_t
duk_mod_search(duk_context* ctx)
{
	vector_t*   filenames;
	lstring_t*  filename;
	iter_t      iter;
	size_t      len;
	const char* name;
	lstring_t** p_string;
	char*       slurp;
	lstring_t*  source_text;

	int i;

	name = duk_get_string(ctx, 0);
	if (name[0] == '~')
		duk_error_ni(ctx, -2, DUK_ERR_TYPE_ERROR, "SphereFS prefix not allowed in module ID");

	// check whether we're requiring a native Pegasus module
	for (i = 0; i < s_num_modules; ++i) {
		if (strcmp(name, s_modules[i].name) == 0) {
			console_log(1, "initializing native module `%s`", name);
			duk_push_c_function(ctx, s_modules[i].initializer, DUK_VARARGS);
			duk_call(ctx, 0);
			if (s_modules[i].finalizer != NULL) {
				duk_push_c_function(ctx, s_modules[i].finalizer, DUK_VARARGS);
				duk_set_finalizer(ctx, -2);
			}
			duk_put_prop_string(ctx, 3, "exports");
			return 0;
		}
	}

	filenames = vector_new(sizeof(lstring_t*));
	filename = lstr_newf("lib/%s.js", name); vector_push(filenames, &filename);
	filename = lstr_newf("lib/%s.ts", name); vector_push(filenames, &filename);
	filename = lstr_newf("lib/%s.coffee", name); vector_push(filenames, &filename);
	filename = lstr_newf("~sys/modules/%s.js", name); vector_push(filenames, &filename);
	filename = lstr_newf("~sys/modules/%s.ts", name); vector_push(filenames, &filename);
	filename = lstr_newf("~sys/modules/%s.coffee", name); vector_push(filenames, &filename);
	filename = NULL;
	iter = vector_enum(filenames);
	while (p_string = vector_next(&iter)) {
		if (filename == NULL && sfs_fexist(g_fs, lstr_cstr(*p_string), NULL))
			filename = lstr_dup(*p_string);
		lstr_free(*p_string);
	}
	vector_free(filenames);
	if (filename == NULL)
		duk_error_ni(ctx, -2, DUK_ERR_REFERENCE_ERROR, "module `%s` not found", name);
	console_log(1, "initializing JS module `%s` as `%s`", name, lstr_cstr(filename));
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
