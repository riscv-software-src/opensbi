#include <string.h>
#undef strlen

size_t
strlen(const char *s)
{
	const char *t;

	for (t = s; *t; ++t)
		;
	return t - s;
}
