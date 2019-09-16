#include "string_api.h"

int separator_count(const char *src, const char *spearator){
    int i = 0;
    int count = 0;
    int s_len;
    if(NULL == src||NULL == spearator){
        return -1;
    }
    s_len = strlen(spearator);
    while(src[i]){
        if(!memcmp(src+i, spearator, s_len)){
            count++;
            i +=s_len;
        }else{
            ++i;
        }
    }
    return count;
}

int split(char *src, char **dst, const char *spearator){
    char *p;
    int i = 1;
    p = strtok(src, spearator);
    memcpy(*dst, p, strlen(p));
    while((p = strtok(NULL, spearator))){
        memcpy(*(dst+i), p, strlen(p));
        printf("dst:%s\n", *(dst+i));
        ++i;
    }
    return i;
}
