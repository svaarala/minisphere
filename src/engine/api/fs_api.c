#include "minisphere.h"
#include "api/fs_api.h"

#include "api.h"
#include "utility.h"

static duk_ret_t js_exists            (duk_context* ctx);
static duk_ret_t js_mkdir             (duk_context* ctx);
static duk_ret_t js_open              (duk_context* ctx);
static duk_ret_t js_rename            (duk_context* ctx);
static duk_ret_t js_rmdir             (duk_context* ctx);
static duk_ret_t js_unlink            (duk_context* ctx);
static duk_ret_t js_File_finalize     (duk_context* ctx);
static duk_ret_t js_File_get_position (duk_context* ctx);
static duk_ret_t js_File_set_position (duk_context* ctx);
static duk_ret_t js_File_get_length   (duk_context* ctx);
static duk_ret_t js_File_close        (duk_context* ctx);
static duk_ret_t js_File_read         (duk_context* ctx);
static duk_ret_t js_File_readDouble   (duk_context* ctx);
static duk_ret_t js_File_readFloat    (duk_context* ctx);
static duk_ret_t js_File_readInt      (duk_context* ctx);
static duk_ret_t js_File_readPString  (duk_context* ctx);
static duk_ret_t js_File_readString   (duk_context* ctx);
static duk_ret_t js_File_readUInt     (duk_context* ctx);
static duk_ret_t js_File_write        (duk_context* ctx);
static duk_ret_t js_File_writeDouble  (duk_context* ctx);
static duk_ret_t js_File_writeFloat   (duk_context* ctx);
static duk_ret_t js_File_writeInt     (duk_context* ctx);
static duk_ret_t js_File_writePString (duk_context* ctx);
static duk_ret_t js_File_writeString  (duk_context* ctx);
static duk_ret_t js_File_writeUInt    (duk_context* ctx);

static bool read_vsize_int   (sfs_file_t* file, intmax_t* p_value, int size, bool little_endian);
static bool read_vsize_uint  (sfs_file_t* file, intmax_t* p_value, int size, bool little_endian);
static bool write_vsize_int  (sfs_file_t* file, intmax_t value, int size, bool little_endian);
static bool write_vsize_uint (sfs_file_t* file, intmax_t value, int size, bool little_endian);

duk_ret_t
dukopen_fs(duk_context* ctx)
{
	duk_function_list_entry functions[] = {
		{ "exists", js_exists, DUK_VARARGS },
		{ "mkdir",  js_mkdir,  DUK_VARARGS },
		{ "open",   js_open,   DUK_VARARGS },
		{ "rename", js_rename, DUK_VARARGS },
		{ "rmdir",  js_rmdir,  DUK_VARARGS },
		{ "unlink", js_unlink, DUK_VARARGS },
		{ NULL, NULL, 0 }
	};
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);
	return 1;
}

duk_ret_t
dukclose_fs(duk_context* ctx)
{
	return 0;
}

static duk_ret_t
js_exists(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false);
	duk_push_boolean(ctx, sfs_fexist(g_fs, filename, NULL));
	return 1;
}

static duk_ret_t
js_mkdir(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, "save", false);
	if (!sfs_mkdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create directory `%s`", name);
	return 0;
}

static duk_ret_t
js_open(duk_context* ctx)
{
	// fs.open(filename, mode);
	// Arguments:
	//     filename: The name of the file to open, relative to ~sgm/.
	//     mode:     A string specifying how to open the file, in the same format as
	//               would be passed to C fopen().

	sfs_file_t* file;
	const char* filename;
	const char* mode;

	filename = duk_require_path(ctx, 0, NULL, false);
	mode = duk_require_string(ctx, 1);
	file = sfs_fopen(g_fs, filename, NULL, mode);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to open `%s` with mode `%s`",
			filename, mode);
	duk_push_sphere_obj(ctx, "File", file);
	return 1;
}

static duk_ret_t
js_rename(duk_context* ctx)
{
	const char* name1;
	const char* name2;

	name1 = duk_require_path(ctx, 0, "save", false);
	name2 = duk_require_path(ctx, 1, "save", false);
	if (!sfs_rename(g_fs, name1, name2, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to rename file `%s` to `%s`", name1, name2);
	return 0;
}

static duk_ret_t
js_rmdir(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, "save", false);
	if (!sfs_rmdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to remove directory `%s`", name);
	return 0;
}

static duk_ret_t
js_unlink(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, "save", false);
	if (!sfs_unlink(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to unlink file `%s`", filename);
	return 0;
}

static duk_ret_t
js_File_finalize(duk_context* ctx)
{
	sfs_file_t* file;

	file = duk_require_sphere_obj(ctx, 0, "File");
	if (file != NULL) sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_File_get_position(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_push_number(ctx, sfs_ftell(file));
	return 1;
}

static duk_ret_t
js_File_set_position(duk_context* ctx)
{
	sfs_file_t* file;
	long long   new_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	new_pos = duk_require_number(ctx, 0);
	sfs_fseek(file, new_pos, SFS_SEEK_SET);
	return 0;
}

static duk_ret_t
js_File_get_length(duk_context* ctx)
{
	sfs_file_t* file;
	long        file_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	file_pos = sfs_ftell(file);
	sfs_fseek(file, 0, SEEK_END);
	duk_push_number(ctx, sfs_ftell(file));
	sfs_fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_File_close(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_push_pointer(ctx, NULL);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
	sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_File_read(duk_context* ctx)
{
	// File:read([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	int          num_bytes;
	long         pos;

	argc = duk_get_top(ctx);
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "string length must be zero or greater");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	num_bytes = (int)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_buffer_object(ctx, -1, 0, num_bytes, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_File_readDouble(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	double      value;

	int i;

	argc = duk_get_top(ctx);
	little_endian = argc >= 1 ? duk_require_boolean(ctx, 0) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (sfs_fread(data, 1, 8, file) != 8)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read double from file");
	if (little_endian == is_cpu_little_endian())
		memcpy(&value, data, 8);
	else {
		for (i = 0; i < 8; ++i)
			((uint8_t*)&value)[i] = data[7 - i];
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t
js_File_readFloat(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	float       value;

	int i;

	argc = duk_get_top(ctx);
	little_endian = argc >= 1 ? duk_require_boolean(ctx, 0) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (sfs_fread(data, 1, 4, file) != 4)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read float from file");
	if (little_endian == is_cpu_little_endian())
		memcpy(&value, data, 4);
	else {
		for (i = 0; i < 4; ++i)
			((uint8_t*)&value)[i] = data[3 - i];
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t
js_File_readInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	num_bytes = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "int byte size must be in [1-6] range");
	if (!read_vsize_int(file, &value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read int from file");
	duk_push_number(ctx, (double)value);
	return 1;
}

static duk_ret_t
js_File_readPString(duk_context* ctx)
{
	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	intmax_t     length;
	bool         little_endian;
	int          uint_size;

	argc = duk_get_top(ctx);
	uint_size = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (uint_size < 1 || uint_size > 4)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "length bytes must be in [1-4] range (got: %d)", uint_size);
	if (!read_vsize_uint(file, &length, uint_size, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read pstring from file");
	buffer = malloc((size_t)length);
	if (sfs_fread(buffer, 1, (size_t)length, file) != (size_t)length)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read pstring from file");
	duk_push_lstring(ctx, buffer, (size_t)length);
	free(buffer);
	return 1;
}

static duk_ret_t
js_File_readString(duk_context* ctx)
{
	// File:readString([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	int          num_bytes;
	long         pos;

	argc = duk_get_top(ctx);
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "string length must be zero or greater");
	buffer = malloc(num_bytes);
	num_bytes = (int)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_lstring(ctx, buffer, num_bytes);
	free(buffer);
	return 1;
}

static duk_ret_t
js_File_readUInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	int         num_bytes;
	intmax_t     value;

	argc = duk_get_top(ctx);
	num_bytes = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "uint byte size must be in [1-6] range");
	if (!read_vsize_uint(file, &value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read uint from file");
	duk_push_number(ctx, (double)value);
	return 1;
}

static duk_ret_t
js_File_write(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  num_bytes;

	duk_require_stack_top(ctx, 1);
	data = duk_require_buffer_data(ctx, 0, &num_bytes);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (sfs_fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write data to file");
	return 0;
}

static duk_ret_t
js_File_writeDouble(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	double      value;

	int i;

	argc = duk_get_top(ctx);
	value = duk_require_number(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (little_endian == is_cpu_little_endian())
		memcpy(data, &value, 8);
	else {
		for (i = 0; i < 8; ++i)
			data[i] = ((uint8_t*)&value)[7 - i];
	}
	if (sfs_fwrite(data, 1, 8, file) != 8)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write double to file");
	return 0;
}

static duk_ret_t
js_File_writeFloat(duk_context* ctx)
{
	int         argc;
	uint8_t     data[4];
	sfs_file_t* file;
	bool        little_endian;
	float       value;

	int i;

	argc = duk_get_top(ctx);
	value = duk_require_number(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (little_endian == is_cpu_little_endian())
		memcpy(data, &value, 4);
	else {
		for (i = 0; i < 4; ++i)
			data[i] = ((uint8_t*)&value)[3 - i];
	}
	if (sfs_fwrite(data, 1, 4, file) != 4)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write float to file");
	return 0;
}

static duk_ret_t
js_File_writeInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    min_value;
	intmax_t    max_value;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	value = (intmax_t)duk_require_number(ctx, 0);
	num_bytes = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "int byte size must be in [1-6] range");
	min_value = pow(-2, num_bytes * 8 - 1);
	max_value = pow(2, num_bytes * 8 - 1) - 1;
	if (value < min_value || value > max_value)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "value is unrepresentable in `%d` bytes", num_bytes);
	if (!write_vsize_int(file, value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write int to file");
	return 0;
}

static duk_ret_t
js_File_writePString(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    max_len;
	intmax_t    num_bytes;
	const char* string;
	duk_size_t  string_len;
	int         uint_size;

	argc = duk_get_top(ctx);
	string = duk_require_lstring(ctx, 0, &string_len);
	uint_size = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (uint_size < 1 || uint_size > 4)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "length bytes must be in [1-4] range", uint_size);
	max_len = pow(2, uint_size * 8) - 1;
	num_bytes = (intmax_t)string_len;
	if (num_bytes > max_len)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "string is too long for `%d`-byte length", uint_size);
	if (!write_vsize_uint(file, num_bytes, uint_size, little_endian)
		|| sfs_fwrite(string, 1, string_len, file) != string_len)
	{
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write pstring to file");
	}
	return 0;
}

static duk_ret_t
js_File_writeString(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  num_bytes;

	duk_require_stack_top(ctx, 1);
	data = duk_get_lstring(ctx, 0, &num_bytes);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (sfs_fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write string to file");
	return 0;
}

static duk_ret_t
js_File_writeUInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    max_value;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	value = (intmax_t)duk_require_number(ctx, 0);
	num_bytes = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "uint byte size must be in [1-6] range");
	max_value = pow(2, num_bytes * 8) - 1;
	if (value < 0 || value > max_value)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "value is unrepresentable in `%d` bytes", num_bytes);
	if (!write_vsize_uint(file, value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write int to file");
	return 0;
}

static bool
read_vsize_int(sfs_file_t* file, intmax_t* p_value, int size, bool little_endian)
{
	// NOTE: supports decoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!

	uint8_t data[6];
	int     mul = 1;

	int i;

	if (sfs_fread(data, 1, size, file) != size)
		return false;

	// variable-sized int decoding adapted from Node.js
	if (little_endian) {
		*p_value = data[i = 0];
		while (++i < size && (mul *= 0x100))
			*p_value += data[i] * mul;
	}
	else {
		*p_value = data[i = size - 1];
		while (i > 0 && (mul *= 0x100))
			*p_value += data[--i] * mul;
	}
	if (*p_value >= mul * 0x80)
		*p_value -= pow(2, 8 * size);

	return true;
}

static bool
read_vsize_uint(sfs_file_t* file, intmax_t* p_value, int size, bool little_endian)
{
	// NOTE: supports decoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!

	uint8_t data[6];
	int     mul = 1;

	int i;

	if (sfs_fread(data, 1, size, file) != size)
		return false;

	// variable-sized uint decoding adapted from Node.js
	if (little_endian) {
		*p_value = data[i = 0];
		while (++i < size && (mul *= 0x100))
			*p_value += data[i] * mul;
	}
	else {
		*p_value = data[--size];
		while (size > 0 && (mul *= 0x100))
			*p_value += data[--size] * mul;
	}

	return true;
}

static bool
write_vsize_int(sfs_file_t* file, intmax_t value, int size, bool little_endian)
{
	// NOTE: supports encoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!

	uint8_t data[6];
	int     mul = 1;
	int     sub = 0;

	int i;

	// variable-sized int encoding adapted from Node.js
	if (little_endian) {
		data[i = 0] = value & 0xFF;
		while (++i < size && (mul *= 0x100)) {
			if (value < 0 && sub == 0 && data[i - 1] != 0)
				sub = 1;
			data[i] = (value / mul - sub) & 0xFF;
		}
	}
	else {
		data[i = size - 1] = value & 0xFF;
		while (--i >= 0 && (mul *= 0x100)) {
			if (value < 0 && sub == 0 && data[i + 1] != 0)
				sub = 1;
			data[i] = (value / mul - sub) & 0xFF;
		}
	}

	return sfs_fwrite(data, 1, size, file) == size;
}

static bool
write_vsize_uint(sfs_file_t* file, intmax_t value, int size, bool little_endian)
{
	// NOTE: supports encoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!

	uint8_t data[6];
	int     mul = 1;

	int i;

	// variable-sized uint encoding adapted from Node.js
	if (little_endian) {
		data[i = 0] = value & 0xFF;
		while (++i < size && (mul *= 0x100))
			data[i] = (value / mul) & 0xFF;
	}
	else {
		data[i = size - 1] = value & 0xFF;
		while (--i >= 0 && (mul *= 0x100))
			data[i] = (value / mul) & 0xFF;
	}

	return sfs_fwrite(data, 1, size, file) == size;
}
