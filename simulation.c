#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "parking.h"

int shm_fd;
volatile void *shm;
// needed so that a car can be generated that is allowed
char allowed_cars[NUM_ALLOW_CARS][PLATESIZE];

// local linked list struct for cars within each level or waiting at entrance/exit
// not in shared mem
struct cars{
    char lplate[PLATESIZE];
    struct car *next;
};

// struct to pass address and a number to args
struct addr_num_args{
	void *addr;
	int num;
};

struct addr_str_args{
	void *addr;
	char *str;
};

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

// cars simulated locally in simulation file, but update LPR values
// cars update LPR value of entrance, updates LPR of level it goes to, waits updates LPR of level it leaves, updates LPR of its chosen exit

// 1 if allowed, 0 if not, allows car generator to see if it has generated a car with the correct plate
int checklicense_forcargen(char *plate){
	for(int i = 0; i < NUM_ALLOW_CARS; i++){
		if (strcmp(plate, allowed_cars[i]) == 0){
			return 1;
		}
	}
	return 0;
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

/**********************Car park Temperature *******************/
void *change_temp(void *args){
	struct addr_num_args *numargs = (struct addr_num_args *)args;
	struct level *lvl = numargs->addr;

	// no mutex lock required, only one thread at a time should be changing the temperature in simulation exe

	lvl->tempsensor = numargs->num;
	/*pthread_mutex_lock(&lvl->m);
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
	*/
}

//expects args with lvl address and number
void *change_LPR(void *args){
	struct addr_str_args *strargs;
	strargs = (struct addr_str_args *)args;
	struct level *lvl = strargs->addr;

	pthread_mutex_lock(&lvl->lpr->m);
	strcpy(lvl->lpr, strargs->str);
	pthread_mutex_unlock(&lvl->lpr->m);
}

// trigger when simulating fire, aggressively change temperature on one level
void *simulate_fire(void *lvl_addr){

}

void *simulate_temp(){
    int delay = 0.002;
    int Temp;
    int baseTemp = 20, maxTemp = 58;
 
    
    srand(time(NULL));
  
    // Record the median temp of 5 recent temps as a smoothed temp value

    //Fixed temp fire detection
    //Fire alarm starts reading after 30 smoothed temps
    //If 90% of smoothed temps is 58 degress or higher -> set off fire alarm

    // Rate of fire detection
    // if most recent temp is 8 degreese higher than most recent smoothed temp than set off alarm


    //Create manual method of setting off fire alarm
    for(;;){
       Temp = (rand() % (maxTemp - baseTemp + 1)) + baseTemp;
       if(Temp >= maxTemp) {

           printf("Fire has started\n");
           break; 
       }
       else{
                printf("Temperature: %d degrees\n", Temp);
                sleep(delay);        
       }
    }
}

// simulate environment (temps)
void simulate_env(){
    //seperate threads for changing temps on each level
	pthread_t pthread1, pthread2, pthread3, pthread4, pthread5;

	/*
	level1_tempThread = pthread_create(&pthread1, NULL, simulate_temp, NULL); 
	level2_tempThread = pthread_create(&pthread2, NULL, simulate_temp, NULL);
	level3_tempThread = pthread_create(&pthread3, NULL, simulate_temp, NULL);
	level4_tempThread = pthread_create(&pthread4, NULL, simulate_temp, NULL);
	level5_tempThread = pthread_create(&pthread5, NULL, simulate_temp, NULL);
	*/
}

void set_firealarm(){
	
}

// setup parking levels, initial temp and sensor values
void init(){
	
	// set all temps to initial 25C

	pthread_t *tempsetthreads = malloc(sizeof(pthread_t) * LEVELS);
	for (int i = 0; i < LEVELS; i++) {
		volatile struct addr_num_args *args = malloc(sizeof(struct addr_num_args));
		args->addr = shm + 104 * i + 2496; //temp sensors start at 2496, spaced by 104 bytes
		args->num = 25;
		//volatile struct boomgate *bg = shm + *args->addraddr;
		pthread_create(tempsetthreads + i, NULL, change_temp, args);

		free(args);
	}

	// set fire alarms on all levels to 0
	pthread_t *falarmsetthreads = malloc(sizeof(pthread_t) * LEVELS);
	for (int i = 0; i < LEVELS; i++) {
		volatile struct addr_num_args *args = malloc(sizeof(struct addr_num_args));
		args->addr = shm + 104 * i + 2498; //fire alarms start at 2498, spaced by 104 bytes
		args->num = 0;
		//volatile struct boomgate *bg = shm + *args->addraddr;
		pthread_create(falarmsetthreads + i, NULL, change_temp, args);

		free(args);
	}

	// set levels LPRs
	pthread_t *LPRLevelssetthreads = malloc(sizeof(pthread_t) * LEVELS);
	for (int i = 0; i < LEVELS; i++) {
		volatile struct addr_str_args *args = malloc(sizeof(struct addr_str_args));
		args->addr = shm + 104 * i + 2400;
		strcpy(args->str, "------");
		//volatile struct boomgate *bg = shm + *args->addraddr;
		pthread_create(LPRLevelssetthreads + i, NULL, change_LPR, args);

		free(args);
	}

	pthread_t *LPREntExsetthreads = malloc(sizeof(pthread_t) * (ENTRANCES + EXITS));
	for (int i = 0; i < LEVELS; i++) {
		volatile struct addr_str_args *args = malloc(sizeof(struct addr_str_args));
		args->addr = shm + 288 * i + 0;
		strcpy(args->str, "------");
		//volatile struct boomgate *bg = shm + *args->addraddr;
		pthread_create(LPREntExsetthreads + i, NULL, change_LPR, args);

		free(args);
	}

	// close all boomgates
	pthread_t *boomgatethreads = malloc(sizeof(pthread_t) * (ENTRANCES + EXITS));
	for (int i = 0; i < ENTRANCES; i++) {
		int addr = 288 * i + 96;
		volatile struct boomgate *bg = shm + addr;
		pthread_create(boomgatethreads + i, NULL, closeboomgate, bg);
	}
	for (int i = 0; i < EXITS; i++) {
		int addr = 192 * i + 1536;
		volatile struct boomgate *bg = shm + addr;
		pthread_create(boomgatethreads + ENTRANCES + i, NULL, closeboomgate, bg);
	}
}


int main(int argc, int * argv){


    shm_fd = shm_open("PARKING", O_CREAT, 0);
	shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	// TESTING SECTION
	read_allowed_plates_from_file();

	for(int i = 0; i < NUM_ALLOW_CARS; i++){
		printf("%s\n", allowed_cars[i]);
	}

	if(checklicense_forcargen("510SLS")){
		printf("Found car!\n");
	}
	// TESTING SECTION
	munmap((void *)shm, 2920);
	close(shm_fd);

}
