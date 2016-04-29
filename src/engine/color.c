#include "minisphere.h"
#include "color.h"

struct colormat
{
	unsigned int refcount;
	float        m[5][5];
};

ALLEGRO_COLOR
nativecolor(color_t color)
{
	return al_map_rgba(color.r, color.g, color.b, color.alpha);
}

color_t
color_new(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
	color_t color;

	color.r = r;
	color.g = g;
	color.b = b;
	color.alpha = alpha;
	return color;
}

color_t
color_lerp(color_t color, color_t other, float w1, float w2)
{
	color_t blend;
	float   sigma;

	sigma = w1 + w2;
	blend.r = (color.r * w1 + other.r * w2) / sigma;
	blend.g = (color.g * w1 + other.g * w2) / sigma;
	blend.b = (color.b * w1 + other.b * w2) / sigma;
	blend.alpha = (color.alpha * w1 + other.alpha * w2) / sigma;
	return blend;
}
