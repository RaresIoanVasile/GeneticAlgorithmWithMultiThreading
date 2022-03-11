#include <stdlib.h>
#include "genetic_algorithm.h"
#include <pthread.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	int nr_threads = 0;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &nr_threads, argc, argv)) {
		return 0;
	}

	struct myStruct array[nr_threads];
	pthread_t threads[nr_threads];

	pthread_barrier_t* barrier = (pthread_barrier_t*) malloc(sizeof(pthread_barrier_t));
	pthread_barrier_init(barrier, NULL, nr_threads);
	individual* current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual* next_generation = (individual*) calloc(object_count, sizeof(individual));
	individual* tmp = NULL;

	for (int i = 0; i < nr_threads; i++) {
		array[i].objects = (sack_object*) calloc(object_count, sizeof(sack_object));
		array[i].object_count = object_count;
		array[i].objects = objects;
		array[i].generations_count = generations_count;
		array[i].sack_capacity = sack_capacity;
		array[i].nr_threads = nr_threads;
		array[i].id = i;
		array[i].barrier = barrier;
		array[i].current_generation = current_generation;
		array[i].next_generation = next_generation;
		array[i].tmp = tmp;
		pthread_create(&threads[i], NULL, &run_genetic_algorithm, (void*) &array[i]);
	}

	for (int i = 0; i < nr_threads; i++) {
		pthread_join(threads[i], NULL);
	}
	free(objects);

	return 0;
}
