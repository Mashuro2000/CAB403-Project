#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "parking.h"


int shm_fd;
volatile void *shm;
// needed so that a car can be generated that is allowed
char allowed_cars[NUM_ALLOW_CARS][PLATESIZE];

//local linked list struct for cars within each level or waiting at entrance/exit
// not in shared mem
struct cars{
    char lplate[6];
    struct car *next;
};

// setup parking levels
void init(){

}

// read in license plates from file
void read_allowed_plates(){

}

// cars simulated locally in simulation file, but update LPR values
// cars update LPR value of entrance, updates LPR of level it goes to, waits updates LPR of level it leaves, updates LPR of its chosen exit

// 1 if allowed, 0 if not
int checklicense(char *plate){

}
// generate car, allocating license plate, 50% chance of being on allowed list
// add to linked list of car queue at one of the entrances
void generate_car(){

}

// enter carpark, updates LPR of place where car moves to
void enter_carpark(struct cars * queue ,struct LPR *entrance, struct parkinglot plot){

}

void exit_carpark(struct cars * queue ,struct LPR *exit, struct level *lvl){
    //update LPR as car exits

    // cleans up memory for car
}

// get random time
int randtime(int min, int max){

}

void simulate_car(){
    generate_car();
    // 
}


void *openboomgate(void *arg)
{
	struct boomgate *bg = arg;
	pthread_mutex_lock(&bg->m);
	for (;;) {
		if (bg->s == 'C') {
			bg->s = 'R';
			pthread_cond_broadcast(&bg->c);
		}
		if (bg->s == 'O') {
		}
		pthread_cond_wait(&bg->c, &bg->m);
	}
	pthread_mutex_unlock(&bg->m);
	
}
void *closeboomgate(void *arg){

}

void *change_temp(void *lvl_addr){

}

// trigger when simulating fire, aggressively change temperature on one level
void *simulate_fire(void *lvl_addr){

}

// simulate environment (temps)
void simulate_env(){
    //seperate threads for changing temps on each level
}

int main(int argc, int * argv){


    shm_fd = shm_open("PARKING", O_CREAT, 0);
	shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

}