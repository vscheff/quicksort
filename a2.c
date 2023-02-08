#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/*
  NUM_FUNCS := Number of functions being tested
SAMPLE_SIZE := Size of the array used in the verbose sample
MIN_SUB_LEN := Minimum subarray length for which Median and Random will select an alternate pivot
OUTPUT_FILE := Name of the output file where result data will be stored
 HEADER_ROW := Header row of the output file
 CLOCK_CALC := Math used to compute the nanoseconds required during benchmarks
*/
#define NUM_FUNCS 3
#define SAMPLE_SIZE 16
#define MIN_SUB_LEN 5
#define OUTPUT_FILE "output.csv"
#define HEADER_ROW "n,legacy,median,random,worst legacy,worst median,worst random\n"
#define CLOCK_CALC (end_t.tv_sec - top_t.tv_sec) * 1000000000 + end_t.tv_nsec - top_t.tv_nsec

// Holds the timing data for one sorting function
// attr           name - name of the function
// attr rand_data_time - nanoseconds required to sort an array with random elements
// attr wrst_case_time - nanoseconds required to sort an already sorted array
// attr           func - pointer to the sorting function
typedef struct {
	char *name;
	uint64_t rand_data_time;
	uint64_t wrst_case_time;
	void (*func)(uint64_t *, int64_t, int64_t);
} FUNC_TIME_T;

void verbose_sample(FUNC_TIME_T *);
void legacy_quicksort(uint64_t*, int64_t, int64_t);
void median_quicksort(uint64_t*, int64_t, int64_t);
void random_quicksort(uint64_t*, int64_t, int64_t);

// partition() will print subarrays when this is set
_Bool FLAG_print_subarrays = 0;

int main(int argc, char **argv) {
	// Warn user if they input an incorrect number of arguments, and halt execution
	if (argc != 4) { fprintf(stderr, "Usage: %s <seed> <num_sizes> <num_tests>\n", argv[0]); return -1; }

	// Warn user if they entered a non-positive integer for num_of_tests
	const uint8_t Num_Tests = atoi(argv[3]);
	if (Num_Tests < 1) { fprintf(stderr, "At least 1 test required for average times\n"); return -2; }

	const uint8_t num_sizes = atoi(argv[2]);
	if (num_sizes < 1) return -3;

	// Compute the maximum array length per the input number of sizes
	const uint64_t Max_Array_Length = 1 << (num_sizes - 1);

	srand(atoi(argv[1]));

	FUNC_TIME_T funcs[NUM_FUNCS] = {
		{"Legacy", 0, 0, &legacy_quicksort},
       		{"Median", 0, 0, &median_quicksort},
		{"Random", 0, 0, &random_quicksort}
	};

	verbose_sample(funcs);
	
	// Write header row to output file
	FILE *output_file = fopen(OUTPUT_FILE, "w");
	fwrite(HEADER_ROW, sizeof(char), strlen(HEADER_ROW), output_file);

	uint64_t *arr = NULL, *arr_copy = NULL;
	struct timespec top_t, end_t;

	// Loop for each array length
	for (uint64_t n = 1; n <= Max_Array_Length; n *= 2) {	
		arr = realloc(arr, n * (sizeof *arr));
		arr_copy = realloc(arr_copy, n * (sizeof *arr_copy));

		// Reset all function timers to 0
		for (uint8_t i = 0; i < NUM_FUNCS; i++) funcs[i].rand_data_time = funcs[i].wrst_case_time = 0;

		// Loop a number of times equal to the requeseted number of tests
		for (uint8_t _ = 0; _ < Num_Tests; _++) {
			// Populate the array with random integers in range [0, n]
			for (uint64_t i = 0; i < n; i++) arr[i] = arr_copy[i] = rand() % n;

			// Loop for each sorting function
			for (uint8_t i = 0; i < NUM_FUNCS; i++) {
				// Record time taken to sort an array of random integers
				clock_gettime(CLOCK_MONOTONIC, &top_t);
				funcs[i].func(arr, 0, n - 1);
				clock_gettime(CLOCK_MONOTONIC, &end_t);
				funcs[i].rand_data_time += CLOCK_CALC;

				// Record time taken to sort an already sorted array
				clock_gettime(CLOCK_MONOTONIC, &top_t);
				funcs[i].func(arr, 0, n - 1);
				clock_gettime(CLOCK_MONOTONIC, &end_t);
				funcs[i].wrst_case_time += CLOCK_CALC;

				if (i < NUM_FUNCS - 1) memcpy(arr, arr_copy, n * (sizeof *arr));
			}
		}

		// Print the average times for each function
		for (uint8_t i = 0; i < NUM_FUNCS; i++) {
			funcs[i].rand_data_time /= Num_Tests;
			funcs[i].wrst_case_time /= Num_Tests;
			printf(
				"%s rand data time with %lu len array = %lu\n",
				funcs[i].name, n, funcs[i].rand_data_time
			);
			printf(
				"%s wrst case time with %lu len array = %lu\n",
				funcs[i].name, n, funcs[i].wrst_case_time
			);
		}

		printf("\n");

		// Write the average times to the output file
		char *output_str = NULL;
		int16_t str_len = asprintf(
				&output_str, "%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", n,
				funcs[0].rand_data_time, funcs[1].rand_data_time, funcs[2].rand_data_time, 
				funcs[0].wrst_case_time, funcs[1].wrst_case_time, funcs[2].wrst_case_time
		);
		fwrite(output_str, sizeof(char), str_len, output_file);

		free(output_str);
	}

	fclose(output_file);
	free(arr);
	free(arr_copy);

	return 0;
}

// Prints an array of integers
// param  arr - pointer to the beginning of the array
// param size - number of elements in the array
void print_array(uint64_t *arr, uint64_t size) {
	printf("[");

	for (uint64_t *end = &arr[size - 1]; arr <= end; arr++) printf("%lu, ", *arr);

	printf("\b\b]\n");
}

// Runs a small sample array through each of the functions, while verbosely printing each recursive step
// param func_times - array of timing objects for each function
void verbose_sample(FUNC_TIME_T *funcs) {
	uint64_t arr[SAMPLE_SIZE], arr_copy[SAMPLE_SIZE];
	
	// Populate the array with integers in range [0, 99]
	for (uint8_t i = 0; i < SAMPLE_SIZE; i++) arr[i] = arr_copy[i] = rand() % 100;

	printf("Beginning sample run with array of length %d.\n", SAMPLE_SIZE);

	FLAG_print_subarrays = 1;

	// Run the sample array through each sorting function
	for (uint8_t i = 0; i < NUM_FUNCS; i++) {
		printf("\n\n%s Quicksort with random array: ", funcs[i].name);
		print_array(arr, SAMPLE_SIZE);
		funcs[i].func(arr, 0, SAMPLE_SIZE - 1);

		printf("\n%s Quicksort with sorted array: ", funcs[i].name);
		print_array(arr, SAMPLE_SIZE);
		funcs[i].func(arr, 0, SAMPLE_SIZE - 1);

		if (i < NUM_FUNCS - 1) memcpy(arr, arr_copy, SAMPLE_SIZE * (sizeof *arr));
	}

	FLAG_print_subarrays = 0;

	printf("\n");
}

// Swaps the location of two given integers
// param a - pointer to first integer to swap
// param b - pointer to second integer to swap
extern inline void interchange(uint64_t *a, uint64_t *b) {
	uint64_t t = *a;
	*a = *b;
	*b = t;
}

// Helper function for Quicksort functions
//  param a - pointer to an array of integers
//  param m - index of the first element in the subarray
//  param p - index of the last element in the subarray
// return j - index of the pivot element
int64_t partition(uint64_t *a, int64_t m, int64_t p) {
	uint64_t v = a[m];
	int64_t i = m, j = p;

	// Loop until bottom index crosses top index
	while (i < j) {
		// Find index of element greater than pivot element
		do { i++; } while (a[i] < v && i < p);
		
		// Find index of element less than pivot element
		do { j--; } while (a[j] > v);

		if (i < j) interchange(&a[i], &a[j]);
	}

	a[m] = a[j];
	a[j] = v;

	if (FLAG_print_subarrays) {
		printf("Pivot at index [%ld] with value %ld\nSubarray: ", j, v);
		print_array(&a[m], p - m);
	}

	return j;
}

// An implementation of the Quicksort algorithm using the first element as the pivot
// param a - pointer to an array of integers
// param p - index of the first element in the subarray
// param q - index of the last element in the subarray
void legacy_quicksort(uint64_t *a, int64_t p, int64_t q) {
	if (p >= q) return;

	int64_t j = partition(a, p, q + 1);

	legacy_quicksort(a, p, j - 1);
	legacy_quicksort(a, j + 1, q);
}

// An implementation of the Quicksort algorithm using the median of three as the pivot
// param a - pointer to an array of integers
// param p - index of the first element in the subarray
// param q - index of the last element in the subarray
void median_quicksort(uint64_t *a, int64_t p, int64_t q) {
	if (p >= q) return;
	
	if (q - p > MIN_SUB_LEN) {
		int64_t mid = (p + q) / 2;

		// Is a[q] greater than exclusively a[p] or a[mid]?
		if ( (a[q] > a[p]) ^ (a[q] > a[mid]) ) interchange(&a[q], &a[p]); 
	       	// Is a[mid] smaller than exclusively a[p] or a[q]?
		else if ( (a[mid] < a[p]) ^ (a[mid] < a[q]) ) interchange(&a[mid], &a[p]); 
	}

	int64_t j = partition(a, p, q + 1);

	median_quicksort(a, p, j - 1);
	median_quicksort(a, j + 1, q);
}

// An implementation of the Quicksort algorithm using a random element as the pivot
void random_quicksort(uint64_t *a, int64_t p, int64_t q) {
	if (p >= q) return;

	if (q - p > MIN_SUB_LEN) interchange(&a[rand() % (q - p + 1) + p], &a[p]);
	
	int64_t j = partition(a, p, q + 1);

	random_quicksort(a, p, j - 1);
	random_quicksort(a, j + 1, q);
}

