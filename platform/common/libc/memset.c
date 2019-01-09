#include <string.h>
#undef memset

void *
memset(void *s, int c, size_t n)
{
	char *m = s;

	while (n-- > 0)
		*m++ = c;
	return s;
}
