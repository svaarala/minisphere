#include "minisphere.h"
#include "rng.h"

#include "mt19937ar.h"

static const char* const RNG_STRING_CORPUS =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static long s_corpus_size;

void
initialize_rng(void)
{
	unsigned long seed;
	
	console_log(1, "initializing random number generator");
	
	seed = (unsigned long)time(NULL);
	console_log(2, "    seed: %lu", seed);
	init_genrand(seed);

	s_corpus_size = (long)strlen(RNG_STRING_CORPUS);
}

void
seed_rng(unsigned long seed)
{
	console_log(2, "reseeding Mersenne Twister");
	console_log(2, "    seed: %lu", seed);
	
	init_genrand(seed);
}

bool
rng_chance(double odds)
{
	return odds > genrand_real2();
}

double
rng_normal(double mean, double sigma)
{
	static bool   s_have_y = false;
	static double s_y;

	double u, v, w;
	double x;

	if (!s_have_y) {
		do {
			u = 2.0 * genrand_res53() - 1.0;
			v = 2.0 * genrand_res53() - 1.0;
			w = u * u + v * v;
		} while (w >= 1.0);
		w = sqrt(-2 * log(w) / w);
		x = u * w;
		s_y = v * w;
		s_have_y = true;
	}
	else {
		x = s_y;
		s_have_y = false;
	}
	return mean + x * sigma;
}

double
rng_random(void)
{
	return genrand_res53();
}

long
rng_ranged(long lower, long upper)
{
	long range = abs(upper - lower) + 1;
	return (lower < upper ? lower : upper)
		+ genrand_int31() % range;
}

const char*
rng_string(int length)
{
	static char s_name[256];

	long index;
	
	int i;

	for (i = 0; i < length; ++i) {
		index = rng_ranged(0, s_corpus_size - 1);
		s_name[i] = RNG_STRING_CORPUS[index];
	}
	s_name[length - 1] = '\0';
	return s_name;
}

double
rng_uniform(double mean, double variance)
{
	double error;
	
	error = variance * 2 * (0.5 - genrand_real2());
	return mean + error;
}
