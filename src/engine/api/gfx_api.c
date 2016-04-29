#include "minisphere.h"
#include "api/gfx_api.h"

#include "api.h"
#include "image.h"
#include "matrix.h"
#include "primitives.h"
#include "shader.h"

static duk_ret_t js_get_frameRate        (duk_context* ctx);
static duk_ret_t js_set_frameRate        (duk_context* ctx);
static duk_ret_t js_flip                 (duk_context* ctx);
static duk_ret_t js_setResolution        (duk_context* ctx);
static duk_ret_t js_new_Color            (duk_context* ctx);
static duk_ret_t js_Color_clone          (duk_context* ctx);
static duk_ret_t js_Color_blend          (duk_context* ctx);
static duk_ret_t js_new_Group            (duk_context* ctx);
static duk_ret_t js_Group_finalize       (duk_context* ctx);
static duk_ret_t js_Group_get_shader     (duk_context* ctx);
static duk_ret_t js_Group_get_transform  (duk_context* ctx);
static duk_ret_t js_Group_set_shader     (duk_context* ctx);
static duk_ret_t js_Group_set_transform  (duk_context* ctx);
static duk_ret_t js_Group_draw           (duk_context* ctx);
static duk_ret_t js_Group_setFloat       (duk_context* ctx);
static duk_ret_t js_Group_setInt         (duk_context* ctx);
static duk_ret_t js_Group_setMatrix      (duk_context* ctx);
static duk_ret_t js_new_Image            (duk_context* ctx);
static duk_ret_t js_Image_finalize       (duk_context* ctx);
static duk_ret_t js_Image_get_height     (duk_context* ctx);
static duk_ret_t js_Image_get_width      (duk_context* ctx);
static duk_ret_t js_new_Shader           (duk_context* ctx);
static duk_ret_t js_Shader_finalize      (duk_context* ctx);
static duk_ret_t js_new_Shape            (duk_context* ctx);
static duk_ret_t js_Shape_finalize       (duk_context* ctx);
static duk_ret_t js_Shape_get_texture    (duk_context* ctx);
static duk_ret_t js_Shape_set_texture    (duk_context* ctx);
static duk_ret_t js_Shape_draw           (duk_context* ctx);
static duk_ret_t js_new_Surface          (duk_context* ctx);
static duk_ret_t js_Surface_finalize     (duk_context* ctx);
static duk_ret_t js_Surface_toImage      (duk_context* ctx);
static duk_ret_t js_Surface_get_height   (duk_context* ctx);
static duk_ret_t js_Surface_get_width    (duk_context* ctx);
static duk_ret_t js_new_Transform        (duk_context* ctx);
static duk_ret_t js_Transform_finalize   (duk_context* ctx);
static duk_ret_t js_Transform_compose    (duk_context* ctx);
static duk_ret_t js_Transform_identity   (duk_context* ctx);
static duk_ret_t js_Transform_rotate     (duk_context* ctx);
static duk_ret_t js_Transform_scale      (duk_context* ctx);
static duk_ret_t js_Transform_translate  (duk_context* ctx);

duk_ret_t
dukopen_gfx(duk_context* ctx)
{
	register_api_type(g_duk, "Color", NULL);
	register_api_method(g_duk, "Color", "clone", js_Color_clone);
	register_api_method(g_duk, "Color", "blend", js_Color_blend);

	register_api_type(ctx, "Group", js_Group_finalize);
	register_api_prop(ctx, "Group", "shader", js_Group_get_shader, js_Group_set_shader);
	register_api_prop(ctx, "Group", "transform", js_Group_get_transform, js_Group_set_transform);
	register_api_method(ctx, "Group", "draw", js_Group_draw);
	register_api_method(ctx, "Group", "setFloat", js_Group_setFloat);
	register_api_method(ctx, "Group", "setInt", js_Group_setInt);
	register_api_method(ctx, "Group", "setMatrix", js_Group_setMatrix);

	register_api_type(ctx, "Image", js_Image_finalize);
	register_api_prop(ctx, "Image", "height", js_Image_get_height, NULL);
	register_api_prop(ctx, "Image", "width", js_Image_get_width, NULL);

	register_api_type(ctx, "Shader", js_Shader_finalize);

	register_api_type(ctx, "Shape", js_Shape_finalize);
	register_api_prop(ctx, "Shape", "texture", js_Shape_get_texture, js_Shape_set_texture);
	register_api_method(ctx, "Shape", "draw", js_Shape_draw);

	register_api_type(ctx, "Surface", js_Surface_finalize);
	register_api_method(ctx, "Surface", "toImage", js_Surface_toImage);
	register_api_prop(ctx, "Surface", "height", js_Surface_get_height, NULL);
	register_api_prop(ctx, "Surface", "width", js_Surface_get_width, NULL);

	register_api_type(ctx, "Transform", js_Transform_finalize);
	register_api_method(ctx, "Transform", "compose", js_Transform_compose);
	register_api_method(ctx, "Transform", "identity", js_Transform_identity);
	register_api_method(ctx, "Transform", "rotate", js_Transform_rotate);
	register_api_method(ctx, "Transform", "scale", js_Transform_scale);
	register_api_method(ctx, "Transform", "translate", js_Transform_translate);

	duk_function_list_entry functions[] = {
		{ "setResolution",    js_setResolution,    DUK_VARARGS },
		{ "flip",             js_flip,             DUK_VARARGS },
		{ "Color",            js_new_Color,        DUK_VARARGS },
		{ "Group",            js_new_Group,        DUK_VARARGS },
		{ "Shader",           js_new_Shader,       DUK_VARARGS },
		{ "Shape",            js_new_Shape,        DUK_VARARGS },
		{ "Surface",          js_new_Surface,      DUK_VARARGS },
		{ "Transform",        js_new_Transform,    DUK_VARARGS },
		{ NULL, NULL, 0 }
	};
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);

	duk_push_string(ctx, "frameRate");
	duk_push_c_function(ctx, js_get_frameRate, DUK_VARARGS);
	duk_push_c_function(ctx, js_set_frameRate, DUK_VARARGS);
	duk_def_prop(ctx, -4, DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE
		| DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);

	duk_push_string(ctx, "screen");
	duk_push_sphere_obj(ctx, "Surface", NULL);
	duk_def_prop(ctx, -3, DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_SET_CONFIGURABLE
		| DUK_DEFPROP_HAVE_VALUE);

	return 1;
}

duk_ret_t
dukclose_gfx(duk_context* ctx)
{
	return 0;
}

void
duk_push_sphere_color(duk_context* ctx, color_t color)
{
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Color");
	duk_push_number(ctx, color.r);
	duk_push_number(ctx, color.g);
	duk_push_number(ctx, color.b);
	duk_push_number(ctx, color.alpha);
	duk_new(ctx, 4);
	duk_remove(ctx, -2);
}

color_t
duk_require_sphere_color(duk_context* ctx, duk_idx_t index)
{
	int r, g, b;
	int alpha;

	duk_require_sphere_obj(ctx, index, "Color");
	duk_get_prop_string(ctx, index, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); alpha = duk_get_int(ctx, -1); duk_pop(ctx);
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
	return color_new(r, g, b, alpha);
}

static duk_ret_t
js_get_frameRate(duk_context* ctx)
{
	duk_push_int(ctx, g_framerate);
	return 1;
}

static duk_ret_t
js_set_frameRate(duk_context* ctx)
{
	g_framerate = duk_require_int(ctx, 0);
	return 0;
}

static duk_ret_t
js_flip(duk_context* ctx)
{
	screen_flip(g_screen, g_framerate);
	return 0;
}

static duk_ret_t
js_setResolution(duk_context* ctx)
{
	int  res_width;
	int  res_height;

	res_width = duk_require_int(ctx, 0);
	res_height = duk_require_int(ctx, 1);

	if (res_width < 0 || res_height < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "screen resolution cannot be negative (got X: %d, Y: %d)",
			res_width, res_height);
	screen_resize(g_screen, res_width, res_height);
	return 0;
}

static duk_ret_t
js_new_Color(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int r = duk_require_int(ctx, 0);
	int g = duk_require_int(ctx, 1);
	int b = duk_require_int(ctx, 2);
	int alpha = n_args >= 4 ? duk_require_int(ctx, 3) : 255;

	// clamp components to 8-bit [0-255]
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;

	// construct a Color object
	duk_push_sphere_obj(ctx, "Color", NULL);
	duk_push_int(ctx, r); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, g); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, b); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, alpha); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

static duk_ret_t
js_Color_clone(duk_context* ctx)
{
	color_t color;

	duk_push_this(ctx);
	color = duk_require_sphere_color(ctx, -1);

	duk_push_sphere_color(ctx, color);
	return 1;
}

static duk_ret_t
js_Color_blend(duk_context* ctx)
{
	color_t color;
	color_t target_color;
	double  weight;

	duk_push_this(ctx);
	color = duk_require_sphere_color(ctx, -1);
	weight = duk_require_number(ctx, 0);
	target_color = duk_require_sphere_color(ctx, 1);

	weight = fmin(fmax(weight, 0.0), 1.0);
	duk_push_sphere_color(ctx, color_lerp(color, target_color, weight, 1.0 - weight));
	return 1;
}

static duk_ret_t
js_new_Group(duk_context* ctx)
{
	duk_require_object_coercible(ctx, 0);
	shader_t* shader = duk_require_sphere_obj(ctx, 1, "Shader");

	size_t    num_shapes;
	group_t*  group;
	shape_t*  shape;

	duk_uarridx_t i;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "argument 1 of Shape() must be an array");
	if (!(group = group_new(shader)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create shape group");
	num_shapes = duk_get_length(ctx, 0);
	for (i = 0; i < num_shapes; ++i) {
		duk_get_prop_index(ctx, 0, i);
		shape = duk_require_sphere_obj(ctx, -1, "Shape");
		group_add_shape(group, shape);
	}
	duk_push_sphere_obj(ctx, "Group", group);
	return 1;
}

static duk_ret_t
js_Group_finalize(duk_context* ctx)
{
	group_t* group;

	group = duk_require_sphere_obj(ctx, 0, "Group");
	group_free(group);
	return 0;
}

static duk_ret_t
js_Group_get_shader(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");

	duk_push_sphere_obj(ctx, "Shader", group_get_shader(group));
	return 1;
}

static duk_ret_t
js_Group_get_transform(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");

	duk_push_sphere_obj(ctx, "Transform", group_get_transform(group));
	return 1;
}

static duk_ret_t
js_Group_set_shader(duk_context* ctx)
{
	group_t*  group;
	shader_t* shader;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	shader = duk_require_sphere_obj(ctx, 0, "Shader");

	group_set_shader(group, shader);
	return 0;
}

static duk_ret_t
js_Group_set_transform(duk_context* ctx)
{
	group_t*  group;
	matrix_t* matrix;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	matrix = duk_require_sphere_obj(ctx, 0, "Transform");

	group_set_transform(group, matrix);
	return 0;
}

static duk_ret_t
js_Group_draw(duk_context* ctx)
{
	group_t* group;
	int      num_args;
	image_t* surface;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	group = duk_require_sphere_obj(ctx, -1, "Group");
	surface = num_args >= 2
		? duk_require_sphere_obj(ctx, 0, "Surface")
		: NULL;

	if (!screen_is_skipframe(g_screen))
		group_draw(group, surface);
	return 0;
}

static duk_ret_t
js_Group_setFloat(duk_context* ctx)
{
	group_t*    group;
	const char* name;
	float       value;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	value = duk_require_number(ctx, 1);

	group_put_float(group, name, value);
	return 1;
}

static duk_ret_t
js_Group_setInt(duk_context* ctx)
{
	group_t*    group;
	const char* name;
	int         value;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	value = duk_require_int(ctx, 1);

	group_put_int(group, name, value);
	return 1;
}

static duk_ret_t
js_Group_setMatrix(duk_context* ctx)
{
	group_t*    group;
	matrix_t*   matrix;
	const char* name;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	matrix = duk_require_sphere_obj(ctx, 1, "Transform");

	group_put_matrix(group, name, matrix);
	return 1;
}

static duk_ret_t
js_new_Image(duk_context* ctx)
{
	const color_t* buffer;
	size_t         buffer_size;
	const char*    filename;
	color_t        fill_color;
	int            height;
	image_t*       image;
	image_lock_t*  lock;
	int            num_args;
	color_t*       p_line;
	int            width;

	int y;

	num_args = duk_get_top(ctx);
	if (num_args >= 3 && duk_is_sphere_obj(ctx, 2, "Color")) {
		// create an Image filled with a single pixel value
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = duk_require_sphere_color(ctx, 2);
		if (!(image = create_image(width, height)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): unable to create new image");
		fill_image(image, fill_color);
	}
	else if (num_args >= 3 && (buffer = duk_get_buffer_data(ctx, 2, &buffer_size))) {
		// create an Image from an ArrayBuffer or similar object
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		if (buffer_size < width * height * sizeof(color_t))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "buffer is too small to describe a %dx%d image", width, height);
		if (!(image = create_image(width, height)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create image");
		if (!(lock = lock_image(image))) {
			free_image(image);
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to lock pixels for writing");
		}
		p_line = lock->pixels;
		for (y = 0; y < height; ++y) {
			memcpy(p_line, buffer + y * width, width * sizeof(color_t));
			p_line += lock->pitch;
		}
		unlock_image(image, lock);
	}
	else {
		// create an Image by loading an image file
		filename = duk_require_path(ctx, 0, NULL, false);
		image = load_image(filename);
		if (image == NULL)
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load image `%s`", filename);
	}
	duk_push_sphere_obj(ctx, "Image", image);
	return 1;
}

static duk_ret_t
js_Image_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_sphere_obj(ctx, 0, "Image");
	free_image(image);
	return 0;
}

static duk_ret_t
js_Image_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Image");
	
	duk_push_int(ctx, get_image_height(image));
	return 1;
}

static duk_ret_t
js_Image_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Image");

	duk_push_int(ctx, get_image_width(image));
	return 1;
}

static duk_ret_t
js_new_Shader(duk_context* ctx)
{
	const char* fs_filename;
	const char* vs_filename;
	shader_t*   shader;

	if (!duk_is_object(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): JS object expected as argument");
	if (duk_get_prop_string(ctx, 0, "vertex"), !duk_is_string(ctx, -1))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): 'vertex' property, string required");
	if (duk_get_prop_string(ctx, 0, "fragment"), !duk_is_string(ctx, -1))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): 'fragment' property, string required");
	duk_pop_2(ctx);

	if (!are_shaders_active())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): shaders not supported on this system");

	duk_get_prop_string(ctx, 0, "vertex");
	duk_get_prop_string(ctx, 0, "fragment");
	vs_filename = duk_require_path(ctx, -2, NULL, false);
	fs_filename = duk_require_path(ctx, -1, NULL, false);
	duk_pop_2(ctx);
	if (!(shader = shader_new(vs_filename, fs_filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): failed to build shader from `%s`, `%s`", vs_filename, fs_filename);
	duk_push_sphere_obj(ctx, "Shader", shader);
	return 1;
}

static duk_ret_t
js_Shader_finalize(duk_context* ctx)
{
	shader_t* shader = duk_require_sphere_obj(ctx, 0, "Shader");

	shader_free(shader);
	return 0;
}

static duk_ret_t
js_new_Shape(duk_context* ctx)
{
	bool         is_missing_uv = false;
	int          num_args;
	size_t       num_vertices;
	shape_t*     shape;
	duk_idx_t    stack_idx;
	image_t*     texture;
	shape_type_t type;
	vertex_t     vertex;

	duk_uarridx_t i;

	num_args = duk_get_top(ctx);
	duk_require_object_coercible(ctx, 0);
	type = num_args >= 2 ? duk_require_int(ctx, 2) : SHAPE_AUTO;
	texture = num_args >= 3 && !duk_is_null(ctx, 1)
		? duk_require_sphere_obj(ctx, 1, "Image")
		: NULL;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Shape(): first argument must be an array");
	if (type < 0 || type >= SHAPE_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Shape(): invalid shape type constant");
	if (!(shape = shape_new(type, texture)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Shape(): unable to create shape object");
	num_vertices = duk_get_length(ctx, 0);
	for (i = 0; i < num_vertices; ++i) {
		duk_get_prop_index(ctx, 0, i); stack_idx = duk_normalize_index(ctx, -1);
		vertex.x = duk_get_prop_string(ctx, stack_idx, "x") ? duk_require_number(ctx, -1) : 0.0f;
		vertex.y = duk_get_prop_string(ctx, stack_idx, "y") ? duk_require_number(ctx, -1) : 0.0f;
		if (duk_get_prop_string(ctx, stack_idx, "u"))
			vertex.u = duk_require_number(ctx, -1);
		else
			is_missing_uv = true;
		if (duk_get_prop_string(ctx, stack_idx, "v"))
			vertex.v = duk_require_number(ctx, -1);
		else
			is_missing_uv = true;
		vertex.color = duk_get_prop_string(ctx, stack_idx, "color")
			? duk_require_sphere_color(ctx, -1)
			: color_new(255, 255, 255, 255);
		duk_pop_n(ctx, 6);
		if (!shape_add_vertex(shape, vertex))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Shape(): vertex list allocation failure");
	}
	if (is_missing_uv)
		shape_calculate_uv(shape);
	shape_upload(shape);
	duk_push_sphere_obj(ctx, "Shape", shape);
	return 1;
}

static duk_ret_t
js_Shape_finalize(duk_context* ctx)
{
	shape_t* shape;

	shape = duk_require_sphere_obj(ctx, 0, "Shape");
	shape_free(shape);
	return 0;
}

static duk_ret_t
js_Shape_get_texture(duk_context* ctx)
{
	shape_t* shape;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");

	duk_push_sphere_obj(ctx, "Image", ref_image(shape_texture(shape)));
	return 1;
}

static duk_ret_t
js_Shape_set_texture(duk_context* ctx)
{
	shape_t* shape;
	image_t* texture;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	texture = duk_require_sphere_obj(ctx, 0, "Image");

	shape_set_texture(shape, texture);
	return 0;
}

static duk_ret_t
js_Shape_draw(duk_context* ctx)
{
	matrix_t* matrix;
	int       num_args;
	shader_t* shader;
	shape_t*  shape;
	image_t*  target;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	num_args = duk_get_top(ctx) - 1;
	target = num_args >= 1 ? duk_require_sphere_obj(ctx, 0, "Surface") : NULL;
	matrix = num_args >= 2 ? duk_require_sphere_obj(ctx, 1, "Transform") : NULL;
	shader = num_args >= 3 ? duk_require_sphere_obj(ctx, 1, "Shader") : NULL;

	shape_draw(shape, target, matrix, shader);
	return 0;
}

static duk_ret_t
js_new_Surface(duk_context* ctx)
{
	int      height;
	image_t* image;
	int      width;

	width = duk_require_int(ctx, 0);
	height = duk_require_int(ctx, 1);
	image = create_image(width, height);
	
	duk_push_sphere_obj(ctx, "Surface", image);
	return 1;
}

static duk_ret_t
js_Surface_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_sphere_obj(ctx, 0, "Surface");
	free_image(image);
	return 0;
}

static duk_ret_t
js_Surface_toImage(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if (!(image = clone_image(image)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "internal: unable to create image");
	duk_push_sphere_obj(ctx, "Image", image);
	return 1;
}

static duk_ret_t
js_Surface_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	duk_push_int(ctx, image != NULL ? get_image_height(image) : g_res_y);
	return 1;
}

static duk_ret_t
js_Surface_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	duk_push_int(ctx, image != NULL ? get_image_width(image) : g_res_x);
	return 1;
}

static duk_ret_t
js_new_Transform(duk_context* ctx)
{
	matrix_t* matrix;

	matrix = matrix_new();
	duk_push_sphere_obj(ctx, "Transform", matrix);
	return 1;
}

static duk_ret_t
js_Transform_finalize(duk_context* ctx)
{
	matrix_t* matrix;

	matrix = duk_require_sphere_obj(ctx, 0, "Transform");
	matrix_free(matrix);
	return 0;
}

static duk_ret_t
js_Transform_compose(duk_context* ctx)
{
	matrix_t* matrix;
	matrix_t* other_matrix;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	other_matrix = duk_require_sphere_obj(ctx, 0, "Transform");

	matrix_compose(matrix, other_matrix);
	return 0;
}

static duk_ret_t
js_Transform_identity(duk_context* ctx)
{
	matrix_t* matrix;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	matrix_identity(matrix);
	return 0;
}

static duk_ret_t
js_Transform_rotate(duk_context* ctx)
{
	matrix_t* matrix;
	float     theta;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	theta = duk_require_number(ctx, 0);

	matrix_rotate(matrix, theta, 0.0, 0.0, 1.0);
	return 0;
}

static duk_ret_t
js_Transform_scale(duk_context* ctx)
{
	matrix_t* matrix;
	float     sx;
	float     sy;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	sx = duk_require_number(ctx, 0);
	sy = duk_require_number(ctx, 1);

	matrix_scale(matrix, sx, sy, 1.0);
	return 0;
}

static duk_ret_t
js_Transform_translate(duk_context* ctx)
{
	matrix_t* matrix;
	float     tx;
	float     ty;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	tx = duk_require_number(ctx, 0);
	ty = duk_require_number(ctx, 1);

	matrix_translate(matrix, tx, ty, 0.0);
	return 0;
}
