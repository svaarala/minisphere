#ifndef MINISPHERE__API_H__INCLUDED
#define MINISPHERE__API_H__INCLUDED

void   initialize_api         (duk_context* ctx);
void   shutdown_api           (void);
void   register_api_const     (duk_context* ctx, const char* name, double value);
void   register_api_ctor      (duk_context* ctx, const char* name, duk_c_function fn, duk_c_function finalizer);
void   register_api_function  (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn);
void   register_api_method    (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
void   register_api_module    (const char* name, duk_c_function initializer, duk_c_function finalizer);
void   register_api_prop      (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter);
void   register_api_type      (duk_context* ctx, const char* name, duk_c_function finalizer);

duk_bool_t duk_is_sphere_obj      (duk_context* ctx, duk_idx_t index, const char* ctor_name);
noreturn   duk_error_ni           (duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...);
void       duk_push_sphere_obj    (duk_context* ctx, const char* ctor_name, void* udata);
void*      duk_require_sphere_obj (duk_context* ctx, duk_idx_t index, const char* ctor_name);

#endif // MINISPHERE__API_H__INCLUDED
