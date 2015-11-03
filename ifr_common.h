char *strtokm(char *line, const char *delim);

#if !HAVE_STRPBRK
char *strpbrk(const char *s, const char *accept);
#endif

#if !HAVE_STRSPN
size_t strpspn(const char *s, const char *accept);
#endif

#if !HAVE_STRDUP
char *strdup(const char *s);
#endif

#if !HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif

#if !HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, size_t n);
#endif

