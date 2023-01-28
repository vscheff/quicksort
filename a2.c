#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_FUNCS 3
#define SAMPLE_SIZE 10
#define MIN_SUB_LEN 5
#define OUTPUT_FILE "ouput.csv"
#define HEADER_ROW "n,legacy,median,random,worst legacy,worst median,worst random\n"
#define CLOCK_CALC (end_t.tv_sec - top_t.tv_sec) * 1000000000 + end_t.tv_nsec - top_t.tv_nsec

typedef struct {
	char *name;
	size_t rand_data_time;
	size_t wrst_case_time;
	void (*func)(int *, int, int);
} FUNC_TIME_T;

void verbose_sample(FUNC_TIME_T *);
void legacy_quicksort(int*, int, int);
void median_quicksort(int*, int, int);
void random_quicksort(int*, int, int);

short print_subarrays = 0;

int main(int argc, char **argv) {
	// Warn user if they input an incorrect number of arguments, and halt execution
	if (argc != 4) { fprintf(stderr, "Usage: %s <seed> <num_sizes> <num_tests>\n", argv[0]); return -1; }

	// Warn user if they entered a non-positive integer for num_of_tests
	const int Num_Tests = atoi(argv[3]);
	if (Num_Tests < 1) { fprintf(stderr, "At least 1 test required for average times\n"); return -2; }

	int num_sizes = atoi(argv[2]);
	if (num_sizes < 1) return -3;

	// Compute the maximum array length per the input number of sizes
	const size_t Max_Array_Length = 1 << (num_sizes - 1);

	srand(atoi(argv[1]));

	FUNC_TIME_T func_times[NUM_FUNCS] = {
		{"Legacy", 0, 0, &legacy_quicksort},
       		{"Median", 0, 0, &median_quicksort},
		{"Random", 0, 0, &random_quicksort}
	};

	verbose_sample(func_times);
	
	// Write header row to output files
	FILE *output_file = fopen(OUTPUT_FILE, "w");
	fwrite(HEADER_ROW, sizeof(char), strlen(HEADER_ROW), output_file);
	fclose(output_file);

	int *arr = NULL, *arr_copy = NULL;
	
	struct timespec top_t, end_t;

	for (size_t n = 1; n <= Max_Array_Length; n *= 2) {	
		arr = realloc(arr, n * sizeof(int));
		arr_copy = realloc(arr_copy, n * sizeof(int));

		for (int i = 0; i < NUM_FUNCS; i++) {
			func_times[i].rand_data_time = func_times[i].wrst_case_time = 0;
		}

		for (unsigned _ = 0; _ < Num_Tests; _++) {
			for (size_t i = 0; i < n; i++) arr[i] = arr_copy[i] = rand() % n;

			for (int i = 0; i < NUM_FUNCS; i++) {
				clock_gettime(CLOCK_MONOTONIC, &top_t);
				func_times[i].func(arr, 0, n - 1);
				clock_gettime(CLOCK_MONOTONIC, &end_t);
				func_times[i].rand_data_time += CLOCK_CALC;

				clock_gettime(CLOCK_MONOTONIC, &top_t);
				func_times[i].func(arr, 0, n - 1);
				clock_gettime(CLOCK_MONOTONIC, &end_t);
				func_times[i].wrst_case_time += CLOCK_CALC;

				if (i < 2) memcpy(arr, arr_copy, n * sizeof(int));
			}
		}

		for (int i = 0; i < NUM_FUNCS; i++) {
			func_times[i].rand_data_time /= Num_Tests;
			func_times[i].wrst_case_time /= Num_Tests;
			printf(
				"%s rand data time with %lu len array = %lu\n",
				func_times[i].name, n, func_times[i].rand_data_time
			);
			printf(
				"%s wrst case time with %lu len array = %lu\n",
				func_times[i].name, n, func_times[i].wrst_case_time
			);
		}

		printf("\n");

		char * output_str = NULL;
		int len = asprintf(
				&output_str, "%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", n,
				func_times[0].rand_data_time, func_times[1].rand_data_time,
				func_times[2].rand_data_time, func_times[0].wrst_case_time,
				func_times[1].wrst_case_time, func_times[2].wrst_case_time
			  );
		output_file = fopen(OUTPUT_FILE, "a");
		fwrite(output_str, sizeof(char), len, output_file);

		fclose(output_file);
		free(output_str);
	}

	return 0;
}

void print_array(int arr[], int size) {
	printf("[");

	for (int i = 0; i < size; i++) printf("%d, ", arr[i]);

	printf("\b\b]\n");
}

void verbose_sample(FUNC_TIME_T *func_times) {
	int arr[SAMPLE_SIZE], arr_copy[SAMPLE_SIZE];
	for (int i = 0; i < SAMPLE_SIZE; i++) arr[i] = arr_copy[i] = rand() % 100;

	printf("Beginning verbose sample run with array of length %d: ", SAMPLE_SIZE);
	print_array(arr, SAMPLE_SIZE);

	print_subarrays = 1;
		
	for (int i = 0; i < NUM_FUNCS; i++) {
		printf("\n%s Quicksort:\n", func_times[i].name);
		func_times[i].func(arr, 0, SAMPLE_SIZE - 1);

		if (i < 2) memcpy(arr, arr_copy, SAMPLE_SIZE * sizeof(int));	
	}

	print_array(arr, SAMPLE_SIZE);
	printf("\n");
	print_subarrays = 0;
}

void interchange(int *a, int *b) {
	int t = *a;
	*a = *b;
	*b = t;
}

int partition(int *a, int m, int p) {
	int v = a[m], i = m, j = p;
	
	while (i < j) {
		do { i++; } while (a[i] < v);
		
		do { j--; } while (a[j] > v);

		if (i < j) interchange(&a[i], &a[j]);
	}

	a[m] = a[j];
	a[j] = v;

	if (! print_subarrays) return j;

	printf("Pivot at index [%d] with value %d\nSubarray: ", j, v);
	print_array(&a[m], p - m);

	return j;
}

void legacy_quicksort(int *a, int p, int q) {
	if (p >= q) return;

	int j = partition(a, p, q + 1);

	legacy_quicksort(a, p, j - 1);
	legacy_quicksort(a, j + 1, q);
}

int median(int *a, int m, int p) {
	int x = m, y = (m + p) / 2, z = p;
	
	if (a[x] <= a[y] && a[y] <= a[z]) return y;
	if (a[x] <= a[z] && a[z] <= a[y]) return z;
	if (a[y] <= a[x] && a[x] <= a[z]) return x;
	if (a[y] <= a[z] && a[z] <= a[x]) return z;
	if (a[z] <= a[x] && a[x] <= a[y]) return x;
	
	return y;
}

void median_quicksort(int *a, int p, int q) {
	if (p >= q) return;
	
	if (q - p > MIN_SUB_LEN) interchange(&a[median(a, p, q)], &a[p]);
	
	int j = partition(a, p, q + 1);

	median_quicksort(a, p, j - 1);
	median_quicksort(a, j + 1, q);
}

void random_quicksort(int *a, int p, int q) {
	if (p >= q) return;

	if (q - p > MIN_SUB_LEN) interchange(&a[rand() % (q - p + 1) + p], &a[p]);
	
	int j = partition(a, p, q + 1);

	random_quicksort(a, p, j - 1);
	random_quicksort(a, j + 1, q);
}

