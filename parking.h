#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

//original SHMSZ
#define SHMSZ 2920

#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5
#define PLATESIZE 6 //  6 character plate
#define NUM_ALLOW_CARS 100


//#define LPR_ENT_SIZE 96 Use sizeof() with type
#define ENT_GAP 288

#define EXT_OFFSET (ENTRANCES*ENT_GAP)
//#define LPR_EXT_SIZE 96
#define EXT_GAP 192

#define LVL_OFFSET (EXT_OFFSET + EXITS*EXT_GAP)
//#define LPR_LVL_SIZE 96
#define LVL_GAP 104

#define BOOM_SIZE 96
//#define INF_SIZE 



typedef struct LPR LPR;
struct LPR {
    pthread_mutex_t m;
    pthread_cond_t c;
    char plate[6];
	char padding[2];
};

typedef struct boomgate boomgate;
struct boomgate {
	pthread_mutex_t m;
	pthread_cond_t c;
	char s;
	char padding[7];
};

typedef struct infosign infosign;
struct infosign {
	pthread_mutex_t m;
	pthread_cond_t c;
	char display;
	char padding[7];
};

typedef struct temp_alarm temp_alarm;
struct temp_alarm{
	int tempsensor;
	char falarm;
	char padding[5];
};


// localised (not in shared memory)
/* REMOVED
struct level{
    struct LPR *lpr;
    int tempsensor;
    char falarm;

};*/

/* REMOVED
struct parkinglot{
    struct level l[LEVELS];
};*/

struct tempnode {
	int temperature;
	struct tempnode *next;
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