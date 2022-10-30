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

lvl_count = 0; // should not exceed 20;

LPR *ent_lpr_addr[ENTRANCES];
boomgate *ent_boom_addr[ENTRANCES];
infosign *ent_info_addr[ENTRANCES];

LPR *ext_lpr_addr[EXITS];
boomgate *ext_boom_addr[EXITS];

LPR *lvl_lpr_addr[EXITS];
temp_alarm *lvl_tmpalrm_addr[EXITS];

void init()
{
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

	
}

char allowed_cars[NUM_ALLOW_CARS][PLATESIZE];

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
	printf("%s", i->lplate);
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

car_t *htab_find(htab *h, char *key)
{
	for (car_t *i = htab_bucket(h, key); i != NULL; i = i->next)
	{
		if (strcmp(i->key, key) == 0)
		{ // found the key
			return i;
		}
	}
	return NULL;
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

// open and close boomgate functions
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
	// number of vehicles on each level
	// max capacity on each level
	// current state of each LPR
	// total billing revenue recorded by manager
	// temp sensors

	//Capacity number/20
	printf("\n");
	printf("Level 1 Capacity: %d/20 \n");
	printf("Level 2 Capacity: %d/20 \n");
	printf("Level 3 Capacity: %d/20 \n");
	printf("Level 4 Capacity: %d/20 \n");
	printf("Level 5 Capacity: %d/20 \n");

	//Entrance boom gate current status
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{
		printf("Entrance %d: %c \t\n", i, (char)ent_boom_addr[0]->s);
	}

	//Exit boom gate current status
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{		
		printf("Exit %d: %c \t\n", i, (char)ext_boom_addr[i]->s);
	}

	//current temp 
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{
		printf("Level %d Temperature: %d \n", i, (int)lvl_tmpalrm_addr[i]->tempsensor);
	}
	
	//Alarm status 1 = on 0 = false
	printf("\n");
	for (int i = 0; i < LEVELS; i++)
	{
		printf("Level %d Alarm: %d \n", i, (int)lvl_tmpalrm_addr[i]->falarm);
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
	FILE *fptr;
	char ch;

	// open license plate files
	fptr = fopen("plates.txt", "r");

	if (fptr == NULL)
	{
		printf("Authorised Plates File not Found - Simulation\n");
	}
	else{
		printf("Authorised Plates File Found\n");
	}

	char tmp;
	for (int i = 0; i < NUM_ALLOW_CARS; i++)
	{
		fgets(allowed_cars[i], PLATESIZE, fptr);
		fgetc(fptr); // consume new line character
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
	int entrance_num = *(int *)arg;

	//ent_lpr_addr[entrance_num];
}

int main(int argc, char **argv)
{
	htab carparklvls[LEVELS];
	// read in license plate file

	// open shared memory
	shm_fd = shm_open("PARKING", O_RDWR, 0666);
	// shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	init();

	// create hashtables for each level
	size_t num_parks = 20;
	
	for (int i = 0; i < LEVELS; i++)
	{
		if (!htab_init(&carparklvls, num_parks))
    	{
        	printf("failed to initialise hash table\n");
        	return EXIT_FAILURE;
    	}

	}
	
	

	usleep(5000);
	
	
	for(;;) {
		display();
	}

	pthread_t checkentlprthread[LEVELS];
	//billing();
	// munmap((void *)shm, 2920);

	

	cleanup();
	close(shm_fd);
}
