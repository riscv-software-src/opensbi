#include <string.h>
#undef memcpy

void *
memcpy(void * restrict dst, const void * restrict src, size_t n)
{
	char *s1 = dst;
	const char *s2 = src;

	while (n-- > 0)
		*s1++ = *s2++;
	return dst;
}
