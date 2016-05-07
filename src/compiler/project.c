#include "cell.h"
#include "project.h"

struct project
{
	duk_context* js_ctx;
	duk_idx_t    manifest_idx;
	path_t*      path;
};

project_t*
project_new(const path_t* path, const char* title)
{
	duk_context* js_ctx;
	path_t*      file_path;
	project_t*   proj;

	if (!(file_path = path_resolve(path_dup(path), NULL)))
		return NULL;
	path_append(file_path, "game.s2gm");

	js_ctx = duk_create_heap_default();
	duk_push_object(js_ctx);
	duk_push_string(js_ctx, title);
	duk_put_prop_string(js_ctx, -2, "name");

	proj = calloc(1, sizeof(project_t));
	proj->js_ctx = js_ctx;
	proj->manifest_idx = duk_normalize_index(js_ctx, -1);
	proj->path = file_path;
	
	printf("preparing new manifest `%s`\n", path_cstr(file_path));
	project_set_title(proj, "Untitled");
	project_set_author(proj, "Author Unknown");
	project_set_res(proj, 320, 240);
	return proj;
}

project_t*
project_open(const path_t* path)
{
	path_t*      file_path;
	duk_context* js_ctx;
	size_t       json_size;
	char*        json_text;
	project_t*   proj;

	file_path = path_dup(path);
	if (!path_append(file_path, "game.s2gm") || !path_resolve(file_path, NULL))
		goto on_error;
	if (!(json_text = fslurp(path_cstr(file_path), &json_size)))
		goto on_error;
	js_ctx = duk_create_heap_default();
	duk_push_lstring(js_ctx, json_text, json_size);
	duk_json_decode(js_ctx, -1);

	proj = calloc(1, sizeof(project_t));
	proj->js_ctx = js_ctx;
	proj->manifest_idx = duk_normalize_index(js_ctx, -1);
	proj->path = file_path;
	return proj;

on_error:
	path_free(file_path);
	return NULL;
}

void
project_free(project_t* proj)
{
	duk_destroy_heap(proj->js_ctx);
	free(proj);
}

void
project_set_api_level(project_t* proj, int level)
{
	duk_push_int(proj->js_ctx, level);
	duk_put_prop_string(proj->js_ctx, proj->manifest_idx, "apiLevel");
}

void
project_set_author(project_t* proj, const char* name)
{
	duk_push_string(proj->js_ctx, name);
	duk_put_prop_string(proj->js_ctx, proj->manifest_idx, "author");
}

void
project_set_res(project_t* proj, int width, int height)
{
	duk_push_sprintf(proj->js_ctx, "%dx%d", width, height);
	duk_put_prop_string(proj->js_ctx, proj->manifest_idx, "resolution");
}

void
project_set_summary(project_t* proj, const char* summary)
{
	duk_push_string(proj->js_ctx, summary);
	duk_put_prop_string(proj->js_ctx, proj->manifest_idx, "summary");
}

void
project_set_title(project_t* proj, const char* title)
{
	duk_push_string(proj->js_ctx, title);
	duk_put_prop_string(proj->js_ctx, proj->manifest_idx, "name");
}

void
project_save(project_t* proj)
{
	FILE*   file;
	path_t* path;

	printf("writing manifest `%s`\n", path_cstr(proj->path));
	
	duk_dup(proj->js_ctx, proj->manifest_idx);
	duk_json_encode(proj->js_ctx, -1);
	path = path_resolve(path_new("./"), NULL);
	path_append(path, "game.s2gm");
	if (!(file = fopen(path_cstr(path), "wt"))) {
		printf("error: unable to save `%s`\n", path_cstr(path));
		goto on_finished;
	}
	fputs(duk_get_string(proj->js_ctx, -1), file);
	fclose(file);

on_finished:
	path_free(path);
}
