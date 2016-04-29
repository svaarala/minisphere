#include "minisphere.h"
#include "shader.h"

#include "matrix.h"

struct shader
{
	unsigned int   refcount;
#ifdef MINISPHERE_USE_SHADERS
	ALLEGRO_SHADER* program;
#endif
};

static bool s_have_shaders = false;

void
initialize_shaders(bool enable_shading)
{
	console_log(1, "initializing shader support");
#ifdef MINISPHERE_USE_SHADERS
	s_have_shaders = enable_shading;
#endif
	reset_shader();
}

void
shutdown_shaders(void)
{
	console_log(1, "shutting down shader manager");
}

bool
are_shaders_active(void)
{
	return s_have_shaders;
}

shader_t*
shader_new(const char* vs_filename, const char* fs_filename)
{
	char*      fs_source = NULL;
	char*      vs_source = NULL;
	shader_t*  shader;

	shader = calloc(1, sizeof(shader_t));
	
	if (!(vs_source = sfs_fslurp(g_fs, vs_filename, NULL, NULL)))
		goto on_error;
	if (!(fs_source = sfs_fslurp(g_fs, fs_filename, NULL, NULL)))
		goto on_error;
#ifdef MINISPHERE_USE_SHADERS
	if (!(shader->program = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	if (!al_attach_shader_source(shader->program, ALLEGRO_VERTEX_SHADER, vs_source)) {
		fprintf(stderr, "\nvertex shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_attach_shader_source(shader->program, ALLEGRO_PIXEL_SHADER, fs_source)) {
		fprintf(stderr, "\nfragment shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_build_shader(shader->program)) {
		fprintf(stderr, "\nerror building shader program:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
#endif
	free(vs_source);
	free(fs_source);
	return shader_ref(shader);

on_error:
	free(vs_source);
	free(fs_source);
#ifdef MINISPHERE_USE_SHADERS
	if (shader->program != NULL)
		al_destroy_shader(shader->program);
#endif
	free(shader);
	return NULL;
}

shader_t*
shader_ref(shader_t* shader)
{
	if (shader == NULL)
		return shader;
	++shader->refcount;
	return shader;
}

void
shader_free(shader_t* shader)
{
	if (shader == NULL || --shader->refcount > 0)
		return;
#ifdef MINISPHERE_USE_SHADERS
	al_destroy_shader(shader->program);
#endif
	free(shader);
}

bool
apply_shader(shader_t* shader)
{
#ifdef MINISPHERE_USE_SHADERS
	if (are_shaders_active()) {
		if (!al_use_shader(shader != NULL ? shader->program : NULL))
			return false;
		return true;
	}
	else {
		// if shaders are not supported, degrade gracefully. this simplifies the rest
		// of the engine, which simply assumes shaders are always supported.
		return true;
	}
#else
	return true;
#endif
}

void
reset_shader(void)
{
#ifdef MINISPHERE_USE_SHADERS
	if (s_have_shaders) al_use_shader(NULL);
#endif
}
