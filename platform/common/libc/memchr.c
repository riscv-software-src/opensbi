#include <string.h>
#undef memchr

void *
memchr(const void *s, int c, size_t n)
{
	const unsigned char *bp = s;

	while (n > 0 && *bp++ != c)
		--n;
	return (n == 0) ? NULL : (void*)bp-1;
}
