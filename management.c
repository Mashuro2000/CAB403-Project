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


LPR *ent_lpr_addr[ENTRANCES];
boomgate *ent_boom_addr[ENTRANCES];
infosign *ent_info_addr[ENTRANCES];

LPR *ext_lpr_addr[EXITS];
boomgate *ext_boom_addr[EXITS];

LPR *lvl_lpr_addr[EXITS];
temp_alarm *lvl_tmpalrm_addr[EXITS];

void init(){
	// MAP memory spaces
	// Initial memory space
	if ((ent_lpr_addr[0] = mmap(0, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 

	// MAP rest of entrances
	for (int i = 0; i < ENTRANCES; i++){
		if ((ent_lpr_addr[i] = mmap(ent_lpr_addr[0] + ENT_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		if ((ent_boom_addr[i] = mmap(ent_lpr_addr[0] +sizeof(LPR) + ENT_GAP*i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		if ((ent_info_addr[i] = mmap(ent_lpr_addr[0] +sizeof(LPR) + sizeof(boomgate) + ENT_GAP*i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
	}

	// map exits
	for (int i = 0; i < EXITS; i++){
		if ((ext_lpr_addr[i] = mmap(ent_lpr_addr[0]+ EXT_OFFSET + EXT_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		if ((ext_boom_addr[i] = mmap(ent_lpr_addr[0] + EXT_OFFSET +sizeof(LPR) + EXT_GAP*i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 	
	}

	// map levels
	for (int i = 0; i < LEVELS; i++){
		if ((lvl_lpr_addr[i] = mmap(ent_lpr_addr[0]+ LVL_OFFSET + LVL_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		if ((lvl_tmpalrm_addr[i] = mmap(ent_lpr_addr[0] + LVL_OFFSET + sizeof(LPR) + LVL_GAP*i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 	
	}
}

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

// read in license plates from file
void read_allowed_plates_from_file(){
	FILE* fptr;

	// open license plate files
	fptr = fopen("plates.txt", "r");

	if (fptr == NULL){
		printf("Authorised Plates File not Found - Simulation\n");
	}

	char tmp;
	for (int i = 0; i < NUM_ALLOW_CARS; i++){
		fgets(allowed_cars[i], PLATESIZE, fptr);
		fgetc(fptr); //consume new line character
	}
}

//1 if allowed, 0 if not, allows management sytem to know whether to allow entry
int checklicense_forentry(char *plate){

}

int main(int argc, int *argv){
    //read in license plate file

    // open shared memory
    shm_fd = shm_open("PARKING", O_RDWR, 0666);
	//shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	init();


    	
	munmap((void *)shm, 2920);
	close(shm_fd);
}