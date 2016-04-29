#include "minisphere.h"
#include "api/random_api.h"

#include "api.h"
#include "rng.h"

static duk_ret_t js_chance  (duk_context* ctx);
static duk_ret_t js_normal  (duk_context* ctx);
static duk_ret_t js_random  (duk_context* ctx);
static duk_ret_t js_range   (duk_context* ctx);
static duk_ret_t js_sample  (duk_context* ctx);
static duk_ret_t js_seed    (duk_context* ctx);
static duk_ret_t js_string  (duk_context* ctx);
static duk_ret_t js_uniform (duk_context* ctx);

duk_ret_t
dukopen_random(duk_context* ctx)
{
	initialize_rng();

	duk_function_list_entry functions[] = {
		{ "chance",  js_chance,  DUK_VARARGS },
		{ "normal",  js_normal,  DUK_VARARGS },
		{ "random",  js_random,  DUK_VARARGS },
		{ "range",   js_range,   DUK_VARARGS },
		{ "sample",  js_sample,  DUK_VARARGS },
		{ "seed",    js_seed,    DUK_VARARGS },
		{ "string",  js_string,  DUK_VARARGS },
		{ "uniform", js_uniform, DUK_VARARGS },
		{ NULL, NULL, 0 }
	};
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);
	return 1;
}

duk_ret_t
dukclose_random(duk_context* ctx)
{
	return 0;
}

static duk_ret_t
js_chance(duk_context* ctx)
{
	double odds;

	odds = duk_require_number(ctx, 0);
	duk_push_boolean(ctx, rng_chance(odds));
	return 1;
}

static duk_ret_t
js_normal(duk_context* ctx)
{
	double mean;
	double sigma;

	mean = duk_require_number(ctx, 0);
	sigma = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_normal(mean, sigma));
	return 1;
}

static duk_ret_t
js_random(duk_context* ctx)
{
	duk_push_number(ctx, rng_random());
	return 1;
}

static duk_ret_t
js_range(duk_context* ctx)
{
	long lower;
	long upper;

	lower = duk_require_number(ctx, 0);
	upper = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_ranged(lower, upper));
	return 1;
}

static duk_ret_t
js_sample(duk_context* ctx)
{
	duk_uarridx_t index;
	long          length;

	duk_require_object_coercible(ctx, 0);
	length = (long)duk_get_length(ctx, 0);
	index = rng_ranged(0, length - 1);
	duk_get_prop_index(ctx, 0, index);
	return 1;
}

static duk_ret_t
js_seed(duk_context* ctx)
{
	unsigned long new_seed;

	new_seed = duk_require_number(ctx, 0);
	seed_rng(new_seed);
	return 0;
}

static duk_ret_t
js_string(duk_context* ctx)
{
	int num_args;
	int length;

	num_args = duk_get_top(ctx);
	length = num_args >= 1 ? duk_require_number(ctx, 0)
		: 10;
	if (length < 1 || length > 255)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RNG.string(): length must be [1-255] (got: %d)", length);

	duk_push_string(ctx, rng_string(length));
	return 1;
}

static duk_ret_t
js_uniform(duk_context* ctx)
{
	double mean;
	double variance;

	mean = duk_require_number(ctx, 0);
	variance = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_uniform(mean, variance));
	return 1;
}
