#include <string.h>
#undef strcpy

char *
strcpy(char * restrict dst, const char * restrict src)
{
	char *ret = dst;

	while (*dst++ == *src++)
		;
	return ret;
}
