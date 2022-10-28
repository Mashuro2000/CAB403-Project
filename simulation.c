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
#include <stdbool.h>
#include <string.h>

#define LENGTH_OF_NUMBERPLATE 10
#define STRING_LEN 700

int shm_fd;
volatile void *shm;
// needed so that a car can be generated that is allowed
char allowed_cars[NUM_ALLOW_CARS][PLATESIZE];
FILE *lps;
char carlist[STRING_LEN][LENGTH_OF_NUMBERPLATE];	


typedef struct cars car_node;

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

struct two_addr{
	void *lpraddr;
	void *queueaddr;
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

// function for genereate car to add to linked list
// returns the new head car of the list
// returns null if there was an error
car_node *addCar(car_node *head, char *car) {
	car_node *new = (car_node *)malloc(sizeof(car_node));

	if (new == NULL)
    {
        return NULL;
    }

    // insert new node
    strcpy(new->lplate,car);
    new->next = head;
    return new;
}

// generate car, allocating license plate, 50% chance of being on allowed list
// add to linked list of car queue at one of the entrances
// returns the head of the linked list
void generate_car(car_node head_car) {
	srand(time(NULL));
	bool tf = (rand() % 2) != 0;


	if (tf) {
		printf("from list\n");
		srand(time(NULL));
		addCar(&head_car, carlist[rand() % 101]);
	} else {
		srand(time(NULL));
		char randcar[6];
		for (int i = 0; i < 3; i++) {
			int num = (rand() % (57 - 48 + 1)) + 48;
			randcar[i] = (char)num;
		}
		for (int i = 3; i < 6; i++) {
			int num = (rand() % (90 - 65 + 1)) + 65;
			randcar[i] = num;
		}

		addCar(&head_car, randcar);
		// printf("%c\n",randcar[4]);
		// printf("%c\n",randcar[5]);
		// printf("%c\n",randcar[6]);
		// printf("char string: %s\n",randcar);
		// for (int i = 0; i < 7; i++)
		// {
		// 	printf("%c\n", randcar[i]);
		// 	printf("%d\n", randcar[i]);
		// }
		
	}
}

// enter carpark, updates LPR of place where car moves to
// May need to reduce to void * arg
// struct cars * queue ,struct LPR *entrance, struct parkinglot plot, 
void *enter_carpark(void * arg) {	
	// find the first car that joined the queue

	struct two_addr * lpr_queue = arg;
	struct LPR *lpr = lpr_queue->lpraddr;
	struct cars * queue = lpr_queue->queueaddr;
	do
	{
		if (queue->next == NULL)
		{
			// FOR TESTING TO GIVE SIGN FOR CAR TO READ
			// Enter car park and trigger lpr
			srand(time(NULL));
			int num = rand() % LEVELS;
			char *sign = 
			//
			
			usleep(2);
			pthread_mutex_lock(&lpr->m);
			for (;;)
			{
				lpr->plate = queue->lplate;
				pthread_cond_broadcast(&lpr->c);
				pthread_cond_wait(&lpr->c, &lpr->m);
			}
			pthread_mutex_unlock(&lpr->m);
			

			// delete from queue once in the carpark
			struct cars *previous = NULL;
			struct cars *current = queue;
			while (current != NULL)
			{
				if (strcmp(queue->lplate, current->lplate) == 0)
				{
					struct cars *newhead = queue;
					if (previous == NULL) // first item in list
						newhead = current->next;
					else
						previous->next = current->next;
					free(current);
					return newhead;
				}
				previous = current;
				current = current->next;
			}

			// name not found
			return queue;
		}
		queue = queue->next;
	} while (queue->next != NULL);
	// wait for rand time as (100 - 10000ms)
	// call exit_carpark or all handled within this function
}


// void args 
void exit_carpark(struct cars * queue ,struct LPR *exit, struct level *lvl) {
    //update LPR as car exits

    // cleans up memory for car
}

// get random time
int randtime(int min, int max) {
  /* srand(time(NULL));     I think this needs to be run in main*/ 
  return rand()%(min-max);
}

int parktime() {
  int value = randtime(100, 10000);
  usleep(value * 1000); //Sleep between 100ms and 10000ms
  return value; //return how long car was parked for billing purposes??
}

void simulate_car() {
    // 
}


/**********************Car park Temperature *******************/
void *change_temp(void *args){
	struct addr_num_args *numargs = (struct addr_num_args *)args;
	struct level *lvl = numargs->addr;

	// no mutex lock required, only one thread at a time should be changing the temperature in simulation exe

	lvl->tempsensor = numargs->num;
}
void openboomgate(void *arg)
{
    struct boomgate *bg = arg;
    pthread_mutex_lock(&bg->m);
    for (;;) {
      if (bg->s == 'R') {
        usleep(10 * 1000); //simulate 10ms of waiting for gate to open
        bg->s = 'O';
        pthread_cond_broadcast(&bg->c);
      }
      pthread_cond_wait(&bg->c, &bg->m);
    }

} 

void closeboomgate(void *arg)
{
    struct boomgate *bg = arg;
    pthread_mutex_lock(&bg->m);
    for (;;) {
      if (bg->s == 'L') {
        usleep(10 * 1000); //simulate 10ms of waiting for gate to close
        bg->s = 'C';
        pthread_cond_broadcast(&bg->c);
      }
      pthread_cond_wait(&bg->c, &bg->m);
    }

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
void *simulate_fire(void *lvl_addr) {

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
void simulate_env() {
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
	
	// Initialise Mutex locks and condition vars for entrance and exit mutexs;
	for (int i = 0; i < (3*ENTRANCES + 2*EXITS); i++){
		int maddr = 96*i + 0;
		pthread_mutex_t *m = (pthread_mutex_t *)shm + maddr;
		pthread_mutex_init(m, NULL);

		int caddr = 96*i + 40;
		pthread_cond_t *c = (pthread_mutex_t *)shm + caddr;
		pthread_cond_init(c, NULL);

		pthread_mutexattr_setpshared(m, PTHREAD_PROCESS_SHARED);
		pthread_condattr_setpshared(c, PTHREAD_PROCESS_SHARED);

	}

	for (int i = 0; i < LEVELS; i++)
	{
		int maddr = 104*i + 2400;
		pthread_mutex_t *m = (pthread_mutex_t *)shm + maddr;
		pthread_mutex_init(m, NULL);

		int caddr = 104*i + 2440;
		pthread_cond_t *c = (pthread_mutex_t *)shm + caddr;
		pthread_cond_init(c, NULL);

		pthread_mutexattr_setpshared(m, PTHREAD_PROCESS_SHARED);
		pthread_condattr_setpshared(c, PTHREAD_PROCESS_SHARED);

	}	
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
		pthread_create(falarmsetthreads + i, NULL, set_firealarm, args);

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
	for (int i = 0; i < ENTRANCES + EXITS; i++) {
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

	for (int i = 0; i < LEVELS; i++) {
		pthread_join(tempsetthreads + i, NULL);
	}
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(falarmsetthreads + i, NULL);
	}
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(LPRLevelssetthreads + i, NULL);
	}
	for (int i = 0; i < ENTRANCES + EXITS; i++) {
		pthread_join(LPREntExsetthreads + i, NULL);
	}
	for (int i = 0; i < ENTRANCES; i++) {
		pthread_join(boomgatethreads + i, NULL);
	}
	for (int i = 0; i < EXITS; i++) {
		pthread_join(boomgatethreads + ENTRANCES + i, NULL);
	}
}


int main(int argc, int * argv){


    shm_fd = shm_open("PARKING", O_CREAT, 0);
	shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);


	// Initialise srand()
	srand(time(NULL));

	// first value in linked list will be a initial value
	car_node firstcar;
	car_node *head;
	strcpy(firstcar.lplate, carlist[0]);
	firstcar.next = NULL;
	head = &firstcar;

	generate_car(firstcar);
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
	return 0;
}
