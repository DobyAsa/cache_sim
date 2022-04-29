#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <strings.h>

void parse(int, char **);

void fetchIns(void *ccache, unsigned address, unsigned size);

void execute(void *cache);

void lineIn(void *ccache, unsigned sid, unsigned tid);

struct cache_line { // 一个cache行的声明
    char valid;
    unsigned tag;
    unsigned counter;
};

unsigned hit_count, miss_count, eviction_count, LRU_counter;
unsigned opt, s, E, b, S;
char *trace_path;

int main(int argc, char **argv) {
    parse(argc, argv);

    struct cache_line (*cache)[E] = (struct cache_line (*)[E]) malloc(S * E * sizeof(struct cache_line));
    memset(cache, 0, s * E * sizeof(struct cache_line));

    execute(cache);
    printf("%d %d %d", hit_count, miss_count, eviction_count);
    free(cache);
    return 0;
}

void parse(int argc, char **argv) {
    while (-1 != (opt = getopt(argc, argv, "s:E:b:t:"))) {
        switch (opt) {
            case 's':
                s = atoi(optarg);
                S = 1 << s;
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_path = optarg;
                break;
            default:
                printf("wrong argument\n");
                break;
        }
    }
}

void execute(void *cache) {
    FILE *pInstruct;
    pInstruct = fopen(trace_path, "r");
    char type;
    unsigned address;
    unsigned size;

    while (fscanf(pInstruct, " %c %x,%u", &type, &address, &size) > 0) {
        switch (type) {
            case 'M':
                fetchIns(cache, address, size);
                fetchIns(cache, address, size);
                break;
            case 'L':
            case 'S':
                fetchIns(cache, address, size);
                break;
            case 'I':
                break;
            default:
                printf("Input error!\n");
        }
    }

    fclose(pInstruct);
}

void fetchIns(void *ccache, unsigned address, unsigned size) {
    unsigned find = 0;
    struct cache_line (*cache)[E] = ccache;
    unsigned and_opn = (1 << s) - 1;
    //每次访问的大小不同，访问的地址可能分布在不同的块中

    unsigned lid = (address) >> b; //lid 即当前访问的地址所在的块编号
    unsigned sid = lid & (and_opn);   //sid 为当前块应该在哪一个set
    unsigned tid = lid >> s;
    for (unsigned i = 0; i < E; i++) {
        if (tid == cache[sid][i].tag) {
            //如果找到了对应的tag
            find = 1;
            if (cache[sid][i].valid == 1) {
                printf("addr:%x lid:%x sid:%x tid:%x hit\n", address, lid, sid, tid);
                hit_count++;
                cache[sid][i].counter = LRU_counter++;
                break;
            } else {
                printf("addr:%x lid:%x sid:%x tid:%x miss\n", address, lid, sid, tid);
                miss_count++;
                lineIn(cache, sid, tid);
                break;
            }
        }
    }
    if (!find) {
        printf("addr:%x lid:%x sid:%x tid:%x miss\n", address, lid, sid, tid);
        miss_count++;
        lineIn(cache, sid, tid);
    }
}

void lineIn(void *ccache, unsigned sid, unsigned tid) {
    struct cache_line (*cache)[E] = ccache;
    unsigned index = 0;
    for (unsigned i = 0; i < E; i++) {
        if (cache[sid][i].valid == 0) {
            index = i;
            break;
        }
        if (cache[sid][i].counter < cache[sid][index].counter) {
            index = i;
        }
    }
    if (cache[sid][index].valid == 1) {
//        printf("eviction\n");
        eviction_count++;
    }
    cache[sid][index].tag = tid;
    cache[sid][index].valid = 1;
    cache[sid][index].counter = LRU_counter++;

}