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

LPR *ent_lpr_addr[ENTRANCES];
boomgate *ent_boom_addr[ENTRANCES];
infosign *ent_info_addr[ENTRANCES];

LPR *ext_lpr_addr[EXITS];
boomgate *ext_boom_addr[EXITS];

LPR *lvl_lpr_addr[EXITS];
temp_alarm *lvl_tmpalrm_addr[EXITS];

void init()
{
	// MAP memory spaces
	// Initial memory space
	if ((ent_lpr_addr[0] = mmap(0, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
		perror("mmap");

	// MAP rest of entrances
	for (int i = 0; i < ENTRANCES; i++)
	{
		if ((ent_lpr_addr[i] = mmap(ent_lpr_addr[0] + ENT_GAP * i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
			perror("mmap");
		if ((ent_boom_addr[i] = mmap(ent_lpr_addr[0] + sizeof(LPR) + ENT_GAP * i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
			perror("mmap");
		if ((ent_info_addr[i] = mmap(ent_lpr_addr[0] + sizeof(LPR) + sizeof(boomgate) + ENT_GAP * i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
			perror("mmap");
	}

	// map exits
	for (int i = 0; i < EXITS; i++)
	{
		if ((ext_lpr_addr[i] = mmap(ent_lpr_addr[0] + EXT_OFFSET + EXT_GAP * i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
			perror("mmap");
		if ((ext_boom_addr[i] = mmap(ent_lpr_addr[0] + EXT_OFFSET + sizeof(LPR) + EXT_GAP * i, sizeof(boomgate), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
			perror("mmap");
	}

	// map levels
	for (int i = 0; i < LEVELS; i++)
	{
		if ((lvl_lpr_addr[i] = mmap(ent_lpr_addr[0] + LVL_OFFSET + LVL_GAP * i, sizeof(LPR), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
			perror("mmap");
		if ((lvl_tmpalrm_addr[i] = mmap(ent_lpr_addr[0] + LVL_OFFSET + sizeof(LPR) + LVL_GAP * i, sizeof(infosign), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (void *)-1)
			perror("mmap");
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

// Managment system can only read sensors and keep track of cars that way, along with issuing commands to
// boom gates and display screens, and time keeping and generating billing

void display()
{
	// number of vehicles on each level
	// max capacity on each level
	// current state of each LPR
	// total billing revenue recorded by manager
	// temp sensors
}

// read in license plates from file
void read_allowed_plates_from_file()
{
	FILE *fptr;

	// open license plate files
	fptr = fopen("plates.txt", "r");

	if (fptr == NULL)
	{
		printf("Authorised Plates File not Found - Simulation\n");
	}

	char tmp;
	for (int i = 0; i < NUM_ALLOW_CARS; i++)
	{
		fgets(allowed_cars[i], PLATESIZE, fptr);
		fgetc(fptr); // consume new line character
	}
}

// 1 if allowed, 0 if not, allows management sytem to know whether to allow entry
int checklicense_forentry(char *plate)
{
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

int main(int argc, char **argv)
{
	// read in license plate file

	// open shared memory
	shm_fd = shm_open("PARKING", O_RDWR, 0666);
	// shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	init();

	// munmap((void *)shm, 2920);

	cleanup();
	close(shm_fd);
}