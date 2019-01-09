#include <string.h>
#undef memcmp

int
memcmp(const void *s1, const void *s2, size_t n)
{
	const char *s = s1;
	const char *t = s2;

	for ( ; n > 0 && *s == *t; --n)
		++s, ++t;

	return (n > 0) ? *(unsigned char *) s - *(unsigned char *) t : 0;
}
