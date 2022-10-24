#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5
#define PLATESIZE 6
#define NUM_ALLOW_CARS 100

struct boomgate {
	pthread_mutex_t m;
	pthread_cond_t c;
	char s;
};
struct parkingsign {
	pthread_mutex_t m;
	pthread_cond_t c;
	char display;
};

struct tempnode {
	int temperature;
	struct tempnode *next;
};

struct LPR {
    pthread_mutex_t m;
    pthread_cond_t c;
    char plate[6];
};

struct levels{
    struct LPR *lpr;
    struct tempnode *tnode;
    char falarm;

};



