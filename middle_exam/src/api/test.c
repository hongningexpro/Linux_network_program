#include "string_api.h"

int main(){
    char src[] = "1-222-3-4-5-6-7";
    char **p = NULL;
    int i;
    int count = separator_count(src, "-");
    printf("count:%d\n", count);

    p = (char**)malloc(sizeof(char *)*(count+1));
    for(i = 0; i<=count;++i){
        *(p+i) = (char*)malloc(128);
        memset(*(p+i), 0, 128);
    }

    i = split(src, p, "-");
    printf("i:%d\n", i);
    i = 0;
    while(*(p[i])){
        printf("%s\n", p[i++]);
    }

    return 0;
}
