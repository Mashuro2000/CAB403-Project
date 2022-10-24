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

// read in license plates from file
void read_allowed_plates(){

}

// cars simulated locally in simulation file, but update LPR values
// cars update LPR value of entrance, updates LPR of level it goes to, waits updates LPR of level it leaves, updates LPR of its chosen exit

// generate car 50% chance of being on allowed list
// add to linked list of car queue at one of the entrances
void generate_car(){

}

// move car, updates LPR of place where car moves to
void move_car(struct LPR *leaving, struct LPR *entering){

}


int main(int argc, int * argv){


    shm_fd = shm_open("PARKING", O_CREAT, 0);
	shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

}