/*
 * read_stderr.c
 *
 *  Created on: 31/mar/2016
 *      Author: timmy
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <omp.h>
#include "mpi.h"

#define PATH_STDERR "/home/timmy/Scrivania/stderr-dataGenerator.txt"
#define PATH_STDOUT "/home/timmy/Scrivania/stdout-dataGenerator.txt"
#define LINES_STDOUT "/home/timmy/Scrivania/stdout-lines.txt"
#define HOUSE "house "
#define HOUSEHOLD "household "
#define PLUG "plug "
#define BLOCK 3600
#define SEND_ALL_HOUSE 0

//TODO modified hour[] from int to float
struct measurement {
	int date;
	float hour[24];
	int num_ts[24];
	struct measurement *next;
};
typedef struct measurement Measurement;

//Definition of plug
struct plug {
	int id;
	Measurement *my_measurements;
	Measurement *last_measurement;
	struct plug *next;
};
typedef struct plug Plug;

//Definition of household
struct household {
	int id;
	Plug *hh_plugs;
	struct household *next;
};
typedef struct household Household;

//Definition of house
struct house {
	int id;
	Household *h_households;
	int num_plug;
	Measurement *median_load;
	struct house *next;
};
typedef struct house House;

int count_plugs(House *);
int count_houses(House *);
int read_group_of_lines (char ***, FILE **, int);
void elaborate_stdout(House *, House **, Household **, Plug **, char *);
void reach_house(House *, House **, Household **, Plug **, int);
void reach_household(Household **, Plug **, int);
void reach_plug(Plug **, int);
void insert_data(Plug *, long, int);
void find_date(long, int *, int *);
void create_initial_structure(char *, House **);
void elaborate_stderr(House **, House **, Household **, Plug **, char *);
void insert_house(House **, House **, char *);
void insert_household(House *, Household **, char *);
void insert_plug(House *, Household *, Plug **, char *);
void print_houses(House *);
void print_house(House *);
void print_household(Household *);
void print_plug(Plug *);
void print_measurement(Measurement *my_mes);
int extract_id(char *, char*);
char *remove_substring(char *, char *);
int is_a_house(char *);
int is_a_household(char *);
int is_a_plug(char *);
int count_lines_stdout(char *);
FILE *open_file(char *);
void to_lowercase(char *);
char *purgeInfo(char *);
int split(char *, char, char ***);
void free_tokens(char ***, int);
char *str_replace(char *, char *, char *);
int startsWith(char *, char *);
//Communication part
int send_message(char *, int, int);
int send_int_message(int, int, int);
int send_long_message(long, int, int);
int send_float_message(float, int, int);
int send_initial_message(int, int, long, int, int);
int receive_message(char **, int, int);
int receive_int_message(int *, int, int);
int receive_long_message(long *, int, int);
int receive_float_message(float *, int, int);
int receive_initial_message(int *, int *, long *, int, int);
void send_house(int, House *, char **, int, int, int);
void send_plug(int, House *, char **, int, int);
House *find_house(House *, int);
int find_plug_before(House *, int);
void receive_house(int);
void receive_plug(int **, int *, int);
int min(int, int);
int get_plug_value(char *);
float calculate_median_load(int *, int);
void assign_median_load(float, int, int, int, House *);

int main(int argc, char *argv[]) {
	int rank, num_processes;
	int i, j, hour_iteration = 0;
	double percentage = 0;
	long lines_stdout = 0;
	long lines_left = 0;
	long lines_read = 0;
	int total_num_plugs = 0;
	int total_num_houses = 0;
	int num_ts = 0;
	int num_thread = -1;
	int *plug_values;
	float *median_load;
	FILE *my_stdout = NULL;
	House *start_house = NULL;
	House *last_house = NULL;
	Household *last_household = NULL;
	Plug *last_plug = NULL;
	char **group_lines;
	clock_t t_start, t_end;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	plug_values = (int *) malloc (sizeof (int) * BLOCK);
	median_load = (float *) malloc (sizeof(float) * num_processes);
	t_start = clock();

	//What the master does
	if(rank == 0){
		//Create the initial structure reading from stderr (PATH_STDERR)
		create_initial_structure(PATH_STDERR, &start_house);
		printf("Created structure\n");
		//Count lines of stdout reading from LINES_STDOUT
		lines_stdout = count_lines_stdout(LINES_STDOUT);
		lines_left = lines_stdout;
		total_num_houses = count_houses(start_house);
		total_num_plugs = count_plugs(start_house);
		printf("MASTER\tLines: %ld\tHouses: %d\tPlugs: %d\n", lines_left, total_num_houses, total_num_plugs);
		//Here send the initial informations
		#pragma omp parallel num_threads(num_processes)
		{
			num_thread = omp_get_thread_num();
			if(num_thread != 0){
				if(send_initial_message(total_num_houses, total_num_plugs, lines_left, num_thread, num_thread) < 1){
					printf("ERROR\tImpossible send initial message\n");
				}
			}
		}
		printf("MASTER\tInformation sent to other processes\n");

		my_stdout = open_file(PATH_STDOUT);
		while(lines_left > 0){
			lines_read = read_group_of_lines (&group_lines, &my_stdout, min (total_num_plugs*BLOCK, lines_left));
			lines_left -= lines_read;
			num_ts = lines_read/total_num_plugs;

			//Complete computation if I'm alone
			if(num_processes == 1){
				//If master is the only one running, I execute everything alone
				last_house = start_house;
				last_household = start_house->h_households;
				last_plug = start_house->h_households->hh_plugs;
				for (i = 0; i < lines_read; i++) {
					percentage += 1;
					elaborate_stdout(start_house, &last_house, &last_household, &last_plug,
						group_lines[i]);
					if (((int) percentage) % 100000 == 0) {
						printf("Execution at:\t%.2f%%\n",
							(percentage / lines_stdout) * 100);
					}
				}
			} else {
			//Here is what MASTER does if there are slaves
				if(SEND_ALL_HOUSE){
					//TODO Doesn't calculate anything
					for (i = 0; i < total_num_houses; i += num_processes){
						#pragma omp parallel num_threads(num_processes)
						{
							num_thread = omp_get_thread_num();
							send_house(i+j, start_house, group_lines, j, num_ts, num_thread);
						}
					}
				} else {
				//In this case I send only a single plug
					for (i = 0; i < total_num_plugs; i += num_processes){
						#pragma omp parallel num_threads(num_processes) private(num_thread)
						{
							num_thread = omp_get_thread_num();
							if (num_thread == 0){
								//I organize the matrix plug_values
								for(j = 0; j < num_ts; j++){
									plug_values[j] = get_plug_value( group_lines[i + (j * total_num_plugs)] );
								}
								median_load[num_thread] = calculate_median_load(plug_values, num_ts);
							} else {
								if(i + num_thread < total_num_plugs){
									//Send data to elaborate
									printf("Master\tElaborate plug #%d\n", i+num_thread);
									send_plug(i+num_thread, start_house, group_lines, num_thread, num_ts);
									//Receive answer
									receive_float_message(&(median_load[num_thread]), num_thread, num_thread);
									printf("Master\tReceived the following median value: %f\n", median_load[num_thread]);
								}
							}
							//Saving elaborated data
							assign_median_load(median_load[num_thread], i+num_thread, hour_iteration, num_ts, start_house);
						}
					}
				}

				//Send the update about remaining lines to read
				#pragma omp parallel num_threads(num_processes)
				{
					num_thread = omp_get_thread_num();
					send_long_message(lines_left, num_thread, num_thread);
				}
			}
			hour_iteration ++;
			free_tokens(&group_lines, lines_read);
		}
	} else {
		//That's what the SLAVE does
		//Receive initial information
		printf("Slave %d\tWaiting initial message\n", rank);
		receive_initial_message(&total_num_houses, &total_num_plugs, &lines_left, 0, rank);

		printf("Slave %d\t#House: %d - #Plug: %d - #Lines: %ld\n", rank, total_num_houses, total_num_plugs, lines_left);

		while(lines_left > 0){
			if(SEND_ALL_HOUSE){
				for (i = rank; i < total_num_houses; i += num_processes){
					receive_house(rank);
					printf("Slave %d\tReceived house: %d\n", rank, i);
				}
			} else {
				//In this case I receive a plug per time
				for (i = rank; i < total_num_plugs; i += num_processes){
					receive_plug (&plug_values, &num_ts, rank);
					median_load[rank] = calculate_median_load(plug_values, num_ts);
					printf("Slave %d\tCalculated the following ML: %f\n", rank, median_load[rank]);
					send_float_message(median_load[rank], 0, rank);
				}
			}
			receive_long_message(&lines_left, 0, rank);
			printf("Slave %d\t#lines left: %ld\n", rank, lines_left);
		}
		printf("Slave %d\tFinished my job, finally free!\n", rank);
	}
	
	t_end = clock();
	printf("Terminated execution\nElapsed time: %ld ms\n",
				(1000 * (t_end - t_start) / (CLOCKS_PER_SEC)));
	
	free(plug_values);
	MPI_Finalize();
	return 0;
}

int count_plugs(House *start_house){
	int count = 0;
	House *temp = start_house;
	while (temp != NULL){
		count += temp->num_plug;
		temp = temp->next;
	}
	return count;
}

int count_houses(House *start_house){
	int count = 0;
	House *temp = start_house;
	while (temp != NULL){
		count ++;
		temp = temp->next;
	}
	return count;
}

/*
 * Read from the stdout a number of lines smaller or equal to one hour of measurement
 *
 * group_to_fill	->	Array of string where all the interested string will be loaded.
 * my_stdout		->	Pointer to the strea, where string will be read
 * num_of_lines		->	Number of lines to read
 *
 * return			->	Number of lines effectively read
 */
int read_group_of_lines (char ***group_to_fill, FILE **my_stdout, int num_of_lines){
	char *read_line = NULL;
	int other_lines;
	size_t len;
	int i;

	*group_to_fill = (char**) malloc(sizeof(char*) * num_of_lines);
	for(i = 0; i < num_of_lines; i++){
		(*group_to_fill)[i] = NULL;
		len = 0;
		other_lines = getline(&read_line, &len, *my_stdout);
		if(other_lines == -1 || read_line == NULL){
//			printf("Exit before saving value #%d\n", i);
			break;
		}
		(*group_to_fill)[i] = (char *) malloc ((strlen(read_line)+1) * sizeof(char));
		strcpy((*group_to_fill)[i], read_line);
		free(read_line);
		read_line = NULL;
	}
	if(num_of_lines != i){
		printf("Attention:	#expected lines: %d\t#read lines: %d\n", num_of_lines, i);
	}
	return i;
}

/*
 * Recognize the string read from stdout and update the structure
 */
void elaborate_stdout(House *start_h, House **last_h, Household **last_hh,
		Plug **last_p, char *my_string) {
	char **token = NULL;
	int num_token_stdout = 0;
	int id_house = -1, id_household = -1, id_plug = -1, value = -1;
	long timestamp = 0;

	//Split the string - No need to be cleared
	num_token_stdout = split(my_string, ',', &token);
	if (num_token_stdout != 6) {
		printf("Error splitting data in this line: [%s]\nTerminate execution\n",
				my_string);
		exit(1);
	}

	/*
	 * Description of structure:
	 * token[0]	->	id_measurement
	 * token[1]	->	timestamp measurement
	 * token[2]	->	house id
	 * token[3]	->	household id
	 * token[4]	->	plug id
	 * token[5]	->	value of measurement
	 */
	//Transform string to int
	timestamp = atol(token[1]);
	id_house = atoi(token[2]);
	id_household = atoi(token[3]);
	id_plug = atoi(token[4]);
	value = atoi(token[5]);

	//Update references before update data
	reach_house(start_h, last_h, last_hh, last_p, id_house);
	reach_household(last_hh, last_p, id_household);
	reach_plug(last_p, id_plug);

	//Update data
	insert_data(*last_p, timestamp, value);

	//Free memory
	free_tokens(&token, num_token_stdout);
}

void reach_house(House *start_h, House **last_h, Household **last_hh,
		Plug **last_p, int id) {
	if ((*last_h)->id == id) {
		//It's the current house, we can exit
		return;
	} else if ((*last_h)->id < id) {
		//We go to the next house - Update all references, not only the ref of the house
		*last_h = (*last_h)->next;
		*last_hh = (*last_h)->h_households;
		*last_p = (*last_h)->h_households->hh_plugs;
		reach_house(start_h, last_h, last_hh, last_p, id);
//		printf("Updated House\n");
		return;
	} else {
		//New set of measurements - restart from House 0
		*last_h = start_h;
		*last_hh = start_h->h_households;
		*last_p = start_h->h_households->hh_plugs;
		reach_house(start_h, last_h, last_hh, last_p, id);
	}
}

void reach_household(Household **last_hh, Plug **last_p, int id) {
	if ((*last_hh)->id == id) {
		//It's the current household, we can exit
		return;
	} else if ((*last_hh)->id < id) {
		//We go to the next household - We update the ref also of the plug
		*last_hh = (*last_hh)->next;
		*last_p = (*last_hh)->hh_plugs;
		reach_household(last_hh, last_p, id);
//		printf("Updated Household\n");
		return;
	} else {
		printf("Something went wrong: household_id: %d\nTerminate\n", id);
		exit(1);
	}
}

void reach_plug(Plug **last_p, int id) {
	if ((*last_p)->id == id) {
		//It's the current plug, we can exit
		return;
	} else if ((*last_p)->id < id) {
		//We go to the next plug
		*last_p = (*last_p)->next;
		reach_plug(last_p, id);
//		printf("Updated Plug\n");
		return;
	} else {
		printf("Something went wrong: plug_id: %d\nTerminate\n", id);
		exit(1);
	}
}

void insert_data(Plug *last_p, long timestamp, int value_measurement) {
	int date = 0;
	int hour = 0;
	int i = 0;
	Measurement *new_measurement;

	find_date(timestamp, &date, &hour);
	if (hour < 0 || hour > 23) {
		printf("Something wrong - Hour is: %d\n Terminate execution\n", hour);
		exit(1);
	}

	//Check if it has any measurement
	if (last_p->last_measurement != NULL) {
		//Check if the last measurement has the same date of this
		if (last_p->last_measurement->date == date) {
			last_p->last_measurement->hour[hour] += value_measurement;
			return;
		}
	}

	//If I reach this point I've to add a new measurement
	new_measurement = (Measurement *) malloc(sizeof(Measurement));
	if (new_measurement == NULL) {
		printf(
				"Not enough memory to complete the computation\nTerminate execution\n");
		exit(1);
	}
	//Inizialize values
	for (i = 0; i < 24; i++) {
		new_measurement->hour[i] = 0;
		;
	}
	//Insert read value
	new_measurement->date = date;
	new_measurement->hour[hour] += value_measurement;
	new_measurement->next = NULL;

	//Properly append the structure I have created
	if (last_p->last_measurement != NULL) {
		last_p->last_measurement->next = new_measurement;
	} else {
		last_p->my_measurements = new_measurement;
	}
	last_p->last_measurement = new_measurement;
}

/*
 * Extract the date and the hour form the timestamp
 * timestamp	->	The time as a long
 * date			->	Pointer to the variable to fill with the right hourdate
 * hour			->	Pointer to the variable to fill with the right hour
 */
void find_date(long timestamp, int *date, int *hour) {
	int num_token = 0;
	char buf[20];
	char **tokens = NULL;
	struct tm *timeinfo;
	time_t time = timestamp;

	timeinfo = localtime(&time);
	if (timeinfo == NULL) {
		printf("Cannot convert to struct tm: %ld\nTerminate execution\n",
				timestamp);
		exit(1);
	}
	strftime(buf, sizeof(buf), "%Y%m%d,%H", timeinfo);

	num_token = split(buf, ',', &tokens);
	if (num_token != 2) {
		printf("Error in the split of %s\nTerminate execution\n", buf);
		exit(1);
	}

	*date = atoi(tokens[0]);
	*hour = atoi(tokens[1]);

//	printf("date: %d\thour: %d\n", *date, *hour);
	free_tokens(&tokens, num_token);
}

/*
 * Creates the initial structure. This structure will be used to save data read by stdout.
 *
 * stream_stderr	->	Where to read data to build up the structure
 * start_house		->	Inside will be saved the reference to the first House.
 *
 */
void create_initial_structure(char *stderr_filepath, House **start_house) {
	FILE *stream_stderr = NULL;
	House *last_house = NULL;
	Household *last_household = NULL;
	Plug *last_plug = NULL;
	char *string_stderr = NULL;
	size_t len;

	//Open stderr
	stream_stderr = open_file(stderr_filepath);
	while (getline(&string_stderr, &len, stream_stderr) != -1) {
		elaborate_stderr(start_house, &last_house, &last_household, &last_plug,
				string_stderr);
		free(string_stderr);
		string_stderr = NULL;
	}
	printf("Finished to create the world\n");

}

/*
 * Recognize the string and updates the structure
 */
void elaborate_stderr(House **start_h, House **last_h, Household **last_hh,
		Plug **last_p, char *my_string) {
	int i;
	char **tokens_stderr = NULL;
	char *purged_string = NULL;
	int num_token_stderr = 0;
	//Now I purge and tokenize what I've read
	purged_string = purgeInfo(my_string);
	num_token_stderr = split(purged_string, ',', &tokens_stderr);
	for (i = 0; i < num_token_stderr; i++) {
		if (is_a_house(tokens_stderr[i])) {
			insert_house(start_h, last_h, tokens_stderr[i]);
		} else if (is_a_household(tokens_stderr[i])) {
			insert_household(*last_h, last_hh, tokens_stderr[i]);
		} else if (is_a_plug(tokens_stderr[i])) {
			insert_plug(*last_h, *last_hh, last_p, tokens_stderr[i]);
		} else {
			printf("Cannot recognize this string:\n[%s]\n", tokens_stderr[i]);
			exit(0);
		}
	}
//	printf("Finished to create an house\n");
}

// Inserts a house inside the initial structure
void insert_house(House **start_house, House **last_appended, char *string_id) {
	//Declaration and allocation
	House *h;
	h = (House *) malloc(sizeof(House));
	int id = -1;

	//Getting id
	id = extract_id(string_id, HOUSE);
	if (id == -1) {
		printf("Cannot find a valid ID");
		exit(1);
	}

	//Security checks
	if (h == NULL) {
		printf("Not enough memory to allocate a new House!\n");
		exit(0);
	}

	//Initialization
	h->id = id;
	h->h_households = NULL;
	h->num_plug = 0;
	h->next = NULL;

	//Appending to the end of the list
	if (*start_house == NULL) {
		//First House of the list
		*start_house = h;
	} else {
		//The last element of the list points to the new element
		(*last_appended)->next = h;
	}
	//Updating the reference to the last element
	*last_appended = h;
}

// Inserts a household inside the initial structure
void insert_household(House *my_house, Household **last_appended_household,
		char *string_id) {
	//Declaration and allocation
	Household *hh;
	hh = (Household *) malloc(sizeof(Household));
	int id = -1;

	//Getting id
	id = extract_id(string_id, HOUSEHOLD);
	if (id == -1) {
		printf("Cannot find a valid ID");
		exit(1);
	}

	//Security checks
	if (hh == NULL) {
		printf("Not enough memory to allocate a new Household!\n");
		exit(0);
	}
	//Insertion of an household before the creation of an house - Something wrong
	if (my_house == NULL) {
		printf("Error: No House - Cannot insert a new Household\n");
		exit(0);
	}

	//Initialization
	hh->id = id;
	hh->hh_plugs = NULL;
	hh->next = NULL;

	//Updating references
	if (my_house->h_households == NULL) {
		//Last household refers to an other House
		my_house->h_households = hh;
	} else {
		//Normally append to the last household
		(*last_appended_household)->next = hh;
	}
	//Update the reference to the last household
	*last_appended_household = hh;
}

// Inserts a plug inside the initial structure
void insert_plug(House *my_house, Household *my_household,
		Plug **last_appended_plug, char *string_id) {
	//Declaration and allocation
	Plug *p;
	p = (Plug *) malloc(sizeof(Plug));
	int id = -1;

	//Getting id
	id = extract_id(string_id, PLUG);
	if (id == -1) {
		printf("Cannot find a valid ID");
		exit(1);
	}
	//Security Checks
	if (p == NULL) {
		printf("Not enough memory to allocate a new Plug!\n");
		exit(0);
	}
	//Insertion of a plug before the creation of an household - something wrong
	if (my_household == NULL) {
		printf("Error: No Household - Cannot insert a new Plug\n");
		exit(0);
	}

	//Initialization
	p->id = id;
	p->next = NULL;
	p->my_measurements = NULL;
	p->last_measurement = NULL;

	//Updating references
	if (my_household->hh_plugs == NULL) {
		//Last Plug references to an other household
		my_household->hh_plugs = p;
	} else {
		//Normally append to the last plug
		(*last_appended_plug)->next = p;
	}
	//Update the reference to the last Plug
	*last_appended_plug = p;

	//Update the house count
	my_house->num_plug += 1;
}

/*
 * Print all the houses
 */
void print_houses(House *start_house) {
	printf("Start printing...\n");
	while (start_house != NULL) {
		print_house(start_house);
		start_house = start_house->next;
	}
	printf("Printing completed\n");
}

/*
 * Prints just the passed house.
 */
void print_house(House *my_house) {
	printf("\nHouse id: %d", my_house->id);
	Household *temp = my_house->h_households;
	while (temp != NULL) {
		print_household(temp);
		temp = temp->next;
	}
}

void print_household(Household *my_household) {
	printf("\n\tHousehold id: %d\n\t", my_household->id);
	Plug *temp = my_household->hh_plugs;
	while (temp != NULL) {
		print_plug(temp);
		temp = temp->next;
	}
}

void print_plug(Plug *my_plug) {
	printf("\n\t\tPlug id: %d", my_plug->id);
	Measurement *temp = my_plug->my_measurements;
	while (temp != NULL) {
		print_measurement(temp);
		temp = temp->next;
	}
}

void print_measurement(Measurement *my_mes) {
	Measurement * next = my_mes->next;
	int i = 0;
	printf("\n\t\tDate: %d\n\t\t", my_mes->date);
	for (i = 0; i < 24; i++) {
		printf("H%d: %f\t", i, my_mes->hour[i]);
	}
	if (next != NULL) {
		print_measurement(next);
	}
}

/*
 * Extracts the id from the string
 */
int extract_id(char *full_string, char *to_remove) {
	int id = -1;
	char *extracted_id;
	//Now I remove to remove from full string
	extracted_id = remove_substring(full_string, to_remove);

//	printf("Extracted_id: [%s]\tOriginal string is: [%s]\n", extracted_id, full_string);
	id = atoi(extracted_id);
	if (id < 0) {
		return -1;
	}
	return id;
}

char *remove_substring(char *full_string, char * to_remove) {
	int i = 0, j = 0;
	char *extracted_id;
	extracted_id = (char *) malloc(
			sizeof((full_string[0]) * strlen(full_string)));
	for (i = 0; i < strlen(to_remove); i++) {
		if (full_string[i] == to_remove[i]) {
			//Ok - continue counting
		} else {
			printf(
					"Second string isn't a substring of the first one\nTerminate execution\n");
			exit(1);
		}
	}
	j = i;
	for (i = 0; i + j < strlen(full_string); i++) {
		if (full_string[i + j] != ' ' && full_string[i + j] != '\n')
			extracted_id[i] = full_string[i + j];
	}
	strcpy(full_string, extracted_id);
	free(extracted_id);
	return full_string;
}

int is_a_house(char *my_string) {
	to_lowercase(my_string);
	return startsWith(HOUSE, my_string);
}

int is_a_household(char *my_string) {
	to_lowercase(my_string);
	return startsWith(HOUSEHOLD, my_string);
}

int is_a_plug(char *my_string) {
	to_lowercase(my_string);
	return startsWith(PLUG, my_string);
}

/*
 * Count lines of stdout
 */
int count_lines_stdout(char *file) {
	FILE *file_lines_stdout = NULL;
	int lines = 0, temp = 0;
	char *temp_line;
	size_t temp2;

	file_lines_stdout = open_file(file);
	temp = getline(&temp_line, &temp2, file_lines_stdout);
	if (temp == -1) {
		printf("Cannot read anything\nTerminate execution\n");
		exit(1);
	}
	lines = atoi(temp_line);
	if (lines < 1) {
		printf("Error converting line\nTerminate execution\n");
		exit(1);
	}
	return lines;
}

/*
 * Gives the reference to the opened file.
 */
FILE *open_file(char *my_filepath) {
	FILE *stream_stderr = NULL;
	stream_stderr = fopen(my_filepath, "r");
	if (stream_stderr == NULL) {
		printf("Cannot read from: [%s]\nTerminate execution\n", my_filepath);
		exit(0);
	}
	return stream_stderr;
}

void to_lowercase(char *my_string) {
	int i;
	char x;
	for (i = 0; my_string[i]; i++) {
		x = tolower(my_string[i]);
		my_string[i] = x;
	}
}

/*
 * Modify the original string in order to manage that as a string coming from stdout
 *
 * NB: Remember to free the return value!
 */
char *purgeInfo(char info[]) {
	char *result = info;
	char *old;
	//Each time I free the memory dynamically allocated when I don't use it anymore
	result = str_replace(result, ", }, }", "");
	old = result;
	result = str_replace(result, "}, ", "");
	free(old);
	old = result;
	result = str_replace(result, "}", "");
	free(old);
	old = result;
	result = str_replace(result, ", ", ",");
	free(old);
	old = result;
	result = str_replace(result, " {", ",");
	free(old);
	return result;
}

/*
 * str 		-> 	Original string to split (not modified)
 * delimiter->	The delimiters
 * tokens	->	All the parts of the string splitted in an array.
 *
 * returns 	->	The number of tokens
 */
int split(char *str, char delimiter, char ***tokens) {
	int count = 1;
	int token_len = 1;
	int i = 0;
	char *p;
	char *t;

	p = str;
	while (*p != '\0') {
		if (*p == delimiter) {
			count++;
		}
		p++;
	}

	*tokens = (char**) malloc(sizeof(char*) * count);
	if (*tokens == NULL) {
		exit(1);
	}

	p = str;
	while (*p != '\0') {
		if (*p == delimiter) {
			(*tokens)[i] = (char*) malloc(sizeof(char) * token_len);
			if ((*tokens)[i] == NULL) {
				exit(1);
			}
			token_len = 0;
			i++;
		}
		p++;
		token_len++;
	}
	(*tokens)[i] = (char*) malloc(sizeof(char) * token_len);
	if ((*tokens)[i] == NULL) {
		exit(1);
	}

	i = 0;
	p = str;
	t = ((*tokens)[i]);
	while (*p != '\0') {
		if (*p != delimiter && *p != '\0') {
			*t = *p;
			t++;
		} else {
			*t = '\0';
			i++;
			t = ((*tokens)[i]);
		}
		p++;
	}

	return count;
}

/*
 * This function frees properly the memory used by function split
 */
void free_tokens(char ***token, int num_token_stdout) {
	int i = 0;
	for (i = 0; i < num_token_stdout; i++) {
		free((*token)[i]);
	}
	free(*token);
}

/*
 * orig		->	Original string
 * rep		->	Token to find
 * with		->	Token to substitute
 *
 * returns	->	The replaced string NB. Must call the function free to free memory.
 */
char *str_replace(char *orig, char *rep, char *with) {
	char *result; // the return string
	char *ins;	// the next insert point
	char *tmp;	// varies
	int len_rep;  // length of rep
	int len_with; // length of with
	int len_front; // distance between rep and end of last rep
	int count;	// number of replacements

	if (!orig)
		return NULL;
	if (!rep)
		rep = "";
	len_rep = strlen(rep);
	if (!with)
		with = "";
	len_with = strlen(with);

	ins = orig;
	for (count = 0; (tmp = strstr(ins, rep)); ++count) {
		ins = tmp + len_rep;
	}

	// first time through the loop, all the variable are set correctly
	// from here on,
	//	tmp points to the end of the result string
	//	ins points to the next occurrence of rep in orig
	//	orig points to the remainder of orig after "end of rep"
	tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

	if (!result)
		return NULL;

	while (count--) {
		ins = strstr(orig, rep);
		len_front = ins - orig;
		tmp = strncpy(tmp, orig, len_front) + len_front;
		tmp = strcpy(tmp, with) + len_with;
		orig += len_front + len_rep; // move to next "end of rep"
	}
	strcpy(tmp, orig);
	return result;
}

/*
 * pre	->	Characters to find at the start of str
 * str	->	The full string
 *
 * returns	->	0 if NOT EQUAL
 * 				any number if str starts with pre
 */
int startsWith(char *pre, char *str) {
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

int send_message(char *my_message, int dest, int tag){
	int i;
	int len = (int)strlen(my_message);
	if (len < 1){
		printf("Error\tTerminate Execution\n");
		return 0;
	}
	MPI_Send(&len, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
	for(i=0; i<len; i++){
		MPI_Send(&(my_message[i]), tag, MPI_CHAR, dest, 1, MPI_COMM_WORLD);
	}
	return 1;
}

int send_int_message(int num, int dest, int tag){
	MPI_Send(&num, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
	return 1;
}

int send_long_message(long num, int dest, int tag){
	MPI_Send(&num, 1, MPI_LONG, dest, tag, MPI_COMM_WORLD);
	return 1;
}

int send_float_message(float num, int dest, int tag){
	MPI_Send(&num, 1, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
	return 1;
}

int receive_message(char **my_message, int source, int tag){
	MPI_Status stat;
	char test = 'q';
	int len = -1;
	int i;
	MPI_Recv(&len, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);
	if (len < 1){
		printf("ERROR - lenght of the message to receive is: %d\tTerminate Execution\n", len);
		return 0;
	}
	(*my_message) = (char *) malloc (sizeof(char)*(len+1));
	for(i=0; i<len; i++){
	MPI_Recv(&test, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);
		(*my_message)[i] = test;
	}
	(*my_message)[len] = '\0';
	return 1;
}

int receive_int_message(int *num, int source, int tag){
		MPI_Status stat;
		*num = -1;
		MPI_Recv(num, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &stat);
		if (*num < 0){
			printf("ERROR - received int is: %d\tTerminate Execution\n", *num);
			return 0;
		}
	return 1;
}

int receive_long_message(long *num, int source, int tag){
		MPI_Status stat;
		*num = -1;
		MPI_Recv(num, 1, MPI_LONG, source, tag, MPI_COMM_WORLD, &stat);
		if (*num < 0){
			printf("ERROR - received long is: %ld\tTerminate Execution\n", *num);
		return 0;
		}
	return 1;
}

int receive_float_message(float *num, int source, int tag){
		MPI_Status stat;
		*num = -1;
		MPI_Recv(num, 1, MPI_FLOAT, source, tag, MPI_COMM_WORLD, &stat);
		if (*num < 0){
			printf("ERROR - received float is: %f\tTerminate Execution\n", *num);
		return 0;
		}
	return 1;
}

/*
 * THREAD SAFE
 * Send the information about a single plug
 */
void send_plug(int plug_before, House *start_house, char **group_lines, int dest, int num_ts){
	int i, line_to_send;
	int total_plug;
	int value;
	total_plug = count_plugs(start_house);
	if (plug_before >= total_plug){
		//If there are X total plug, there cannot be X or more plug before the one I'm sending
		return;
	}

	//Send the information on how many measurements will be sent
	send_int_message(num_ts, dest, dest);
	for(i = 0; i < num_ts; i++){
		line_to_send = plug_before + i*total_plug;
		value = get_plug_value( group_lines[line_to_send] );
		send_int_message (value, dest, dest);
	}
}

//Here I send to the client all the information about his house
void send_house(int house_id, House *start_house, char **group_lines, int dest, int num_ts, int tag){
	int i, j, line_to_send;
	int plug_before, total_plug;
	House *my_house;
	
	if (house_id > count_houses(start_house)){
		//I don't do anything - I have no houses to analyze
		return;
	}
	
	my_house = find_house(start_house, house_id);
	plug_before = find_plug_before(start_house, house_id);
	total_plug = count_plugs(start_house);
	//Send the information on how many houses will be sent
	send_int_message(num_ts*(my_house->num_plug), dest, tag);

	//Cycle to read all timestamps - A number equal to the value of BLOCK
	for(i = 0; i < num_ts; i++){
		//Cycle on all my plugs - Starting from the number of plugs before mine until I don't visit all my plugs. At the end I skip to the next block
		for(j = 0; j < my_house->num_plug; j++){
			//plug before is the initial offset
			line_to_send = plug_before + j + (i * total_plug);
			send_message(group_lines[line_to_send], dest, tag);
		}
	}
}

House *find_house(House *starting_house, int id){
	House *my_house = starting_house;
	while (my_house->id != id){
		if(my_house->next == NULL){
			printf("Fatal error\tHouse not found with id: %d\n", id);
			return NULL;
		}
		my_house = my_house->next;
	}
	return my_house;
}

int find_plug_before(House *start_house, int house_id){
	int num = 0;
	House *my_house = start_house;
	while (my_house->id != house_id){
		num += my_house->num_plug;
		if(my_house->next == NULL){
			printf("Error\tID: %d not found\n", house_id);
			return -1;
		}
		my_house = my_house->next;
	}
	return num;
}

void receive_house(int tag){
	//TODO Actually this function isn't used
	int i, house_to_receive;
	char *my_string = NULL;
	receive_int_message(&house_to_receive, 0, tag);
	for(i = 0; i < house_to_receive; i++){
		receive_message(&my_string, 0, tag);
		//printf("Slave\tString:[%s]\n", my_string);
		free(my_string);
		my_string = NULL;
	}

	printf("Received %d messages\n", i);
}

/*
 * Send all the initial messages to a process
 */
int send_initial_message(int n_houses, int n_plugs, long n_lines, int destination, int tag){
	int i = -2;
	i += send_int_message(n_houses, destination, tag);
	i += send_int_message(n_plugs, destination, tag);
	i += send_long_message(n_lines, destination, tag);
	return i;
}

/*
 * Receive all initial message from a process
 */
int receive_initial_message(int *n_houses, int *n_plugs, long *n_lines, int source, int tag){
	int i = -2;
	i += receive_int_message(n_houses, source, tag);
	i += receive_int_message(n_plugs, source, tag);
	i += receive_long_message(n_lines, source, tag);
	return i;
}

int min(int a, int b){
	return a<b ? a : b;
}

void receive_plug(int ** int_to_evaluate, int *num_ts, int tag){
	int i;
	receive_int_message(num_ts, 0, tag);
	for(i = 0; i < *num_ts; i++){
		receive_int_message ( &((*int_to_evaluate)[i]), 0, tag);
	}
}

/*
 * Given a string read from the stdout return the value
 *
 * string	->	String read from stdout
 *
 * return	->	Converted value read from the string
 */
int get_plug_value(char *string){
	char **splitted_list;
	int value = -1;
	split(string, ',', &splitted_list);
	value = atoi(splitted_list[5]);
	return value;
}

/*
 * Given all the values, it calculates the median value
 *
 * plug_values	->	Array containing all values
 * num_ts		->	Number of samples
 */
float calculate_median_load(int *plug_values, int num_ts){
	float my_val = 0;
	long sum = 0;
	int i;
	for (i = 0; i < num_ts; i++){
		sum += plug_values[i];
	}
	my_val = ((float) sum / num_ts);
	return my_val;
}

/*
 * Finds the appropriate field and fill it with the median load of a certain plug in a certain hour
 *
 * value			->	The calculated median load
 * n_plug_before	->	Number of plug before this current one
 * hour_number		->	The hour of the measurement
 * num_ts			->	Number of samples
 * start_house		->	Reference to the house
 */
void assign_median_load(float value, int n_plug_before, int hour_number, int num_ts, House *start_house){
	House *my_house = start_house;
	Household *my_household;
	Plug *my_plug;
	Measurement *my_measurement;
	int date;
	//Find the right house
	while(n_plug_before >= my_house->num_plug){
		n_plug_before -= my_house->num_plug;
		my_house = my_house->next;
	}
	//Find the right plug
	my_household = my_house->h_households;
	my_plug = my_household->hh_plugs;
	while(n_plug_before == 0){
		if(my_plug->next == NULL){
			my_household = my_household->next;
			my_plug = my_household->hh_plugs;
		} else {
			my_plug = my_plug->next;
		}
		n_plug_before--;
	}
	//Find the right measuration time
	hour_number = hour_number % 24;
	if((hour_number == 0)){
		my_measurement = (Measurement *) malloc(sizeof(Measurement));
		if(my_measurement == NULL){
			printf("Error creation measurement\nSystem run out of memory\n");
		}
		if(my_plug->last_measurement == NULL){
			//That's the first measurement
			my_plug->my_measurements = my_measurement;
			date = 0;
		} else {
			//We append the measurement to the last one
			my_plug->last_measurement->next = my_measurement;
			date = 1 + my_plug->last_measurement->date;
		}
		my_plug->last_measurement = my_measurement;
	} else {
		my_measurement = my_plug->last_measurement;
	}
	//Insert the data
	my_measurement->date = date;
	my_measurement->hour[hour_number] = value;
	my_measurement->num_ts[hour_number] = num_ts;
}
