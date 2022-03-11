#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *nr_threads, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 3) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	*nr_threads = (int) strtol(argv[3], NULL, 10);

	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity, int nr_threads, int id)
{
	int weight;
	int profit;

	int start = id * (double) object_count / nr_threads;
	int end = min((id + 1) * (double) object_count / nr_threads, object_count);
	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

int cmpfunc1(individual first, individual second)
{
	int i;

	int res = second.fitness - first.fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first.chromosome_length && i < second.chromosome_length; ++i) {
			first_count += first.chromosomes[i];
			second_count += second.chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second.index - first.index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation, int nr_threads, int id)
{
	int i;
	int start = id * (double) generation -> chromosome_length / nr_threads;
	int end = min((id + 1) * (double) generation -> chromosome_length / nr_threads, generation -> chromosome_length);
	for (i = start; i < end; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

void merge(individual *arr, int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    individual *L = (individual*)calloc(n1, sizeof(individual)); 
    individual *R = (individual*)calloc(n2, sizeof(individual));
  
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];
  
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (cmpfunc1(L[i], R[j]) < 0) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
  
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
  
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}
  
void mergeSort(individual *arr, int l, int r)
{
    if (l < r) {

        int m = l + (r - l) / 2;

        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
  
        merge(arr, l, m, r);
    }
}

void *run_genetic_algorithm(void* args1)
{
	struct myStruct* args = (struct myStruct*) args1;
	int object_count = args -> object_count;
	int generations_count = args -> generations_count;
	int sack_capacity = args -> sack_capacity;
	int nr_threads = args -> nr_threads;
	int id = args -> id;
	sack_object* objects = (sack_object*) calloc(object_count, sizeof(sack_object));
	objects = args -> objects;
	int count, cursor;
	

	// set initial generation (composed of object_count individuals with a single item in the sack)
	int start = id * (double) object_count / nr_threads;
	int end = (id + 1) * (double) object_count / nr_threads;
	for (int i = start; i < end; ++i) {
		args -> current_generation[i].fitness = 0;
		args -> current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		args -> current_generation[i].chromosomes[i] = 1;
		args -> current_generation[i].index = i;
		args -> current_generation[i].chromosome_length = object_count;

		args -> next_generation[i].fitness = 0;
		args -> next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		args -> next_generation[i].index = i;
		args -> next_generation[i].chromosome_length = object_count;
	}
	pthread_barrier_wait(args -> barrier);


	// iterate for each generation
	for (int k = 0; k < generations_count; ++k) {

		pthread_barrier_wait(args -> barrier);
		cursor = 0;

		// compute fitness and sort by it
		compute_fitness_function(objects, args -> current_generation, object_count, sack_capacity, nr_threads, id);
		pthread_barrier_wait(args -> barrier);

		start = id * (double) object_count / nr_threads;
		end = (id + 1) * (double) object_count / nr_threads;
		end = end - 1;

        mergeSort(args -> current_generation, start, end);
        pthread_barrier_wait(args -> barrier);
        
    	if (id == 0) {
    		for (int i = 0; i < nr_threads - 1; ++i) {
    			end = (i + 2) * (double) object_count / nr_threads;
    			if (end >= object_count) {
    				end = object_count - 1;
    			}
    			merge(args -> current_generation, 0, end / 2, end);	
    		}
    	}
    	pthread_barrier_wait(args -> barrier);	

		// keep first 30% children (elite children selection)
		count = object_count * 3 / 10;
		start = id * (double) count / nr_threads;
		end = (id + 1) * (double) count / nr_threads;
		for (int i = start; i < end; ++i) {
			copy_individual(args -> current_generation + i, args -> next_generation + i);
		}
		pthread_barrier_wait(args -> barrier);

		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = object_count * 2 / 10;
		start = id * (double) count / nr_threads;
		end = (id + 1) * (double) count / nr_threads;
		for (int i = start; i < end; ++i) {
			copy_individual(args -> current_generation + i, args -> next_generation + cursor + i);
			mutate_bit_string_1(args -> next_generation + cursor + i, k);
		}
		pthread_barrier_wait(args -> barrier);

		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = object_count * 2 / 10;
		start = id * (double) count / nr_threads;
		end = (id + 1) * (double) count / nr_threads;
		for (int i = start; i < end; ++i) {
			copy_individual(args -> current_generation + i + count, args -> next_generation + cursor + i);
			mutate_bit_string_2(args -> next_generation + cursor + i, k);
		}
		pthread_barrier_wait(args -> barrier);
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = object_count * 3 / 10;

		if (count % 2 == 1) {
			copy_individual(args -> current_generation + object_count - 1, args -> next_generation + cursor + count - 1);
			count--;
		}
		pthread_barrier_wait(args -> barrier);

		start = id * (double) count / nr_threads;
		end = (id + 1) * (double) count / nr_threads;
		start = (start % 2) ? start + 1 : start;
		for (int i = start; i < count - 1 && i < end; i += 2) {
			crossover(args -> current_generation + i, args -> next_generation + cursor + i, k);
		}
		pthread_barrier_wait(args -> barrier);
		args -> tmp = args -> current_generation;
		args -> current_generation = args -> next_generation;
		args -> next_generation = args -> tmp;

		start = id * (double) object_count / nr_threads;
		end = (id + 1) * (double) object_count / nr_threads;
		for (int i = start; i < end; ++i) {
			args -> current_generation[i].index = i;
		}
		pthread_barrier_wait(args -> barrier);

		if (k % 5 == 0) {
			if (id == 0)
				print_best_fitness(args -> current_generation);
		}
	}

	compute_fitness_function(objects, args -> current_generation, object_count, sack_capacity, nr_threads, id);
    pthread_barrier_wait(args -> barrier);

	start = id * (double) object_count / nr_threads;
	end = (id + 1) * (double) object_count / nr_threads;
	end = end - 1;

    mergeSort(args -> current_generation, start, end);
    pthread_barrier_wait(args -> barrier);    
    if (id == 0) {
    	for (int i = 0; i < nr_threads - 1; ++i) {
    		end = (i + 2) * (double) object_count / nr_threads;
    		if (end >= object_count) {
    			end = object_count - 1;
    		}
    		merge(args -> current_generation, 0, end / 2, end);
    	}
    }
    pthread_barrier_wait(args -> barrier);	
	
	if (id == 0)
		print_best_fitness(args -> current_generation);

	pthread_exit(NULL);
}