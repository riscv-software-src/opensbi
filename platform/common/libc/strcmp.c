#include <string.h>
#undef strcmp

int
strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s2 && *s1 == *s2)
		++s1, ++s2;
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}
