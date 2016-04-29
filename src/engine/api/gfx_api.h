#ifndef MINISPHERE__GFX_API_H__INCLUDED
#define MINISPHERE__GFX_API_H__INCLUDED

duk_ret_t     dukopen_gfx                    (duk_context* ctx);
duk_ret_t     dukclose_gfx                   (duk_context* ctx);
void          duk_push_sphere_color          (duk_context* ctx, color_t color);
color_t       duk_require_sphere_color       (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__GFX_API_H__INCLUDED
