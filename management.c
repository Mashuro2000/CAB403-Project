#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "parking.h"
#include <string.h>
#include <stdbool.h>

int shm_fd;
volatile void *shm;


struct car_num_count {
	int count_lvl[5];
	pthread_mutex_t m;
	pthread_cond_t c;
};
typedef struct car_num_count car_num_count;


LPR *ent_lpr_addr[ENTRANCES];
boomgate *ent_boom_addr[ENTRANCES];
infosign *ent_info_addr[ENTRANCES];

LPR *ext_lpr_addr[EXITS];
boomgate *ext_boom_addr[EXITS];

LPR *lvl_lpr_addr[EXITS];
temp_alarm *lvl_tmpalrm_addr[EXITS];

void init()
{
	
	if ((shm = mmap(0, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1) perror("mmap");

	

	// MAP rest of entrances

	for(int i = 0; i < ENTRANCES; i++){ //ent_lpr_addr[0] + 
		
		ent_lpr_addr[i] = (LPR *)(shm + ENT_GAP*i);
	}
	for (int i = 0; i < ENTRANCES; i++){
		

		ent_boom_addr[i] = (boomgate *)(shm + sizeof(LPR) + ENT_GAP*i);
		ent_info_addr[i] = (infosign *)(shm + sizeof(LPR) + sizeof(boomgate) + ENT_GAP*i);
	}

	// map exits
	for (int i = 0; i < EXITS; i++){
		
		ext_lpr_addr[i] = (LPR *)(shm + EXT_OFFSET + EXT_GAP*i);
		ext_boom_addr[i] = (boomgate *)(shm + EXT_OFFSET +sizeof(LPR) + EXT_GAP*i);
	}

	// map levels
	for (int i = 0; i < LEVELS; i++){
		
		lvl_lpr_addr[i] = (LPR *)(shm + LVL_OFFSET + LVL_GAP*i);
		lvl_tmpalrm_addr[i] = (temp_alarm*)(shm + LVL_OFFSET + sizeof(LPR) + LVL_GAP*i);
		

	}

	

	
}

char allowed_cars[NUM_ALLOW_CARS][PLATESIZE];
car_num_count cars_on_lvl;
typedef struct car car_t;

struct car
{
	char *key;
	char *lplate;
	car_t *next;
};

typedef struct
{
	car_t **buckets;
	size_t size;
} htab;

void print_car(car_t *i)
{
	printf("%s\n", i->lplate);
}

bool htab_init(htab *h, size_t n)
{
	h->size = n;
	h->buckets = (car_t **)calloc(n, sizeof(car_t *));
	return h->buckets != NULL;
}

size_t hash_function(char *s)
{
	size_t hash = 5381;
	int c;
	while ((c = *s++) != '\0')
	{
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

size_t htab_index(htab *h, char *key)
{
	return hash_function(key) % h->size;
}

car_t *htab_bucket(htab *h, char *key)
{
	return h->buckets[htab_index(h, key)];
}

bool *htab_find(htab *h, char *key)
{
	for (car_t *i = htab_bucket(h, key); i != NULL; i = i->next)
	{
		if (strcmp(i->key, key) == 0)
		{ // found the key
			return true;
		}
	}
	return false;
}

bool htab_add(htab *h, char *key, char *lplate)
{
	// allocate new item
	car_t *newhead = (car_t *)malloc(sizeof(car_t));
	if (newhead == NULL)
	{
		return false;
	}
	newhead->key = key;
	newhead->lplate = lplate;

	// hash key and place item in appropriate bucket
	size_t bucket = htab_index(h, key);
	newhead->next = h->buckets[bucket];
	h->buckets[bucket] = newhead;
	return true;
}

void htab_print(htab *h)
{
	printf("hash table with %d buckets\n", h->size);
	for (size_t i = 0; i < h->size; ++i)
	{
		printf("bucket %d: ", i);
		if (h->buckets[i] == NULL)
		{
			printf("empty\n");
		}
		else
		{
			for (car_t *j = h->buckets[i]; j != NULL; j = j->next)
			{
				print_car(j);
				if (j->next != NULL)
				{
					printf(" -> ");
				}
			}
			printf("\n");
		}
	}
}

void htab_delete(htab *h, char *key)
{
	car_t *head = htab_bucket(h, key);
	car_t *current = head;
	car_t *previous = NULL;
	while (current != NULL)
	{
		if (strcmp(current->key, key) == 0)
		{
			if (previous == NULL)
			{
				h->buckets[htab_index(h, key)] = current->next;
			}
			else
			{
				previous->next = current->next;
			}
			free(current);
			break;
		}
		previous = current;
		current = current->next;
	}
}

void htab_destroy(htab *h)
{
	// free linked lists
	for (size_t i = 0; i < h->size; ++i)
	{
		car_t *bucket = h->buckets[i];
		while (bucket != NULL)
		{
			car_t *next = bucket->next;
			free(bucket);
			bucket = next;
		}
	}

	// free buckets array
	free(h->buckets);
	h->buckets = NULL;
	h->size = 0;
}

htab carparklvls[LEVELS];

// open and close boomgate functions
void *openent_boomgate(void *arg)
{
    int entrance_num = *(int*)arg;
    pthread_mutex_lock(&ent_boom_addr[entrance_num]->m);
	printf("MUTEX LOCKED, going to try raise gate\n");
	printf("GATE STAT %c\n", ent_boom_addr[entrance_num]->s);
    for (;;) {
		if(ent_boom_addr[entrance_num]->s == 'C'){
			ent_boom_addr[entrance_num]->s = 'R';
		}
		else if(ent_boom_addr[entrance_num]->s == 'O'){
			break;
		}

      	if (ent_boom_addr[entrance_num]->s == 'R') {
        	usleep(10 * 1000); //simulate 10ms of waiting for gate to open
        	ent_boom_addr[entrance_num]->s = 'O';
        	pthread_cond_broadcast(&ent_boom_addr[entrance_num]->c);
		break;
      	}
      	pthread_cond_wait(&ent_boom_addr[entrance_num]->c, &ent_boom_addr[entrance_num]->m);
    }
	pthread_mutex_unlock(&ent_boom_addr[entrance_num]->m);
} 

void *openext_boomgate(void *arg)
{
    int exit_num = *(int*)arg;
    pthread_mutex_lock(&ext_boom_addr[exit_num]->m);
	printf("MUTEX LOCKED, going to try raise gate\n");
	printf("GATE STAT %c\n", ent_boom_addr[exit_num]->s);
    for (;;) {
		if(ext_boom_addr[exit_num]->s == 'C'){
			ext_boom_addr[exit_num]->s = 'R';
		}
		else if(ext_boom_addr[exit_num]->s == 'O'){
			break;
		}

      	if (ext_boom_addr[exit_num]->s == 'R') {
        	usleep(10 * 1000); //simulate 10ms of waiting for gate to open
        	ext_boom_addr[exit_num]->s = 'O';
        	pthread_cond_broadcast(&ext_boom_addr[exit_num]->c);
		break;
      	}
      	pthread_cond_wait(&ext_boom_addr[exit_num]->c, &ext_boom_addr[exit_num]->m);
    }
	pthread_mutex_unlock(&ext_boom_addr[exit_num]->m);
} 

void *closeent_boomgate(void *arg)
{
    int entrance_num = *(int *)arg;
    pthread_mutex_lock(&ent_boom_addr[entrance_num]->m);
	printf("MUTEX LOCKED, going to try lower gate\n");
	printf("GATE STAT %c\n", ent_boom_addr[entrance_num]->s);
    for (;;) {
		if(ent_boom_addr[entrance_num]->s == 'O'){
			ent_boom_addr[entrance_num]->s = 'L';
			pthread_cond_broadcast(&ent_boom_addr[entrance_num]->c);
		}
		else if(ent_boom_addr[entrance_num]->s == 'C'){
			break;
		}
		
      	if (ent_boom_addr[entrance_num]->s == 'L') {
        	usleep(10 * 1000); //simulate 10ms of waiting for gate to open
        	ent_boom_addr[entrance_num]->s = 'C';
        	pthread_cond_broadcast(&ent_boom_addr[entrance_num]->c);
		break;
      	}
      	pthread_cond_wait(&ent_boom_addr[entrance_num]->c, &ent_boom_addr[entrance_num]->m);
    }
	pthread_mutex_unlock(&ent_boom_addr[entrance_num]->m);
} 

void *closeexit_boomgate(void *arg)
{
    int exit_num = *(int *)arg;
    pthread_mutex_lock(&ext_boom_addr[exit_num]->m);
	printf("MUTEX LOCKED, going to try lower gate\n");
	printf("GATE STAT %c\n", ext_boom_addr[exit_num]->s);
    for (;;) {
		if(ext_boom_addr[exit_num]->s == 'O'){
			ext_boom_addr[exit_num]->s = 'L';
			pthread_cond_broadcast(&ext_boom_addr[exit_num]->c);
		}
		else if(ext_boom_addr[exit_num]->s == 'C'){
			break;
		}
		
      	if (ext_boom_addr[exit_num]->s == 'L') {
        	usleep(10 * 1000); //simulate 10ms of waiting for gate to close
        	ext_boom_addr[exit_num]->s = 'C';
        	pthread_cond_broadcast(&ext_boom_addr[exit_num]->c);
		break;
      	}
      	pthread_cond_wait(&ext_boom_addr[exit_num]->c, &ext_boom_addr[exit_num]->m);
    }
	pthread_mutex_unlock(&ext_boom_addr[exit_num]->m);
} 

//parking billing is done by 5 cents for every millisecond car is at carpark from entering to exit
//Calculate and create the bill txt file
void billing(){

	//open file
	FILE *fptr;

	fptr = fopen("billing.txt", "a");

	//to do - write fuction to get how much revenue

	//Write license plate and how much its parking cost by accessing shm data
	fprintf(fptr, "%s $%.2f \n");
	fclose(fptr);


}


// Managment system can only read sensors and keep track of cars that way, along with issuing commands to
// boom gates and display screens, and time keeping and generating billing

void display()
{
	printf("\n");
	
	for (int i = 0; i < LEVELS; i++)
	{
		printf("Level %d Capacity: %d/20 \n",i+1, cars_on_lvl.count_lvl[i]);
	}
	

	//Entrance boom gate current status
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{
		printf("Entrance %d: %c \t\n", i+1, (char)ent_boom_addr[0]->s);
	}

	//Exit boom gate current status
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{		
		printf("Exit %d: %c \t\n", i+1, (char)ext_boom_addr[i]->s);
	}

	//current temp 
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{
		printf("Level %d Temperature: %d \n", i+1, (int)lvl_tmpalrm_addr[i]->tempsensor);
	}
	
	//Alarm status 1 = on 0 = false
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{
		printf("Level %d Alarm: %d \n", i+1, (int)lvl_tmpalrm_addr[i]->falarm);
	}

	//How much money system has made
	printf("\n");
	printf("Revenue: %d");

	printf("\n");
	usleep(5000);
	system("clear");
		
	
}

// read in license plates from file
void read_allowed_plates_from_file()
{
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

void cleanup()
{
	for (int i = 1; i < ENTRANCES; i++)
	{
		munmap(ent_lpr_addr[i] + ENT_GAP * i, sizeof(LPR));
		munmap(ent_boom_addr[i] + ENT_GAP * i, sizeof(boomgate));
		munmap(ent_info_addr[i] + ENT_GAP * i, sizeof(infosign));
	}

	// map exits
	for (int i = 0; i < EXITS; i++)
	{
		munmap(ext_lpr_addr[i] + EXT_GAP * i, sizeof(LPR));
		munmap(ext_boom_addr[i] + EXT_GAP * i, sizeof(boomgate));
	}

	// map levels
	for (int i = 0; i < LEVELS; i++)
	{
		munmap(lvl_lpr_addr[i] + LVL_GAP * i, sizeof(LPR));
		munmap(lvl_tmpalrm_addr[i] + LVL_GAP * i, sizeof(temp_alarm));
	}
}

void *checkLPR(void * arg) {
	
	int entrance_num = (int *)arg;
	pthread_mutex_unlock(&ent_lpr_addr[entrance_num]->m);
	pthread_mutex_lock(&ent_lpr_addr[entrance_num]->m);
	for (int i = 0; i < NUM_ALLOW_CARS; i++)
	{
		//usleep(100);
		char temp[6]; 
		copy_plate(temp, allowed_cars[i]);
		if (strcmp(temp,ent_lpr_addr[entrance_num]->plate) == 0)
		{
			if (htab_find(&carparklvls[entrance_num], temp))
			{
				break;
			} else {
				pthread_mutex_unlock(&cars_on_lvl.m);
				pthread_mutex_lock(&cars_on_lvl.m);
				cars_on_lvl.count_lvl[i]++;
				pthread_cond_broadcast(&cars_on_lvl.c);
				htab_add(&carparklvls[entrance_num],ent_lpr_addr[entrance_num]->plate,ent_lpr_addr[entrance_num]->plate);
				pthread_cond_broadcast(&ent_lpr_addr[entrance_num]->c);
				break;
			}
			
		} else {
			//printf("CAR NOT ALLOWED\n");
			continue;
		}
		pthread_cond_wait(&ent_lpr_addr[entrance_num]->c,&ent_lpr_addr[entrance_num]->m);
		pthread_cond_wait(&cars_on_lvl.c,&cars_on_lvl.m);
	}
	pthread_mutex_unlock(&ent_lpr_addr[entrance_num]->m);
	
	
}

int main(int argc, char **argv)
{
	
	// read in license plate file

	// open shared memory
	shm_fd = shm_open("PARKING", O_RDWR, 0666);
	
	init();

	
	
	
	for(;;) {
		display();

		//checking lpr and adding them to the hashtable data struct


		
		pthread_t checkLPRthread[LEVELS];
		for (int i = 0; i < LEVELS; ++i)
		{
			if (pthread_create(&checkLPRthread[i], NULL, checkLPR, i) != 0) {
				perror("Pthread error");
				break;
			}
		}
		
		for (int i = 0; i < LEVELS; ++i)
		{
			if (pthread_join(checkLPRthread[i], NULL) != 0)
        	{
          		fprintf(stderr, "error: Cannot join thread # %d\n", i);
        	}
		}

	}
	

	cleanup();
	close(shm_fd);
}