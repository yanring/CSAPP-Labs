#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct
{
    int vaild;
    int tag;
    int timestamp;
} cacheline;
int hit=0,miss=0,eviction=0;
int load(int address, int offset, int index_group,int E, cacheline** cache){
    cacheline *cache_group = cache[index_group];
    int outdated_index = 0;
    cacheline *item;
    time_t t;
    t = time(&t);
    for(int i = 0; i < E; i++){
        item  = &cache_group[i];
        if(cache_group[outdated_index].timestamp>item->timestamp){
            outdated_index = i;
        }
        if(address==item->tag){
            item->timestamp = t;
            item->vaild=1;
            hit++;
            printf(" hit");
            return 1;
        }
    }
    item = & cache_group[outdated_index];
    if(item->timestamp != -1){
        eviction++;
        printf(" eviction");
    }
    item->tag = address;
    item->timestamp = t;
    item->vaild = 1;
    miss++;
    printf(" miss");
    return 0;
}
long hex2int(char* str){
    int len = strlen(str);
    long res = 0;
    for (int i=0; i < len; i++){
        char c = str[i];
        res = res*16;
        if(c>='0'&&c<='9') res+= c-'0';
	    if(c>='a'&&c<='f') res+= c-'a'+10;
	    if(c>='A'&&c<='F') res+= c-'A'+10;
    }
    return res;

}
int main(int argc, char * const argv[])
{
    int opt,s,S,E,b,i;
    char str1[20],str2[20],str3[20],filename[50], buf[100];
    FILE *fp;
    cacheline **cache;
    long address = 0;
    int index_group, offset;
    while((opt = getopt(argc,argv,"vs:E:b:t:"))!=-1){
        // printf("opt = %c, optarg = %s, optind = %d, argv[%d] = %s\n", 
        //                         opt, optarg, optind, optind, argv[optind]);
        switch(opt){
            case 's':
                s=atoi(optarg);
                S=1<<s;
            case 'E':
                E=atoi(optarg);
            case 'b':
                b=atoi(optarg);
            case 't':
                strcpy(filename,optarg);
        }
    }
    cache = (cacheline **)malloc(sizeof(cacheline *)*S);
    for (i=0; i<S; i++){
        cache[i] = (cacheline *)malloc(sizeof(cacheline)*E);
        for(int j = 0; j <E; j++){
            cache[i][j].timestamp=-1;
            cache[i][j].vaild=-1;
            cache[i][j].tag=-1;
        }
    }
    fp = fopen(filename,"r");
    while(fgets(buf, sizeof buf, fp)){
        sscanf(buf,"%s %[^,],%s",str1,str2,str3);
        address = hex2int(str2);
        offset=0;
        index_group=0;
        if(str1[0]=='I')
            continue;
        for(i=0; i<b; i++){
            offset=2*offset+(address&1);
            address>>=1;
        }
        for(i=0; i<s; i++){
            index_group=2*index_group+(address&1);
            address>>=1;
        }
        printf("%s | %s | index:%d | add:%ld | hit:%d ",str1,str2,index_group,address,hit);
        load(address,offset,index_group,E,cache);
        if(str1[0]=='M')
            load(address,offset,index_group,E,cache);
        printf("\n");
    }
    printSummary(hit, miss, eviction);
    for (i=0; i<S; i++){
        free(cache[i]);
    }
    free(cache);
    return 0;
}
