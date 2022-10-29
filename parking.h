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
#define PLATESIZE 6 //  6 character plate
#define NUM_ALLOW_CARS 100
#define SHMSZ 2920

#define LPR_ENT_SIZE 96
#define LPR_ENT_GAP 288

struct LPR {
    pthread_mutex_t m;
    pthread_cond_t c;
    char plate[6];
	char padding[2];
};

struct boomgate {
	pthread_mutex_t m;
	pthread_cond_t c;
	char s;
	char padding[7];
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

struct level{
    struct LPR *lpr;
    int tempsensor;
    char falarm;

};

// localised (not in shared memory)
struct parkinglot{
    struct level l[LEVELS];
};


// license plate helper functions
void copy_plate(char * dest, char *src){
	for(int i = 0; i < PLATESIZE; i++){
		*(dest + i) = *(src + i);
	}
}
// return 0 on success
int cmp_plate(char * p1, char *p2){
	for(int i = 0; i < PLATESIZE; i++){
		if (*(p1 + i) != *(p2 + i)) return 1;
	}
	return 0;
}

void print_plate(char *p){
	for(int i = 0; i < PLATESIZE; i++){
		printf("%c", *(p+i));
	}
}