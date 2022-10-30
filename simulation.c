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
	pthread_mutex_t m;
	pthread_cond_t c;
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
pthread_mutex_t ent_qm[ENTRANCES] = {PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t ent_qc[ENTRANCES] = {PTHREAD_COND_INITIALIZER};

car_node *cars_parked[LEVELS];
pthread_mutex_t ent_cpm[LEVELS] = {PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t ent_cpc[LEVELS] = {PTHREAD_COND_INITIALIZER};

car_node *ext_queue[EXITS];
pthread_mutex_t ext_qm[EXITS] = {PTHREAD_MUTEX_INITIALIZER};
pthread_cond_t ext_qc[EXITS] = {PTHREAD_COND_INITIALIZER};


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
		if (cmp_plate(plate, allowed_cars[i]) == 0){
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
    copy_plate(new->lplate,car);
    new->next = head;
    return new;
}

// generate car, allocating license plate, 50% chance of being on allowed list
// add to linked list of car queue at one of the entrances
// returns the head of the linked list

// gets passed the entrance number
void * generate_car(void * args) {
	for(;;){ //for

	//printf("CAR GEN TEST 0\n");
	
	int entrance = ((int *)args);
	//printf("Entrance %d\n", entrance);
	// wait random time before generating car
	usleep(randtime(1, 100) * 10000); //TODO
	// Multiple threads for each entrance, handles each queue
	//bool tf = (rand() % 2) != 0;
	bool tf = true;
	
	car_node * gen;
	//printf("CAR GEN TEST 1\n");
	//pthread_mutex_unlock(&ent_queue[entrance]->m);
	pthread_mutex_lock(&ent_qm[entrance]);
	//rintf("CAR GEN TEST 2\n");
	if (tf) {
		//printf("from list\n");

		//printf("CAR GEN TEST 3\n");
		ent_queue[entrance] = addCar(ent_queue[entrance], allowed_cars[rand() % 100]);


	} else {

		char randcar[6];
		for (int i = 0; i < 3; i++) {
			int num = (rand() % (57 - 48 + 1)) + 48;
			randcar[i] = (char)num;
		}
		for (int i = 3; i < 6; i++) {
			int num = (rand() % (90 - 65 + 1)) + 65;
			randcar[i] = num;
		}

		ent_queue[entrance] = addCar(ent_queue[entrance], randcar);
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
	pthread_cond_signal(&ent_qc[entrance]);
	pthread_mutex_unlock(&ent_qm[entrance]);
	}

}

car_node * delete_last(car_node * car){
	car_node *ptr = car;
	if(ptr == NULL){
		return NULL;
	}
	else if(ptr->next == NULL){
		ptr = NULL;
		return NULL;
	}
	else{
		while(ptr->next != NULL){}
		ptr=NULL;
		return car;
	}
}

// void args 
void handle_carpark(void *args) {

	char *argsin = (char *)args;
	char plate[6];
	copy_plate(plate, argsin);

	int lvl = *(argsin+7);
    
	usleep(randtime(100, 10000) * 1000); //Park for time between 100 - 10000ms
	usleep(10 * 1000); //Takes the car 10ms to drive to a random exit

	pthread_mutex_lock(&ent_cpm[lvl]);

	int exit = rand() % (EXITS);
	car_node *ptr;
	car_node *prev;
	ptr = cars_parked[lvl];
	prev = ptr;
	if(ptr == NULL){return;}
	ptr = ptr->next;
	if(ptr == NULL){
		printf("EXIT TEST\n");

		addCar(ext_queue[exit], prev->lplate);
		cars_parked[lvl] = NULL;
		return;
		//prev = NULL;
	}
	
	while(cmp_plate(ptr->lplate, plate) !=0){
							

		ptr = ptr->next;

		if(ptr == NULL){
			prev=NULL;
		}
		else{
			prev = prev->next;
		}
	}
	prev->next = ptr->next;
	addCar(ext_queue[exit], ptr->lplate);
	pthread_mutex_signal(&ext_qc[exit]);
	pthread_mutex_unlock(&ent_cpm[lvl]);
	free(ptr);
	
  //Still need to update LPR
  //free(queue); //<-- may be wrong

}



void *exit_carpark(void * arg) {	
	for(;;){
	/*struct two_addr * lpr_queue = (struct two_addr *) arg;
	struct LPR *lpr = (struct LPR *)lpr_queue->lpraddr;
	struct cars *queue = (struct cars*)lpr_queue->queueaddr;

	sprintf((shm + 88), "%s", queue->lplate);*/
	printf("TEST CAR 0\n");

    int exit = ((int *)arg);
	// find the first car that joined the queue
	//pthread_mutex_unlock(&ent_queue[entrance]->m);
	pthread_mutex_lock(&ext_qm[exit]);
	printf("TEST CAR 1\n");
	while(ext_queue[exit] == NULL){pthread_cond_wait(&ext_qc[exit], &ext_qm[exit]);} // wait for car in queue (signal from car gen)

	car_node * firstcar;
	firstcar = ext_queue[exit];
	while(firstcar->next != NULL){
		firstcar = firstcar->next;
	}
	pthread_mutex_unlock(&ext_qm[exit]); // should be free to unlock after finding first car in queue

	// car reads
	//pthread_mutex_unlock(&ent_lpr_addr[entrance]->m);
	usleep(10*1000); // wait 2ms before triggering LPR

	//pthread_mutex_unlock(&ent_lpr_addr[entrance]->m);
	pthread_mutex_lock(&ext_lpr_addr[exit]->m);

    copy_plate(ext_lpr_addr[exit]->plate, ext_queue[exit]->lplate);
	//print_plate(ext_lpr_addr[exit]->plate);
	pthread_mutex_unlock(&ext_lpr_addr[exit]->m);

	//openboomgate and random sign for testing for testing (should be done by management system)
	int lvl = rand() % (LEVELS + 1);




	//Testing
	pthread_t boom_op = malloc(sizeof(pthread_t));
	pthread_create(boom_op, NULL, openboomgate, ent_boom_addr[exit]);
	//testing

	
	while(ent_boom_addr[exit]->s != 'O'){
		printf("Waiting on gate.. \n");
	}
	printf("GO!\n");

		
	pthread_mutex_lock(&ext_qm[exit]);
	ext_queue[exit] = delete_last(ext_queue);
	/*
	car_node *del = ext_queue[exit];
	if(del->next == NULL){ext_queue[exit] = NULL;}
	else{
		while(del->next->next != NULL);
		del->next = NULL;
	}*/
	pthread_mutex_unlock(&ext_qm[exit]);
	/*
	car_node * prevcar = ext_queue[exit];
	
	if(prevcar == firstcar){
		ext_queue[exit] = NULL;
	}
	else{
		while(prevcar->next != NULL){
			prevcar = prevcar->next;
		}
	}
	prevcar->next = NULL;*/
	//free(firstcar);

	}
}

// enter carpark, updates LPR of place where car moves to
// May need to reduce to void * arg
// struct cars * queue ,struct LPR *entrance, struct parkinglot plot, 
void *enter_carpark(void * arg) {	
	for(;;){
	/*struct two_addr * lpr_queue = (struct two_addr *) arg;
	struct LPR *lpr = (struct LPR *)lpr_queue->lpraddr;
	struct cars *queue = (struct cars*)lpr_queue->queueaddr;

	sprintf((shm + 88), "%s", queue->lplate);*/
	printf("TEST CAR 0\n");

    int entrance = ((int *)arg);
	// find the first car that joined the queue
	//pthread_mutex_unlock(&ent_queue[entrance]->m);
	pthread_mutex_lock(&ent_qm[entrance]);
	printf("TEST CAR 1\n");
	while(ent_queue[entrance] == NULL){pthread_cond_wait(&ent_qc[entrance], &ent_qm[entrance]);} // wait for car in queue (signal from car gen)

	car_node * firstcar;
	firstcar = ent_queue[entrance];
	while(firstcar->next != NULL){
		firstcar = firstcar->next;
	}
	pthread_mutex_unlock(&ent_qm[entrance]); // should be free to unlock after finding first car in queue

	printf("Entrance %d\n", entrance);
	print_plate(firstcar->lplate);

	printf("TEST CAR 0.5\n");

	// car reads
	//pthread_mutex_unlock(&ent_lpr_addr[entrance]->m);
	usleep(2*1000); // wait 2ms before triggering LPR

	//pthread_mutex_unlock(&ent_lpr_addr[entrance]->m);
	pthread_mutex_lock(&ent_lpr_addr[entrance]->m);

    copy_plate(ent_lpr_addr[entrance]->plate, ent_queue[entrance]->lplate);
	print_plate(ent_lpr_addr[entrance]->plate);
	pthread_mutex_unlock(&ent_lpr_addr[entrance]->m);

	//openboomgate and random sign for testing for testing (should be done by management system)
	int lvl = rand() % (LEVELS + 1);

	pthread_mutex_unlock(&ent_info_addr[entrance]->m);
	pthread_mutex_lock(&ent_info_addr[entrance]->m);
	if (lvl == LEVELS + 1){
		ent_info_addr[entrance]->display = 'X';
	}
	else{
		ent_info_addr[entrance]->display = lvl + 'A';
	}

	pthread_mutex_unlock((&ent_info_addr[entrance]->m));

	if(ent_info_addr[entrance]->display == 'X'){
		pthread_mutex_lock(&ent_qm[entrance]);
		car_node *del = ent_queue[entrance];
		if(del->next == NULL){ent_queue[entrance] = NULL;}
		else{
			while(del->next->next != NULL);
			del->next = NULL;
		}
		continue;
	}

	//Testing
	pthread_t boom_op = malloc(sizeof(pthread_t));
	pthread_create(boom_op, NULL, openboomgate, ent_boom_addr[entrance]);
	//testing

	
	while(ent_boom_addr[entrance]->s != 'O'){
		printf("Waiting on gate.. \n");
	}
	printf("GO!\n");


	pthread_mutex_unlock(&ent_info_addr[entrance]->m);
	pthread_mutex_lock(&ent_info_addr[entrance]->m);

	lvl = ent_info_addr[entrance]->display;

	int followthesign = rand() % 100;
	if (followthesign <= 20){//follows sign approx 80% of time
		lvl = rand() % (LEVELS); // choose random level
	}

	
	//add to cars parked queue on defined lvl
	usleep(10*1000); //10ms to go to level
	pthread_mutex_lock(&ent_cpm[lvl]);
	cars_parked[lvl] = addCar(cars_parked[lvl], firstcar->lplate);
	pthread_mutex_unlock(&ent_cpm[lvl]);

	pthread_t car_exit_thread = (pthread_t *)malloc(sizeof(pthread_t));
	char exitargs[7];
	copy_plate(exitargs, firstcar->lplate);
	exitargs[7] = lvl;
	pthread_create(car_exit_thread, NULL, handle_carpark, exitargs);

	printf("EXIT TEST\n");
	//remove car from entrance queue
		
	/*car_node * prevcar = ent_queue[entrance];
	
	if(prevcar == firstcar){
		ent_queue[entrance] = NULL;
	}
	else{
		while(prevcar->next != NULL){
			prevcar = prevcar->next;
		}
	}
	prevcar->next = NULL;
	free(firstcar);*/
	pthread_mutex_lock(&ent_qm[entrance]);
	ent_queue[entrance] = delete_last(ent_queue);
	pthread_mutex_unlock(&ent_qm[entrance]);

	pthread_mutex_unlock(&ent_info_addr[entrance]->m);

	printf("TEST CAR 2\n");
	printf("\n");
	
	}
}

// get random time
int randtime(int min, int max) {
  return rand()%(max-min);
}

int parktime() {
  int value = randtime(100, 10000);
  usleep(value * 1000); //Sleep between 100ms and 10000ms
  return value; //return how long car was parked for billing purposes??
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
	//pthread_mutex_lock(&lvl_lpr_addr[0]->m);
	int lvl = ((int *)args)[0];
	int temp = ((int *)args)[1];
	printf("TEST99 %d %d\n", lvl, temp);

	lvl_tmpalrm_addr[lvl]->tempsensor = temp;
	printf("TEST100\n");
	//pthread_mutex_unlock(&lvl_lpr_addr[0]->m);
}
void *openboomgate(void *arg)
{
    struct boomgate *bg = arg;
    pthread_mutex_lock(&bg->m);
	printf("MUTEX LOCKED, going to try raise gate\n");
	printf("GATE STAT %c\n", bg->s);
    for (;;) {
		if(bg->s == 'C'){
			bg->s = 'R';
		}
		else if(bg->s == 'O'){
			break;
		}

      	if (bg->s == 'R') {
        	usleep(10 * 1000); //simulate 10ms of waiting for gate to open
        	bg->s = 'O';
        	pthread_cond_broadcast(&bg->c);
		break;
      	}
      	pthread_cond_wait(&bg->c, &bg->m);
    }
	pthread_mutex_unlock(&bg->m);
} 

void *closeboomgate(void *arg)
{
    struct boomgate *bg = arg;
    pthread_mutex_lock(&bg->m);
	printf("MUTEX LOCKED, going to try lower gate\n");
	printf("GATE STAT %c\n", bg->s);
    for (;;) {
		if(bg->s == 'O'){
			bg->s = 'L';
		}
		else if(bg->s == 'C'){
			break;
		}
		
      	if (bg->s == 'L') {
        	usleep(10 * 1000); //simulate 10ms of waiting for gate to open
        	bg->s = 'C';
        	pthread_cond_broadcast(&bg->c);
		break;
      	}
      	pthread_cond_wait(&bg->c, &bg->m);
    }
	pthread_mutex_unlock(&bg->m);
} 

//expects args with LPR address and string
void *change_LPR(void *args){
	//struct addr_str_args *strargs = (struct addr_str_args *)args;
	//struct LPR *lvl = (struct level *)(strargs->addr);

	struct addr_str_args *argin = (struct addr_str_args *)args;
	LPR *lpradd;
	lpradd = (LPR *)argin->addr;
	char *newplate = (char *)argin->str;

	//printf("MUTEX ADD2 %x\n", &lpradd->m);
	//printf("MUTEX ADD3 %x\n", &lvl_lpr_addr[0]->m);
	
	//pthread_mutex_unlock(&lpradd->m);
	//printf("UNLOCK ERROR %d\n", pthread_mutex_unlock(&lpradd->m));

	if(pthread_mutex_lock(&lpradd->m) != 0) perror("mutex lock");

	//printf("MUTEX LOCKED\n");
	//printf("Current License: ");
	print_plate(lpradd->plate);printf("\n");
	
	//printf("LICENSE TO CHANGE TO ");
	print_plate(newplate); printf("\n");
	
	
	//srcpy(lvl->lpr->plate, strargs->str);
	//srintf(->plate, "%s", strargs->str);

	copy_plate(lpradd->plate, newplate);
	//printf("NEW LICENSE: ");
	print_plate(lvl_lpr_addr[0]->plate); printf("\n");

	pthread_mutex_unlock(&lpradd->m);
	//pthread_cond_broadcast(&lpradd->c);
	//printf("MUTEX UNLOCKED\n");
	//printf("%d", pthread_self());

	//free(argin->str);
	//printf("FINISHED SETTING LPR\n");

}

// trigger when simulating fire, aggressively change temperature on one level
void *simulate_fire(void *lvl_addr) {

}

void *simulate_temp(){
    int delay = 0.002;
    int Temp;
    usleep(randtime(1,5)*1000);
	int maxTemp = 25;
	int baseTemp = 20;
  
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
	printf("FIREALARM SET\n");
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
	srand(time(NULL));
	printf("TEST INIT 0\n");

	// Initialise local mutexes
	for(int i = 0; i < ENTRANCES; i++){
		printf("TEST AGAIN\n");
		ent_queue[i] = malloc(sizeof(car_node));
		pthread_mutex_init(&ent_queue[i]->m, NULL);
		pthread_cond_init(&ent_queue[i]->c, NULL);	
	}
	for(int i = 0; i < LEVELS; i++){
		cars_parked[i] = malloc(sizeof(car_node));
		pthread_mutex_init(&cars_parked[i]->m, NULL);
		pthread_cond_init(&cars_parked[i]->c, NULL);
	}
	for(int i = 0; i < ENTRANCES; i++){
		printf("TEST AGAIN\n");
		ext_queue[i] = malloc(sizeof(car_node));
		//ext_queue[i] = NULL;
		pthread_mutex_init(&ext_queue[i]->m, NULL);
		pthread_cond_init(&ext_queue[i]->c, NULL);
	}	


	// MAP memory spaces
	// Initial memory space
	if ((shm = mmap(0, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap");

	//if ((ent_lpr_addr[0] = mmap(0, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
	printf("ENT MEM: %d\n", ENT_GAP*0);

	// MAP rest of entrances

	for(int i = 0; i < ENTRANCES; i++){ //ent_lpr_addr[0] + 
		//if ((ent_lpr_addr[i] = (LPR *)mmap(ENT_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap");
		ent_lpr_addr[i] = (LPR *)(shm + ENT_GAP*i);
	}
	for (int i = 0; i < ENTRANCES; i++){
		//if ((ent_boom_addr[i] = (boomgate *)mmap(sizeof(LPR) + ENT_GAP*i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		//if ((ent_info_addr[i] = (infosign *)mmap(sizeof(LPR) + sizeof(boomgate) + ENT_GAP*i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 

		ent_boom_addr[i] = (boomgate *)(shm + sizeof(LPR) + ENT_GAP*i);
		ent_info_addr[i] = (infosign *)(shm + sizeof(LPR) + sizeof(boomgate) + ENT_GAP*i);
	}

	// map exits
	for (int i = 0; i < EXITS; i++){
		//if ((ext_lpr_addr[i] = (LPR *)mmap(EXT_OFFSET + EXT_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		//if ((ext_boom_addr[i] = (boomgate *)mmap( EXT_OFFSET +sizeof(LPR) + EXT_GAP*i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap");
		ext_lpr_addr[i] = (LPR *)(shm + EXT_OFFSET + EXT_GAP*i);
		ext_boom_addr[i] = (boomgate *)(shm + EXT_OFFSET +sizeof(LPR) + EXT_GAP*i);
	}

	// map levels
	for (int i = 0; i < LEVELS; i++){
		//if ((lvl_lpr_addr[i] = (LPR *)mmap( LVL_OFFSET + LVL_GAP*i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 
		//if ((lvl_tmpalrm_addr[i] = (temp_alarm *)mmap( LVL_OFFSET + sizeof(LPR) + LVL_GAP*i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); 	
		lvl_lpr_addr[i] = (LPR *)(shm + LVL_OFFSET + LVL_GAP*i);
		lvl_tmpalrm_addr[i] = (temp_alarm*)(shm + LVL_OFFSET + sizeof(LPR) + LVL_GAP*i);
		
		printf("ENT LPR MEM: %d\n", &ent_lpr_addr[i]);
		//printf("LPR SIZE %d\n", sizeof(LPR));
		printf("ENT BOOM MEM %d: %d\n", i, &ent_boom_addr[i]);
		printf("ENT INFO MEM %d: %d\n", i, &ent_info_addr[i]);
		//printf("EXT LPR MEM: %d\n", &ent_lpr_addr[i]);
		//printf("EXT BOOM MEM: %d\n", &ent_lpr_addr[i]);
	}

	// Initialise Mutex locks and condition vars for entrance and exit mutexs;
	
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
	pthread_mutexattr_destroy(&mattr);
	pthread_condattr_destroy(&cattr);
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
	
	// set entrance LPRs
	pthread_t LPREntsetthreads[LEVELS];
	struct addr_str_args *argsent[LEVELS];
	for (int i = 0; i < LEVELS; i++) {
		//volatile 
		argsent[i] = (struct addr_str_args*)malloc(sizeof(struct addr_str_args));
		//TESTING ON ONE PIECE OF MEMORY
		//args->addr = (void *)&lvl_lpr_addr[0];
		//printf("TEST INIT 5.0 Part %d\n", i);
		//printf("SHARED MEM SIZE: %d\n", SHMSZ);
		argsent[i]->addr = ent_lpr_addr[i];

		argsent[i]->str = malloc(sizeof(char)*6);
		//sprintf(args[i]->str, "--%d---", i);
		copy_plate(argsent[i]->str, "------");
		printf("SETTING CURRENT LICENSE ");
		print_plate(argsent[i]->str);printf("\n");
		//printf("MUTEX ADD1 %x\n", lvl_lpr_addr[i]->m);

		if (pthread_create(&LPREntsetthreads[i], NULL, change_LPR, argsent[i]) != 0){
			perror("pthread error");
		}
		
		printf("TEST INIT 5 Part %d\n", i);
	}

	pthread_t LPRExtsetthreads[EXITS];
	struct addr_str_args *argsext[EXITS];
	for (int i = 0; i < EXITS; i++) {
		//volatile 
		argsext[i] = (struct addr_str_args*)malloc(sizeof(struct addr_str_args));
		//TESTING ON ONE PIECE OF MEMORY
		//args->addr = (void *)&lvl_lpr_addr[0];
		//printf("TEST INIT 5.0 Part %d\n", i);
		//printf("SHARED MEM SIZE: %d\n", SHMSZ);
		argsext[i]->addr = ent_lpr_addr[i];

		argsext[i]->str = malloc(sizeof(char)*6);
		//sprintf(args[i]->str, "--%d---", i);
		copy_plate(argsext[i]->str, "------");
		printf("SETTING CURRENT LICENSE ");
		print_plate(argsext[i]->str);printf("\n");
		//printf("MUTEX ADD1 %x\n", lvl_lpr_addr[i]->m);

		if (pthread_create(&LPRExtsetthreads[i], NULL, change_LPR, argsent[i]) != 0){
			perror("pthread error");
		}
		
		printf("TEST INIT 5 Part %d\n", i);
	}
	// set levels LPRs 
	
	pthread_t LPRLevelsetthreads[LEVELS];
	struct addr_str_args *argslvl[LEVELS];
	for (int i = 0; i < LEVELS; i++) {
		//volatile 
		argslvl[i] = (struct addr_str_args*)malloc(sizeof(struct addr_str_args));
		//TESTING ON ONE PIECE OF MEMORY
		//args->addr = (void *)&lvl_lpr_addr[0];
		//printf("TEST INIT 5.0 Part %d\n", i);
		//printf("SHARED MEM SIZE: %d\n", SHMSZ);
		argslvl[i]->addr = lvl_lpr_addr[i];

		argslvl[i]->str = malloc(sizeof(char)*6);
		//sprintf(args[i]->str, "--%d---", i);
		copy_plate(argslvl[i]->str, "------");
		printf("SETTING CURRENT LICENSE ");
		print_plate(argslvl[i]->str);printf("\n");
		//printf("MUTEX ADD1 %x\n", lvl_lpr_addr[i]->m);

		if (pthread_create(&LPRLevelsetthreads[i], NULL, change_LPR, argslvl[i]) != 0){
			perror("pthread error");
		}
		
		printf("TEST INIT 5 Part %d\n", i);
	}

	// close all boomgates

	//pthread_t entboomgatethreads[ENTRANCES]; //= malloc(sizeof(pthread_t) * (ENTRANCES + EXITS));
	struct boomgate *bgents[ENTRANCES];
	for (int i = 0; i < ENTRANCES; i++) {
		//int addr = 288 * i + 96;
		bgents[i] =  &ent_boom_addr[i]; //(struct boomgate *) volatile 
		//pthread_create(&entboomgatethreads[i], NULL, closeboomgate, bgents[i]);
		ent_boom_addr[i]->s = 'C';
		printf("TEST INIT 7 Part %d\n", i);

		
	}
	printf("BOOM STAT: %c\n", ent_boom_addr[3]->s);
	
	//pthread_t extboomgatethreads[EXITS];
	struct boomgate *bgexts[EXITS];
	for (int i = 0; i < EXITS; i++) {
		//int addr = ext_boom_addr[i];
		bgexts[i] = &ext_boom_addr[i];
		//pthread_create(&extboomgatethreads[i], NULL, closeboomgate, bgexts[i]);
		ext_boom_addr[i]->s = 'C';
		printf("TEST INIT 8 Part %d\n", i);
	}

	// set status signs to '-' initially
	for(int i = 0; i < ENTRANCES; i++){
		ent_info_addr[i]->display = '-';
	}

	//joining threads
	
	for (int i = 0; i < LEVELS; i++) {
		printf("TEST INIT 9 Part %d\n", i);
		pthread_join(tempsetthreads[i], NULL);
	}
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(falarmsetthreads[i], NULL);
	}
	
	
	for (int i = 0; i < LEVELS; i++) {
		pthread_join(LPRLevelsetthreads[i], NULL);
		free(argslvl[i]->str);
	}
	for (int i = 0; i < ENTRANCES; i++) {
		pthread_join(LPREntsetthreads[i], NULL);
		free(argsent[i]->str);

	}
	for (int i = 0; i < EXITS; i++) {
		pthread_join(LPRExtsetthreads[i], NULL);
		free(argsext[i]->str);

	}
	
	/*
	for (int i = 0; i < ENTRANCES; i++) {
		pthread_join(entboomgatethreads[i], NULL);
	}
	for (int i = 0; i < EXITS; i++) {
		pthread_join(extboomgatethreads[i], NULL);
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

	read_allowed_plates_from_file();

    if ((shm_fd = shm_open("PARKING", O_CREAT | O_RDWR, 0666)) < 0) perror("shm_open");

	ftruncate(shm_fd, SHMSZ);
	//if ((shm = mmap(0, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap"); //(volatile void *)

	init();

	printf("TEST1.5\n");
	//init();
	printf("TEST 2\n");

	// Initialise srand()


	// TESTING SECTION
	// first value in linked list will be a initial value
	/*
	car_node firstcar;
	car_node *head;
	strcpy(firstcar.lplate, allowed_cars[0]);
	firstcar.next = NULL;
	head = &firstcar;
	
	
	ent_queue[0] = (car_node *)malloc(sizeof(car_node));
	*ent_queue[0]->lplate = (char *)malloc(sizeof(char)*6);
	copy_plate(ent_queue[0]->lplate, allowed_cars[0]);
	ent_queue[0]->next = NULL;
	
	for(int i = 1; i < ENTRANCES; i ++){
		ent_queue[i] = NULL;
	}
	for(int i = 0; i < LEVELS; i++){
		cars_parked[i] = NULL;
	}

	printf("LICENSE STORE 1 ");
	print_plate(allowed_cars[0]);
	printf("\n");
	
	
	/*
	struct two_addr *args = (struct two_addr *)malloc(sizeof(struct two_addr));
	args->lpraddr = ent_lpr_addr + 0;
	args->queueaddr = n;
	*/
/*
	int *arg = (int *)malloc(sizeof(int));
	*arg = 0;
	//car_node * carptr = (car_node *)args->queueaddr;s
	printf("TEST 2.5\n");

	//printf("%s\n", carptr->lplate);
	printf("TEST 3\n");

	//enter_carpark(arg);
	//strcpy((char *)(shm + 88), "000000");
	//sprintf((shm+88), "%s", "000000");
	printf("TEST4\n");
	//pthread_join(enterparkthread, NULL);

	printf("TEST 4.5\n");
	printf("ENT 1 BOOMGATE STATUS: %c\n", ent_boom_addr[0]->s);
	printf("ENT 1 BOOMGATE ADDR %x\n",&ent_boom_addr[0]);
	printf("ENT 1 LPR ADDR %x\n", &ent_lpr_addr[0]);
	printf("ENT LPR BOOM SPACE %d\n", (int)&ent_boom_addr[0] - (int)&ent_lpr_addr[0]);

	printf("Theoretical size %d\n", sizeof(LPR));
	printf("LVL 2 TEMP SENSOR: %d\n", lvl_tmpalrm_addr[1]->tempsensor);
	printf("LVL 3 FIRE ALARM STATUS: %d\n", lvl_tmpalrm_addr[2]->falarm);

	*openboomgate(ent_boom_addr[2]);
	printf("ENT 3 BOOMGATE STATUS: %c\n", ent_boom_addr[2]->s);

	*closeboomgate(ent_boom_addr[2]);
	printf("ENT 3 BOOMGATE STATUS: %c\n", ent_boom_addr[2]->s);

	for (int i = 0; i < ENTRANCES; i++){
		printf("STATUS SIGN %d: %c\n", i, ent_info_addr[i]->display);
	}*/
	//char *ptr = (char *)(shm + 88);
	//printf("TEST 5\n");

	
	pthread_t cargenthread[ENTRANCES] = malloc(sizeof(pthread_t)*ENTRANCES);
	for(int i = 0; i < ENTRANCES; i++){
		pthread_create(&cargenthread[i], NULL, generate_car, i);
	}

	pthread_t enterparkthread[ENTRANCES] = malloc(sizeof(pthread_t)*ENTRANCES);
	for(int i = 0; i< ENTRANCES; i++){
		pthread_create(&enterparkthread[i], NULL, enter_carpark, i);
	}

	pthread_t carexitthread[EXITS] = malloc(sizeof(pthread_t)*ENTRANCES);
	for(int i = 0; i < EXITS; i++){
		pthread_create(&carexitthread[i], NULL, exit_carpark, i);
	}
	

	//test display
	for(;;){
	printf("CAR PARKS:\n");
	car_node *carptr;
	for(int i = 0; i < LEVELS; i++){
		carptr = cars_parked[i];
		printf("LEVEL %d: ", i+1);
		while(carptr != NULL){
			print_plate(carptr->lplate); printf("--");
			carptr = carptr->next;
		}
		printf("\n");
	}
	usleep(10);
	clrscr();
	//for(int i = 0; i < LEVELS; i++){printf("\v");}
	}
	pthread_join(cargenthread, NULL);
	pthread_join(enterparkthread,NULL);
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
	printf("TEST9\n");
	return 0;
}
