#include "minisphere.h"
#include "api/engine_api.h"

#include "api.h"
#include "async.h"

static const char* const SPHERE_EXTENSIONS[] =
{
	"sphere_coffeescript",
	"sphere_typescript",
};

static duk_ret_t js_get_apiLevel (duk_context* ctx);
static duk_ret_t js_get_name     (duk_context* ctx);
static duk_ret_t js_get_time     (duk_context* ctx);
static duk_ret_t js_get_version  (duk_context* ctx);
static duk_ret_t js_dispatch     (duk_context* ctx);
static duk_ret_t js_doEvents     (duk_context* ctx);
static duk_ret_t js_exit         (duk_context* ctx);
static duk_ret_t js_restart      (duk_context* ctx);
static duk_ret_t js_sleep        (duk_context* ctx);

static unsigned int s_dispatch_id = 0;

duk_ret_t
dukopen_engine(duk_context* ctx)
{
	initialize_async();

	duk_function_list_entry functions[] = {
		{ "dispatch", js_dispatch, DUK_VARARGS },
		{ "doEvents", js_doEvents, DUK_VARARGS },
		{ "exit",     js_exit,     DUK_VARARGS },
		{ "restart",  js_restart,  DUK_VARARGS },
		{ "sleep",    js_sleep,    DUK_VARARGS },
		{ NULL, NULL, 0 }
	};
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);

	duk_push_string(ctx, "apiLevel");
	duk_push_c_function(ctx, js_get_apiLevel, DUK_VARARGS);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER
		| DUK_DEFPROP_SET_CONFIGURABLE | DUK_DEFPROP_SET_ENUMERABLE);

	duk_push_string(ctx, "name");
	duk_push_c_function(ctx, js_get_name, DUK_VARARGS);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER
		| DUK_DEFPROP_SET_CONFIGURABLE | DUK_DEFPROP_SET_ENUMERABLE);
	
	duk_push_string(ctx, "time");
	duk_push_c_function(ctx, js_get_time, DUK_VARARGS);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER
		| DUK_DEFPROP_SET_CONFIGURABLE | DUK_DEFPROP_SET_ENUMERABLE);

	duk_push_string(ctx, "version");
	duk_push_c_function(ctx, js_get_version, DUK_VARARGS);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER
		| DUK_DEFPROP_SET_CONFIGURABLE | DUK_DEFPROP_SET_ENUMERABLE);
	
	return 1;
}

duk_ret_t
dukclose_engine(duk_context* ctx)
{
	shutdown_async();
	return 0;
}

static duk_ret_t
js_get_apiLevel(duk_context* ctx)
{
	duk_push_int(ctx, 2);
	return 1;
}

static duk_ret_t
js_get_name(duk_context* ctx)
{
	duk_push_string(ctx, PRODUCT_NAME);
	return 1;
}

static duk_ret_t
js_get_time(duk_context* ctx)
{
	duk_push_number(ctx, al_get_time());
	return 1;
}

static duk_ret_t
js_get_version(duk_context* ctx)
{
	duk_push_string(ctx, VERSION_NAME);
	return 1;
}

static duk_ret_t
js_dispatch(duk_context* ctx)
{
	script_t* script;
	char*     script_name;

	script_name = strnewf("async~%u", s_dispatch_id++);
	script = duk_require_sphere_script(ctx, 0, script_name);
	free(script_name);

	if (!queue_async_script(script))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to queue async script");
	return 0;
}

static duk_ret_t
js_doEvents(duk_context* ctx)
{
	do_events();
	duk_push_boolean(ctx, true);
	return 1;
}

static duk_ret_t
js_exit(duk_context* ctx)
{
	exit_game(false);
}

static duk_ret_t
js_restart(duk_context* ctx)
{
	restart_engine();
}

static duk_ret_t
js_sleep(duk_context* ctx)
{
	double seconds = duk_require_number(ctx, 0);

	if (seconds < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid sleep time");
	delay(seconds);
	return 0;
}
