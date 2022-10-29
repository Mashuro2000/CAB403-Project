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
void *shm; //volatile

LPR *ent_lpr_addr[ENTRANCES];
boomgate *ent_boom_addr[ENTRANCES];
infosign *ent_info_addr[ENTRANCES];

LPR *ext_lpr_addr[EXITS];
boomgate *ext_boom_addr[EXITS];

LPR *lvl_lpr_addr[EXITS];
temp_alarm *lvl_tmpalrm_addr[EXITS];

// needed so that a car can be generated that is allowed
char allowed_cars[NUM_ALLOW_CARS][PLATESIZE];
FILE *lps;
//char carlist[STRING_LEN][LENGTH_OF_NUMBERPLATE];	



// local linked list struct for cars within each level or waiting at entrance/exit
// not in shared mem
typedef struct cars car_node;
struct cars{
    char lplate[PLATESIZE];
    struct cars *next;
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


car_node *ent_queue[ENTRANCES];

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
		for(int j = 0; j < PLATESIZE; j++){
			allowed_cars[i][j] = fgetc(fptr);
		}
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
	struct cars *new = (struct cars *)malloc(sizeof(car_node));

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
		addCar(&head_car, allowed_cars[rand() % 101]);
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

	
	/*struct two_addr * lpr_queue = (struct two_addr *) arg;
	struct LPR *lpr = (struct LPR *)lpr_queue->lpraddr;
	struct cars *queue = (struct cars*)lpr_queue->queueaddr;

	sprintf((shm + 88), "%s", queue->lplate);*/
	printf("TEST CAR 0\n");

    int entrance = *(int *)arg;

	printf("Entrance %d\n", entrance);
	print_plate(ent_queue[entrance]->lplate);

	printf("TEST CAR 0.5\n");
    copy_plate(ent_lpr_addr[entrance]->plate, ent_queue[entrance]->lplate);

	
	print_plate(ent_lpr_addr[entrance]->plate);
	printf("\n");
	/*do
	{
		if (queue->next == NULL)
		{
			// FOR TESTING TO GIVE SIGN FOR CAR TO READ
			// Enter car park and trigger lpr
			srand(time(NULL));
			int num = rand() % LEVELS;
			printf("TEST CAR1\n");
			//char *sign = (char *)(lpr->plate + 280);


			//*sign = num + 'A';
			//
			
			usleep(2);
			pthread_mutex_lock(&lpr->m);
			//for (;;){
			//
				printf("TEST CAR 2\n");
				strcpy(lpr->plate, queue->lplate);
				//pthread_cond_broadcast(&lpr->c);
				//pthread_cond_wait(&lpr->c, &lpr->m);
			//}
			pthread_mutex_unlock(&lpr->m);
			
			printf("TEST CAR 3\n");
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
	} while (queue->next != NULL);*/
	// wait for rand time as (100 - 10000ms)
	// call exit_carpark or all handled within this function
}


// void args 
void exit_carpark(struct cars * queue ,struct LPR *exit, struct level *lvl) {
    //update LPR as car exits
  usleep(randtime(100, 10000) * 1000); //Park for time between 100 - 10000ms
  usleep(10 * 1000); //Takes the car 10ms to drive to a random exit

  //Still need to update LPR
  free(queue); //<-- may be wrong

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
	//struct addr_num_args *numargs = (struct addr_num_args *)args;
	//struct level *lvl = numargs->addr;
	//int *temp = (int *)(numargs->addr);
	// no mutex lock required, only one thread at a time should be changing the temperature in simulation exe

	//*temp = numargs->num;
	//lvl->tempsensor = numargs->num;
	//free(args);
	int lvl = ((int *)args)[0];
	int temp = ((int *)args)[1];
	printf("TEST99 %d %d\n", lvl, temp);

	lvl_tmpalrm_addr[lvl]->tempsensor = temp;
	printf("TEST100\n");
}
void *openboomgate(void *arg)
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
	pthread_mutex_unlock(&bg->m);
	free(arg);
} 

void *closeboomgate(void *arg)
{
	printf("BOOM GATE TEST 0\n");
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
	free(arg);

} 

//expects args with LPR address and string
void *change_LPR(void *args){
	//struct addr_str_args *strargs = (struct addr_str_args *)args;
	//struct LPR *lvl = (struct level *)(strargs->addr);

	struct addr_str_args *argin = (struct addr_str_args *)args;
	LPR *lpradd;
	lpradd = (LPR *)argin->addr;
	char *newplate = (char *)argin->str;

	
	//pthread_mutex_unlock(&lpradd->m);
	pthread_mutex_lock(&lpradd->m);
	printf("Current License: ");
	print_plate(lpradd->plate);printf("\n");
	printf("MUTEX ADD2 %x\n", &lpradd->m);
	printf("MUTEX ADD3 %x\n", &lvl_lpr_addr[0]->m);
	
	printf("LICENSE TO CHANGE TO ");
	print_plate(newplate); printf("\n");
	printf("MUTEX LOCKED\n");
	
	//srcpy(lvl->lpr->plate, strargs->str);
	//srintf(->plate, "%s", strargs->str);

	copy_plate(lpradd->plate, newplate);
	usleep(100);
	printf("NEW LICENSE: ");
	print_plate(lvl_lpr_addr[0]->plate); printf("\n");

	pthread_mutex_unlock(&lpradd->m);
	printf("MUTEX UNLOCKED\n");
	//printf("%d", pthread_self());

	free(args);


}

//TESTING remove print functions to view one thread
void *change_LPR2(void *args){
	//struct addr_str_args *strargs = (struct addr_str_args *)args;
	//struct LPR *lvl = (struct level *)(strargs->addr);

	struct addr_str_args *argin = (struct addr_str_args *)args;
	LPR *lpradd;
	lpradd = (LPR *)argin->addr;
	char *newplate = (char *)argin->str;
	
	//pthread_mutex_unlock(&lpradd->m);
	pthread_mutex_lock(&lpradd->m);

	copy_plate(lpradd->plate, newplate);

	pthread_mutex_unlock(&lpradd->m);


	free(args);


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

void * set_firealarm(void * args){

	char lvl = ((char *)args)[0];
	char status = ((char *)args)[1];

	lvl_tmpalrm_addr[lvl]->falarm = status;
}

void cleanup(){

	// Destroy mutexes
	for (int i = 0; i < ENTRANCES; i++){
		pthread_mutex_destroy(&ent_lpr_addr[i]->m);
		pthread_mutex_destroy(&ent_boom_addr[i]->m);
		pthread_mutex_destroy(&ent_info_addr[i]->m);

		pthread_cond_destroy(&ent_lpr_addr[i]->c);
		pthread_cond_destroy(&ent_info_addr[i]->c);
		pthread_cond_destroy(&ent_boom_addr[i]->c);

	}

	for (int i = 0; i < EXITS; i++){
		pthread_mutex_destroy(&ext_lpr_addr[i]->m);
		pthread_mutex_destroy(&ext_boom_addr[i]->m);

		pthread_cond_destroy(&ext_lpr_addr[i]->c);
		pthread_cond_destroy(&ext_boom_addr[i]->c);
	}


	for (int i = 0; i < LEVELS; i++){
		pthread_mutex_destroy(&lvl_lpr_addr[i]->m);

		pthread_cond_destroy(&lvl_lpr_addr[i]->c);
	}


	// Unmap memory
	// unmap entrances
	for (int i = 1; i < ENTRANCES; i++){
		munmap(ent_lpr_addr[i] + ENT_GAP*i, sizeof(LPR)); 
		munmap(ent_boom_addr[i] + ENT_GAP*i, sizeof(boomgate)); 
		munmap(ent_info_addr[i] + ENT_GAP*i, sizeof(infosign)); 
	}

	// unmap exits
	for (int i = 0; i < EXITS; i++){
		munmap(ext_lpr_addr[i] + EXT_GAP*i, sizeof(LPR)); 
		munmap(ext_boom_addr[i] + EXT_GAP*i, sizeof(boomgate)); 	
	}

	// unmap levels
	for (int i = 0; i < LEVELS; i++){
		munmap(lvl_lpr_addr[i] + LVL_GAP*i, sizeof(LPR)); 
		munmap(lvl_tmpalrm_addr[i] + LVL_GAP*i, sizeof(temp_alarm));
	}

}

// setup parking levels, initial temp and sensor values
void init(){
	printf("TEST INIT 0\n");
	// Initialise Mutex locks and condition vars for entrance and exit mutexs;
	
	// MAP memory spaces
	// Initial memory space
	if ((ent_lpr_addr[0] = mmap(0, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 

	// MAP rest of entrances

	for(int i = 1; i < ENTRANCES; i++){
		if ((ent_lpr_addr[i] = (LPR *)mmap(ent_lpr_addr[0] + ENT_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
	}
	for (int i = 0; i < ENTRANCES; i++){
		if ((ent_boom_addr[i] = (boomgate *)mmap(ent_lpr_addr[0] +sizeof(LPR) + ENT_GAP*i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		if ((ent_info_addr[i] = (infosign *)mmap(ent_lpr_addr[0] +sizeof(LPR) + sizeof(boomgate) + ENT_GAP*i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
	}

	// map exits
	for (int i = 0; i < EXITS; i++){
		if ((ext_lpr_addr[i] = (LPR *)mmap(ent_lpr_addr[0]+ EXT_OFFSET + EXT_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		if ((ext_boom_addr[i] = (boomgate *)mmap(ent_lpr_addr[0] + EXT_OFFSET +sizeof(LPR) + EXT_GAP*i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 	
	}

	// map levels
	for (int i = 0; i < LEVELS; i++){
		if ((lvl_lpr_addr[i] = (LPR *)mmap(ent_lpr_addr[0]+ LVL_OFFSET + LVL_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		if ((lvl_tmpalrm_addr[i] = (temp_alarm *)mmap(ent_lpr_addr[0] + LVL_OFFSET + sizeof(LPR) + LVL_GAP*i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 	
	}

	// initialise mutexes
	
	// mutex shared attributes
	pthread_mutexattr_t mattr;
	if (pthread_mutexattr_init(&mattr)) perror("pthread mutex attribute");
	if (pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)) perror("pthread mutex attribute set pshared");

	pthread_condattr_t cattr;
	pthread_condattr_init(&cattr);
	pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

	// entrance mutexes
	for (int i = 0; i < ENTRANCES; i++){
		pthread_mutex_init(&ent_lpr_addr[i]->m, &mattr);
		//pthread_mutex_unlock(&ent_lpr_addr[i]->m);
		pthread_mutex_init(&ent_boom_addr[i]->m, &mattr);
		//pthread_mutex_unlock(&ent_boom_addr[i]->m);
		pthread_mutex_init(&ent_info_addr[i]->m, &mattr);
		//pthread_mutex_unlock(&ent_info_addr[i]->m);

		pthread_cond_init(&ent_lpr_addr[i]->c, &cattr);
		pthread_cond_init(&ent_boom_addr[i]->c, &cattr);
		pthread_cond_init(&ent_info_addr[i]->c, &cattr);

	}

	for (int i = 0; i < EXITS; i++){
		pthread_mutex_init(&ext_lpr_addr[i]->m, &mattr);
		//pthread_mutex_unlock(&ext_lpr_addr[i]->m);
		pthread_mutex_init(&ext_boom_addr[i]->m, &mattr);
		//pthread_mutex_unlock(&ext_boom_addr[i]->m);

		pthread_cond_init(&ext_lpr_addr[i]->c, &cattr);
		pthread_cond_init(&ext_boom_addr[i]->c, &cattr);
	}

	for (int i = 0; i < LEVELS; i++){
		pthread_mutex_init(&lvl_lpr_addr[i]->m, &mattr);
		//pthread_mutex_unlock(&lvl_lpr_addr[i]->m);

		pthread_cond_init(&lvl_lpr_addr[i]->c, &cattr);
	}


	// destroy attributes after initialising mutexes
	//pthread_mutexattr_destroy(&mattr);
	//pthread_condattr_destroy(&cattr);
	/*
	for (int i = 0; i < (3*ENTRANCES + 2*EXITS); i++){
		//int maddr = 96*i + 0;
		//pthread_mutex_t *m = (pthread_mutex_t *)(shm + maddr);	
		pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init((pthread_mutex_t*)(shm+maddr), &mattr);
	
		
		int caddr = 96*i + 40;
		pthread_cond_t *c = (pthread_cond_t *)(shm + caddr);
		pthread_condattr_t cattr;
		pthread_condattr_init(&cattr);
		pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
		pthread_cond_init(c, &cattr);

		printf("TEST INIT 1 Part %d\n", i);
		//pthread_mutexattr_setpshared(m, );
		//pthread_condattr_setpshared(c, PTHREAD_PROCESS_SHARED);

	}

	for (int i = 0; i < LEVELS; i++)
	{
		int maddr = 104*i + 2400;
		//pthread_mutex_t *m = (pthread_mutex_t *)(shm + maddr);
		pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init((pthread_mutex_t*)(shm+maddr), &mattr);

		int caddr = 104*i + 2440;
		pthread_cond_t *c = (pthread_cond_t *)(shm + caddr);
		pthread_condattr_t cattr;
		pthread_condattr_init(&cattr);
		pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
		pthread_cond_init(c, &cattr);

		printf("TEST INIT 2 Part %d\n", i);
		//pthread_mutexattr_setpshared(m, PTHREAD_PROCESS_SHARED);
		//pthread_condattr_setpshared(c, PTHREAD_PROCESS_SHARED);

	}*/

	// set all temps to initial 25C

	pthread_t tempsetthreads[LEVELS];
	for (int i = 0; i < LEVELS; i++) {
		//volatile struct addr_num_args *args = malloc(sizeof(struct addr_num_args));
		//args->addr = shm + 104 * i + 2496; //temp sensors start at 2496, spaced by 104 bytes
		//args->num = 25;

		// send temp setting as array of 2 ints
		int args[2];
		args[0] = i; //lvl
		args[1] = 25; //temp
		pthread_create(&tempsetthreads[i], NULL, change_temp, &args);
		printf("TEST INIT 3 Part %d\n", i);
	}
	
	// set fire alarms on all levels to 0
	pthread_t falarmsetthreads[LEVELS];
	for (int i = 0; i < LEVELS; i++) {
		//volatile struct addr_num_args *args = malloc(sizeof(struct addr_num_args));
		//args->addr = shm + 104 * i + 2498; //fire alarms start at 2498, spaced by 104 bytes
		//args->num = 0;
		int args[2];
		//pthread_mutex_lock(&lvl_lpr_addr[i]->m);
		args[0] = 0; // status
		args[1] = i; // lvl
		//pthread_mutex_unlock(&lvl_lpr_addr[i]->m);
		pthread_create(&falarmsetthreads[i], NULL, set_firealarm, args);

		printf("TEST INIT 4 Part %d\n", i);
	}
	
	// set levels LPRs
	pthread_t LPRLevelsetthreads[LEVELS];
	for (int i = 0; i < LEVELS; i++) {
		struct addr_str_args *args = (struct addr_str_args *)malloc(sizeof(struct addr_str_args));//volatile 
		//TESTING ON ONE PIECE OF MEMORY
		args->addr = lvl_lpr_addr[i];
		//args->addr = lvl_lpr_addr[i];
		//printf("MEM ADDR 1 %x\n", args->addr);
		printf("TEST INIT 5.0 Part %d\n", i);
		args->str = malloc(sizeof(char)*6);
		sprintf(args->str, "--%d---", i);
		//copy_plate(args->str, i);
		copy_plate(lvl_lpr_addr[i]->plate, "123456");
		printf("SETTING CURRENT LICENSE ");
		print_plate(lvl_lpr_addr[i]->plate);printf("\n");
		//strcpy(args->str, "------\0");
		printf("MUTEX ADD1 %x\n", lvl_lpr_addr[i]->m);
		if (pthread_create(&LPRLevelsetthreads[i], NULL, change_LPR2, args) != 0){
			perror("pthread error");
		}
		
		printf("TEST INIT 5 Part %d\n", i);
	}/*
	for (int i = 1; i < LEVELS; i++) {
		struct addr_str_args *args = (struct addr_str_args *)malloc(sizeof(struct addr_str_args));//volatile 
		//TESTING ON ONE PIECE OF MEMORY
		args->addr = lvl_lpr_addr[0];
		//args->addr = lvl_lpr_addr[i];
		//printf("MEM ADDR 1 %x\n", args->addr);
		printf("TEST INIT 5.0 Part %d\n", i);
		args->str = malloc(sizeof(char)*6);
		sprintf(args->str, "--%d---", i);
		//copy_plate(args->str, i);
		copy_plate(lvl_lpr_addr[i]->plate, "123456");
		printf("SETTING CURRENT LICENSE ");
		print_plate(lvl_lpr_addr[i]->plate);printf("\n");
		//strcpy(args->str, "------\0");
		printf("MUTEX ADD1 %x\n", lvl_lpr_addr[i]->m);
		if (pthread_create(&LPRLevelsetthreads[i], NULL, change_LPR2, (void *)args) != 0){
			perror("pthread error");
		}
		
		printf("TEST INIT 5 Part %d\n", i);
	}*/
	/*
	pthread_t LPRExitsetthreads[EXITS];
	for (int i = 0; i < EXITS; i++) {
		volatile struct addr_str_args *args = malloc(sizeof(struct addr_str_args));
		args->addr = lvl_lpr_addr[i];
		//printf("MEM ADDR 1 %x\n", args->addr);
		printf("TEST INIT 5.0 Part %d\n", i);
		args->str = malloc(sizeof(char)*6);
		copy_plate(args->str, "------");
		//strcpy(args->str, "------\0");
		pthread_create(&LPRExitsetthreads[i], NULL, change_LPR, args);
		printf("TEST INIT 5 Part %d\n", i);
	}
	pthread_t LPREntersetthreads[ENTRANCES];
	for (int i = 0; i < ENTRANCES; i++) {
		volatile struct addr_str_args *args = malloc(sizeof(struct addr_str_args));
		args->addr = lvl_lpr_addr[i];
		//printf("MEM ADDR 1 %x\n", args->addr);
		printf("TEST INIT 5.0 Part %d\n", i);
		args->str = malloc(sizeof(char)*6);
		copy_plate(args->str, "------");
		//strcpy(args->str, "------\0");
		pthread_create(&LPREntersetthreads[i], NULL, change_LPR, args);
		printf("TEST INIT 5 Part %d\n", i);
	}
	
	pthread_t LPREntExsetthreads[ENTRANCES + EXITS];
	for (int i = 0; i < ENTRANCES + EXITS; i++) {
		volatile struct addr_str_args *args = malloc(sizeof(struct addr_str_args));
		args->addr = shm + 288 * i + 0;
		args->str = malloc(sizeof(char)*7);
		strcpy(args->str, "------\0");
		//volatile struct boomgate *bg = shm + *args->addraddr;
		pthread_create(&LPREntExsetthreads[i], NULL, change_LPR, args);
		printf("TEST INIT 6 Part %d\n", i);

	}

	// close all boomgates
	pthread_t boomgatethreads[ENTRANCES + EXITS]; //= malloc(sizeof(pthread_t) * (ENTRANCES + EXITS));
	for (int i = 0; i < ENTRANCES; i++) {
		int addr = 288 * i + 96;
		volatile struct boomgate *bg = (shm + addr); //(struct boomgate *)
		pthread_create(&boomgatethreads[i], NULL, closeboomgate, bg);
		printf("TEST INIT 7 Part %d\n", i);
	}

	for (int i = 0; i < EXITS; i++) {
		int addr = 192 * i + 1536;
		volatile struct boomgate *bg = shm + addr;
		pthread_create(&boomgatethreads[ENTRANCES + i], NULL, closeboomgate, bg);
		printf("TEST INIT 8 Part %d\n", i);
	}
	*/
	for (int i = 0; i < LEVELS; i++) {
		printf("TEST INIT 9 Part %d\n", i);
		pthread_join(tempsetthreads[i], NULL);
	}
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(falarmsetthreads[i], NULL);
	}
	
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(LPRLevelsetthreads[i], NULL);
	}
	/*
	for (int i = 0; i < ENTRANCES + EXITS; i++) {
		pthread_join(LPRExitsetthreads[i], NULL);
	}
	for (int i = 0; i < ENTRANCES + EXITS; i++) {
		pthread_join(LPREntersetthreads[i], NULL);
	}
	
	for (int i = 0; i < ENTRANCES; i++) {
		pthread_join(&boomgatethreads[i], NULL);
	}
	for (int i = 0; i < EXITS; i++) {
		pthread_join(&boomgatethreads[ENTRANCES + i], NULL);
	}
	*/
}


int main(int argc, int * argv){

	printf("TEST\n");

	read_allowed_plates_from_file();

    if ((shm_fd = shm_open("PARKING", O_CREAT | O_RDWR, 0666)) < 0) perror("shm_open");

	ftruncate(shm_fd, SHMSZ);
	//if ((shm = mmap(0, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); //(volatile void *)

	init();


	// MANUAL MUTEX INIT, TAKE OUT AFTER FIXING INIT()
	/*
	for (int i = 0; i < (3*ENTRANCES + 2*EXITS); i++){
		int maddr = 96*i + 0;
		//pthread_mutex_t *m = (pthread_mutex_t *)(shm + maddr);	
		pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init((pthread_mutex_t*)(shm+maddr), &mattr);
	
		
		int caddr = 96*i + 40;
		pthread_cond_t *c = (pthread_cond_t *)(shm + caddr);
		pthread_condattr_t cattr;
		pthread_condattr_init(&cattr);
		pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
		pthread_cond_init(c, &cattr);

		//printf("TEST INIT 1 Part %d\n", i);
		//pthread_mutexattr_setpshared(m, );
		//pthread_condattr_setpshared(c, PTHREAD_PROCESS_SHARED);

	}*/

	printf("TEST1.5\n");
	//init();
	printf("TEST 2\n");

	// Initialise srand()
	srand(time(NULL));


	// TESTING SECTION
	// first value in linked list will be a initial value
	/*
	car_node firstcar;
	car_node *head;
	strcpy(firstcar.lplate, carlist[0]);
	firstcar.next = NULL;
	head = &firstcar;
	*/
	ent_queue[0] = (car_node *)malloc(sizeof(car_node));
	*ent_queue[0]->lplate = (char *)malloc(sizeof(char)*6);
	copy_plate(ent_queue[0]->lplate, allowed_cars[0]);
	ent_queue[0]->next = NULL;
	
	printf("LICENSE STORE 1 ");
	print_plate(allowed_cars[0]);
	printf("\n");
	pthread_t enterparkthread = malloc(sizeof(pthread_t));
	
	/*
	struct two_addr *args = (struct two_addr *)malloc(sizeof(struct two_addr));
	args->lpraddr = ent_lpr_addr + 0;
	args->queueaddr = n;
	*/
	int *arg = (int *)malloc(sizeof(int));
	*arg = 0;
	//car_node * carptr = (car_node *)args->queueaddr;
	printf("TEST 2.5\n");

	//printf("%s\n", carptr->lplate);
	printf("TEST 3\n");
	//pthread_create(enterparkthread, NULL, enter_carpark, &args);
	enter_carpark(arg);

	//strcpy((char *)(shm + 88), "000000");
	//sprintf((shm+88), "%s", "000000");
	printf("TEST4\n");
	//pthread_join(enterparkthread, NULL);

	printf("TEST 4.5\n");
	//char *ptr = (char *)(shm + 88);
	//printf("TEST 5\n");

	//printf("%x\n", shm);
	//char *testplate = (char *)(shm + 88);
	//printf("License plate %s\n", *ptr);

	//generate_car(firstcar);
	
	/* check car
	for(int i = 0; i < NUM_ALLOW_CARS; i++){
		printf("%s\n", allowed_cars[i]);
	}
	if(checklicense_forcargen("510SLS")){
		printf("Found car!\n");
	}*/


	// TESTING SECTION
	//munmap((void *)shm, 2920);
	cleanup();
	close(shm_fd);
	return 0;
}
