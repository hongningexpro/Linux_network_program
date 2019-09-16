#ifndef __STRINGAPI_H_
#define __STRINGAPI_H_
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int separator_count(const char *src, const char *separator);
int split(char *src, char **dst, const char *separator);


#endif
