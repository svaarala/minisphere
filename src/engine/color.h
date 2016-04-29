#ifndef MINISPHERE__COLOR_H__INCLUDED
#define MINISPHERE__COLOR_H__INCLUDED

typedef struct colormat colormat_t;

typedef
struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t alpha;
} color_t;

ALLEGRO_COLOR nativecolor (color_t color);
color_t       color_new   (uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
color_t       color_lerp  (color_t color, color_t other, float w1, float w2);

#endif // MINISPHERE__COLOR_H__INCLUDED
