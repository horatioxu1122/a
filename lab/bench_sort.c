/* bench bubble sort vs qsort at -O2 and -O3. build: sh bench_sort.c */
#if 0
set -e; F="$(cd "$(dirname "$0")";pwd)/bench_sort.c"; T=$(mktemp -d)
for O in O2 O3; do clang -$O -o "$T/s_$O" "$F" && echo "=== -$O ===" && "$T/s_$O"; done
rm -rf "$T"; exit
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct{char n[64];int c;}FC;
static int ctcmp(const void*a,const void*b){return((const FC*)b)->c-((const FC*)a)->c;}
static void bubble(FC*ct,int nc){
    for(int i=0;i<nc-1;i++)for(int j=i+1;j<nc;j++)if(ct[j].c>ct[i].c){
        char tn[64];int tc=ct[i].c;memcpy(tn,ct[i].n,64);
        ct[i].c=ct[j].c;memcpy(ct[i].n,ct[j].n,64);ct[j].c=tc;memcpy(ct[j].n,tn,64);}
}
static void fill(FC*ct,int n){for(int i=0;i<n;i++){snprintf(ct[i].n,64,"item%d",i);ct[i].c=rand()%10000;}}
static double bench(void(*fn)(FC*,int),int n,int reps){
    FC*ct=malloc((size_t)n*sizeof(FC));FC*tmp=malloc((size_t)n*sizeof(FC));
    fill(ct,n);
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int r=0;r<reps;r++){memcpy(tmp,ct,(size_t)n*sizeof(FC));fn(tmp,n);}
    clock_gettime(CLOCK_MONOTONIC,&t1);
    free(ct);free(tmp);
    return((double)(t1.tv_sec-t0.tv_sec)*1e9+(double)(t1.tv_nsec-t0.tv_nsec))/(double)reps;
}
static void do_qsort(FC*ct,int n){qsort(ct,(size_t)n,sizeof(FC),ctcmp);}
int main(void){
    srand(42);
    int sizes[]={16,64,256,1024};
    printf("  %6s  %12s  %12s  %s\n","N","bubble(ns)","qsort(ns)","winner");
    for(int i=0;i<4;i++){
        int n=sizes[i],reps=n<256?100000:n<1024?10000:1000;
        double b=bench(bubble,n,reps),q=bench(do_qsort,n,reps);
        printf("  %6d  %12.0f  %12.0f  %s\n",n,b,q,b<q?"bubble":"qsort");
    }
    return 0;
}
