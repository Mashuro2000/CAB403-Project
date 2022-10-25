#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
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
struct cars {
    char *lplate;
    car_node *next;
};

// setup parking levels, initial temp and sensor values
void init() {

}

// read in license plates from file
void read_allowed_plates_from_file() {

}

// cars simulated locally in simulation file, but update LPR values
// cars update LPR value of entrance, updates LPR of level it goes to, waits updates LPR of level it leaves, updates LPR of its chosen exit

// 1 if allowed, 0 if not, allows car generator to see if it has generated a car with the correct plate
int checklicense_forcargen(char *plate) {

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
    new->lplate = car;
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
void *enter_carpark(struct cars * queue ,struct LPR *entrance, struct parkinglot plot) {
	
}

void exit_carpark(struct cars * queue ,struct LPR *exit, struct level *lvl) {
    //update LPR as car exits

    // cleans up memory for car
}

// get random time
int randtime(int min, int max) {

}

void simulate_car() {
    // 
}


void *openboomgate(void *arg) {
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
void *closeboomgate(void *arg) {

}

void *change_temp(void *lvl_addr) {

}

// trigger when simulating fire, aggressively change temperature on one level
void *simulate_fire(void *lvl_addr) {

}

// simulate environment (temps)
void simulate_env() {
    //seperate threads for changing temps on each level
}

int main(int argc, char* argv[]) {
    shm_fd = shm_open("PARKING", O_CREAT, 0);
	shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	// Read txt file for car list
	
	FILE *textfile;
	char line[LENGTH_OF_NUMBERPLATE];
	int i = 0;
	textfile = fopen("plates.txt", "r");
	while (fgets(line, LENGTH_OF_NUMBERPLATE, textfile) && i < STRING_LEN) {
		for (int j = 0; j < 6; j++) {
			carlist[i][j] = line[j];
		}
		i++;
	}

	

	// first value in linked list will be a initial value
	car_node firstcar;
	car_node *head;
	firstcar.lplate = carlist[0];
	firstcar.next = NULL;
	head = &firstcar;

	generate_car(firstcar);

    	
	munmap((void *)shm, 2920);
	close(shm_fd);
	return 0;
}