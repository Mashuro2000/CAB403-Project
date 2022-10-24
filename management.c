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
char allowed_cars[NUM_ALLOW_CARS][PLATESIZE];

// Managment system can only read sensors and keep track of cars that way, along with issuing commands to
// boom gates and display screens, and time keeping and generating billing

void display(){
    // number of vehicles on each level
    // max capacity on each level
    // current state of each LPR
    // total billing revenue recorded by manager
    // temp sensors

}

int main(int argc, int *argv){
    //read in license plate file

    // open shared memory
    shm_fd = shm_open("PARKING", O_RDWR, 0);
	shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
}