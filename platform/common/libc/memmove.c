#include <string.h>
#undef memmove

void *
memmove(void *dst, const void *src, size_t n)
{
	char *d = dst, *s = (char *) src;

	if (d < s) {
		while (n-- > 0)
			*d++ = *s++;
	} else {
		s += n-1, d += n-1;
		while (n-- > 0)
			*d-- = *s--;
	}
	return dst;
}
