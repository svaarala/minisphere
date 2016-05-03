#include "minisphere.h"
#include "api.h"

#include "file.h"

struct kevfile
{
	unsigned int    id;
	sandbox_t*      fs;
	ALLEGRO_CONFIG* conf;
	char*           filename;
	bool            is_dirty;
};

static unsigned int s_next_file_id = 0;

kevfile_t*
kev_open(sandbox_t* fs, const char* filename)
{
	kevfile_t*   file;
	ALLEGRO_FILE* memfile = NULL;
	void*         slurp;
	size_t        slurp_size;
	
	console_log(2, "opening kevfile #%u as `%s`", s_next_file_id, filename);
	file = calloc(1, sizeof(kevfile_t));
	if (slurp = sfs_fslurp(fs, filename, NULL, &slurp_size)) {
		memfile = al_open_memfile(slurp, slurp_size, "rb");
		if (!(file->conf = al_load_config_file_f(memfile)))
			goto on_error;
		al_fclose(memfile);
		free(slurp);
	}
	else {
		console_log(3, "    `%s` doesn't exist", filename);
		if (!(file->conf = al_create_config()))
			goto on_error;
	}
	file->fs = ref_sandbox(fs);
	file->filename = strdup(filename);
	file->id = s_next_file_id++;
	return file;

on_error:
	console_log(2, "    failed to open kevfile #%u", s_next_file_id++);
	if (memfile != NULL) al_fclose(memfile);
	if (file->conf != NULL)
		al_destroy_config(file->conf);
	free(file);
	return NULL;
}

void
kev_close(kevfile_t* file)
{
	if (file == NULL)
		return;
	
	console_log(3, "disposing kevfile #%u no longer in use", file->id);
	if (file->is_dirty)
		kev_save(file);
	al_destroy_config(file->conf);
	free_sandbox(file->fs);
	free(file);
}

int
kev_num_keys(kevfile_t* file)
{
	ALLEGRO_CONFIG_ENTRY* iter;
	const char*           key;
	int                   sum;

	sum = 0;
	key = al_get_first_config_entry(file->conf, NULL, &iter);
	while (key != NULL) {
		++sum;
		key = al_get_next_config_entry(&iter);
	}
	return sum;
}

const char*
kev_get_key(kevfile_t* file, int index)
{
	ALLEGRO_CONFIG_ENTRY* iter;
	const char*           name;

	int i = 0;

	name = al_get_first_config_entry(file->conf, NULL, &iter);
	while (name != NULL) {
		if (i == index)
			return name;
		++i;
		name = al_get_next_config_entry(&iter);
	}
	return NULL;
}

bool
kev_read_bool(kevfile_t* file, const char* key, bool def_value)
{
	const char* string;
	bool        value;

	string = kev_read_string(file, key, def_value ? "true" : "false");
	value = strcasecmp(string, "true") == 0;
	return value;
}

double
kev_read_float(kevfile_t* file, const char* key, double def_value)
{
	char        def_string[500];
	const char* string;
	double      value;

	sprintf(def_string, "%g", def_value);
	string = kev_read_string(file, key, def_string);
	value = atof(string);
	return value;
}

const char*
kev_read_string(kevfile_t* file, const char* key, const char* def_value)
{
	const char* value;
	
	console_log(2, "reading key `%s` from kevfile #%u", key, file->id);
	if (!(value = al_get_config_value(file->conf, NULL, key)))
		value = def_value;
	return value;
}

bool
kev_save(kevfile_t* file)
{
	void*         buffer = NULL;
	size_t        file_size;
	bool          is_aok = false;
	ALLEGRO_FILE* memfile;
	size_t        next_buf_size;
	sfs_file_t*   sfs_file = NULL;

	console_log(3, "saving kevfile #%u as `%s`", file->id, file->filename);
	next_buf_size = 4096;
	while (!is_aok) {
		buffer = realloc(buffer, next_buf_size);
		memfile = al_open_memfile(buffer, next_buf_size, "wb");
		next_buf_size *= 2;
		al_save_config_file_f(memfile, file->conf);
		is_aok = !al_feof(memfile);
		file_size = al_ftell(memfile);
		al_fclose(memfile); memfile = NULL;
	}
	if (!(sfs_file = sfs_fopen(file->fs, file->filename, NULL, "wt")))
		goto on_error;
	sfs_fwrite(buffer, file_size, 1, sfs_file);
	sfs_fclose(sfs_file);
	free(buffer);
	return true;

on_error:
	if (memfile != NULL)
		al_fclose(memfile);
	sfs_fclose(sfs_file);
	free(buffer);
	return false;
}

void
kev_write_bool(kevfile_t* file, const char* key, bool value)
{
	console_log(3, "writing boolean to kevfile #%u, key `%s`", file->id, key);
	al_set_config_value(file->conf, NULL, key, value ? "true" : "false");
	file->is_dirty = true;
}

void
kev_write_float(kevfile_t* file, const char* key, double value)
{
	char string[500];

	console_log(3, "writing number to kevfile #%u, key `%s`", file->id, key);
	sprintf(string, "%g", value);
	al_set_config_value(file->conf, NULL, key, string);
	file->is_dirty = true;
}

void
kev_write_string(kevfile_t* file, const char* key, const char* value)
{
	console_log(3, "writing string to kevfile #%u, key `%s`", file->id, key);
	al_set_config_value(file->conf, NULL, key, value);
	file->is_dirty = true;
}
