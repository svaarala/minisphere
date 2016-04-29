#include "minisphere.h"
#include "api/debug_api.h"

#include "api.h"
#include "debugger.h"
#include "logger.h"

static duk_ret_t js_abort             (duk_context* ctx);
static duk_ret_t js_alert             (duk_context* ctx);
static duk_ret_t js_assert            (duk_context* ctx);
static duk_ret_t js_print             (duk_context* ctx);
static duk_ret_t js_new_Logger        (duk_context* ctx);
static duk_ret_t js_Logger_finalize   (duk_context* ctx);
static duk_ret_t js_Logger_beginBlock (duk_context* ctx);
static duk_ret_t js_Logger_endBlock   (duk_context* ctx);
static duk_ret_t js_Logger_write      (duk_context* ctx);

duk_ret_t
dukopen_debug(duk_context* ctx)
{
	register_api_type(g_duk, "Logger", js_Logger_finalize);
	register_api_method(g_duk, "Logger", "beginBlock", js_Logger_beginBlock);
	register_api_method(g_duk, "Logger", "endBlock", js_Logger_endBlock);
	register_api_method(g_duk, "Logger", "write", js_Logger_write);
	
	duk_function_list_entry functions[] = {
		{ "abort",  js_abort,      DUK_VARARGS },
		{ "alert",  js_alert,      DUK_VARARGS },
		{ "assert", js_assert,     DUK_VARARGS },
		{ "print",  js_print,      DUK_VARARGS },
		{ "Logger", js_new_Logger, DUK_VARARGS },
		{ NULL, NULL, 0 }
	};
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);
	return 1;
}

duk_ret_t
dukclose_debug(duk_context* ctx)
{
	return 0;
}

static duk_ret_t
js_abort(duk_context* ctx)
{
	const char* message;
	int         num_args;

	num_args = duk_get_top(ctx);
	message = num_args >= 1
		? duk_to_string(ctx, 0)
		: "some type of weird pig ate your game! ... and you*munch*";

	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "%s", message);
}

static duk_ret_t
js_alert(duk_context* ctx)
{
	const char* caller_info;
	const char* filename;
	int         line_number;
	int         num_args;
	const char* text;

	num_args = duk_get_top(ctx);
	text = num_args >= 1 && !duk_is_null_or_undefined(ctx, 0)
		? duk_to_string(ctx, 0)
		: "It's 8:12... do you know where the pig is?\n\nIt's...\n\n\n\n\n\n\nBEHIND YOU! *MUNCH*";

	// get filename and line number of debug.alert() call
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_get_prop_string(ctx, -1, "act");
	duk_push_int(ctx, -3);
	duk_call(ctx, 1);
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
js_assert(duk_context* ctx)
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
js_print(duk_context* ctx)
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
js_new_Logger(duk_context* ctx)
{
	const char* filename;
	logger_t*   logger;

	filename = duk_require_path(ctx, 0, NULL, false);
	if (!(logger = open_log_file(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Logger(): unable to open file for logging `%s`", filename);
	duk_push_sphere_obj(ctx, "Logger", logger);
	return 1;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	logger = duk_require_sphere_obj(ctx, 0, "Logger");
	free_logger(logger);
	return 0;
}

static duk_ret_t
js_Logger_beginBlock(duk_context* ctx)
{
	const char* title = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	if (!begin_log_block(logger, title))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Log:beginBlock(): unable to create new log block");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	end_log_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	write_log_line(logger, NULL, text);
	return 0;
}
