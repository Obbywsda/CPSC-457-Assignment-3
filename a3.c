#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//cpsc 457 assignment 3

#define MAX_REFS 300000
#define MAX_PAGES 100000

static int P[MAX_REFS];
static unsigned char D[MAX_REFS];
static int N=0;
static int page_hi=-1;

typedef struct{
    int count;
    int *pos;
    int ptr;
}PosList;

typedef struct{
    int page;
    int dirty;
    int load_time;
    uint32_t rbits;
}Frame;

static PosList perpage[MAX_PAGES];

//reads csv from stdin 
static void read_csv(void){

    char line[256];

    //read the header line
    if(!fgets(line,sizeof(line), stdin) || strstr(line, "Page#")==NULL){
        printf("missing header\n");
        exit(1);
    }

    //read each data line "page,dirty"
    while(fgets(line,sizeof(line), stdin)){

        int pg;
        int di;

        sscanf(line,"%d ,%d", &pg, &di);
        P[N]=pg;
        D[N]=di?1:0;
        if(pg>page_hi){
            page_hi=pg;
        }
        N++;
    }
    page_hi+=1;
}

//builds positions for each page for opt
static void build_positions(void){

    for(int i = 0; i < page_hi; i++){
        perpage[i].count=0;
        perpage[i].pos=NULL;
        perpage[i].ptr=0;
    }

    //count occurrences
    for(int i=0;i<N;i++) perpage[P[i]].count++;

    //allocate arrays for positions
    for(int pg = 0; pg < page_hi; pg++){
        if(perpage[pg].count>0){
            perpage[pg].pos=malloc(sizeof(int)*perpage[pg].count);
        }
    }

    //fill positions
    int *filled=calloc(page_hi,sizeof(int));

    for(int i=0;i<N;i++){
        int pg=P[i];
        perpage[pg].pos[filled[pg]++]=i;
    }
    free(filled);
}

//resets next-use pointers for opt
static void reset_opt_ptrs(void){
    for(int pg=0; pg < page_hi; pg++){
        perpage[pg].ptr=0;
    }
}

//advances ptr to first occurrence strictly after i
static void advance_ptr_past(int pg,int i){

    PosList *pl=&perpage[pg];

    while(pl->ptr<pl->count && pl->pos[pl->ptr]<=i){
        pl->ptr++;
    }
}

//returns index of oldest-loaded frame (fifo tie-break)
static int oldest_index(Frame *f,int used){

    int idx = 0;
    int best=f[0].load_time;

    for(int i=1;i<used;i++){
        if(f[i].load_time<best){
            best=f[i].load_time;
            idx=i;
        }
    }
    return idx;
}

static void print_hdr_frames(const char *name){

    printf("%s\n", name);
    printf("+----------+----------------+-----------------+\n");
    printf("| Frames   | Page Faults    | Write-backs     |\n");
    printf("+----------+----------------+-----------------+\n");
}

static void print_row_frames(int F,long long pf,long long wb){

    printf("| %8d | %14lld | %15lld |\n",F,pf,wb);
    printf("+----------+----------------+-----------------+\n");
}


static void run_fifo(int F, long long *faults, long long *writes){

    *faults = 0;
    *writes = 0;

    Frame *frames = malloc(sizeof(Frame)*F);
    int used = 0;
    int time = 0;

    for(int i = 0; i < N; i++){

        int p = P[i];
        int d = D[i];
        time++;

        //check hit
        int hit=-1;
        for(int j = 0; j < used; j++){

            if(frames[j].page==p){
            hit=j;
            break;
            }
        }

        //on hit update dirty
        if(hit >= 0){
            if(d&&!frames[hit].dirty) {
                frames[hit].dirty=1;
            }
            continue;
        }

        //on miss place or evict
        (*faults)++;

        if(used < F){
            frames[used].page = p;
            frames[used].dirty = d;
            frames[used].load_time = time;
            frames[used].rbits = 0;
            used++;

        }
        else{
            int k=oldest_index(frames,used);
            if(frames[k].dirty){
                (*writes)++;
            }
            frames[k].page = p;
            frames[k].dirty = d;
            frames[k].load_time = time;
            frames[k].rbits = 0;
        }
    }
    free(frames);
}
