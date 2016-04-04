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

#define PATH_STDERR "/home/timmy/Scrivania/stderr-dataGenerator.txt"
#define PATH_STDOUT "/home/timmy/Scrivania/stdout-dataGenerator.txt"
#define LINES_STDOUT "/home/timmy/Scrivania/stdout-lines.txt"
#define HOUSE "house "
#define HOUSEHOLD "household "
#define PLUG "plug "

struct measurement{
	int date;
	int hour[24];
	struct measurement *next;
};
typedef struct measurement Measurement;

//Definition of plug
struct plug{
	int id;
	int valore;
	Measurement *my_measurements;
	struct plug *next;
};
typedef struct plug Plug;

//Definition of household
struct household{
	int id;
	Plug *hh_plugs;
	struct household *next;
};
typedef struct household Household;

//Definition of house
struct house{
	int id;
	Household *h_households;
	struct house *next;
};
typedef struct house House;

void elaborate_stdout(House *, House **, Household **, Plug **, char *);
void reach_house(House *, House **, Household **, Plug **, int);
void reach_household(Household **, Plug **, int);
void reach_plug(Plug **, int);
void insert_data(Plug *, long, int);
void create_initial_structure(char *, House **);
void elaborate_stderr(House **, House **, Household **, Plug **, char *);
void insert_house (House **, House **, char *);
void insert_household (House *, Household **, char *);
void insert_plug (Household *, Plug **, char *);
void print_houses (House *);
void print_house (House *);
int extract_id (char *, char*);
char *remove_substring(char *, char * );
int is_a_house (char *);
int is_a_household (char *);
int is_a_plug (char *);
int count_lines_stdout (char *);
FILE *open_file (char *);
void to_lowercase (char *);
char *purgeInfo (char info[]);
int split (char *str, char delimiter, char ***tokens);
char *str_replace (char *orig, char *rep, char *with) ;
int startsWith (char *pre, char *str);

int main(int argc, char *argv[]){
	int lines_stdout = 0;
	FILE *my_stdout = NULL;
	House *start_house = NULL;
	House *last_house = NULL;
	Household *last_household = NULL;
	Plug *last_plug = NULL;
	size_t len;
	char *string_stdout = NULL;

	//Create the initial structure reading from stderr (PATH_STDERR)
	create_initial_structure(PATH_STDERR, &start_house);

	//Count lines of stdout reading from LINES_STDOUT
	lines_stdout = count_lines_stdout(LINES_STDOUT);
	printf("Stdout has: %d lines\n", lines_stdout);

	//TODO I fill the memory doing this - Somewhere I don't free memory
	//Read from stdout
	last_house = start_house;
	last_household = start_house->h_households;
	last_plug = start_house->h_households->hh_plugs;
	my_stdout = open_file(PATH_STDOUT);
	while (getline(&string_stdout, &len, my_stdout) != -1) {
		elaborate_stdout(start_house, &last_house, &last_household, &last_plug, string_stdout);
	}
	printf("Finished to elaborate stdout\n");

	free(string_stdout);

	return 0;
}

/*
 * Recognize the string read from stdout and update the structure
 */
void elaborate_stdout(House *start_h, House **last_h, Household **last_hh, Plug **last_p, char *my_string){
	char **token = NULL;
	int num_token_stdout = 0;
	int id_house = -1, id_household = -1, id_plug = -1, value = -1;
	long timestamp = 0;

	//Split the string - No need to be cleared
	num_token_stdout = split (my_string, ',', &token);
	if(num_token_stdout != 6){
		printf("Error splitting data in this line: [%s]\nTerminate execution\n", my_string);
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

}

void reach_house(House *start_h, House **last_h, Household **last_hh, Plug **last_p, int id){
	if((*last_h)->id == id){
		//It's the current house, we can exit
		return;
	} else if((*last_h)->id < id){
		//We go to the next house - Update all references, not only the ref of the house
		*last_h = (*last_h)->next;
		*last_hh = (*last_h)->h_households;
		*last_p = (*last_h)->h_households->hh_plugs;
		reach_house(start_h, last_h, last_hh, last_p, id);
		printf("Updated House\n");
		return;
	} else {
		//New set of measurements - restart from House 0
		*last_h = start_h;
		*last_hh = start_h->h_households;
		*last_p = start_h->h_households->hh_plugs;
		reach_house(start_h, last_h, last_hh, last_p, id);
	}
}

void reach_household(Household **last_hh, Plug **last_p, int id){
	if((*last_hh)->id == id){
		//It's the current household, we can exit
		return;
	} else if((*last_hh)->id < id){
		//We go to the next household - We update the ref also of the plug
		*last_hh = (*last_hh)->next;
		*last_p = (*last_hh)->hh_plugs;
		reach_household(last_hh, last_p, id);
		printf("Updated Household\n");
		return;
	} else {
		printf("Something went wrong: household_id: %d\nTerminate\n", id);
		exit(1);
	}
}

void reach_plug(Plug **last_p, int id){
	if((*last_p)->id == id){
		//It's the current plug, we can exit
		return;
	} else if((*last_p)->id < id){
		//We go to the next plug
		*last_p = (*last_p)->next;
		reach_plug(last_p, id);
		printf("Updated Plug\n");
		return;
	} else {
		printf("Something went wrong: plug_id: %d\nTerminate\n", id);
		exit(1);
	}
}

//TODO Write how to update data
void insert_data(Plug *last_p, long timestamp, int value_measurement){

}

/*
 * Creates the initial structure. This structure will be used to save data read by stdout.
 *
 * stream_stderr	->	Where to read data to build up the structure
 * start_house		->	Inside will be saved the reference to the first House.
 *
 */
void create_initial_structure(char *stderr_filepath, House **start_house){
	FILE *stream_stderr = NULL;
	House *last_house = NULL;
	Household *last_household = NULL;
	Plug *last_plug = NULL;
	char *string_stderr = NULL;
	size_t len;

	//Open stderr
	stream_stderr = open_file(stderr_filepath);
	while (getline(&string_stderr, &len, stream_stderr) != -1) {
		elaborate_stderr(start_house, &last_house, &last_household, &last_plug, string_stderr);
	}
	printf("Finished to create the world\n");

	free(string_stderr);
}

/*
 * Recognize the string and updates the structure
 */
void elaborate_stderr(House **start_h, House **last_h, Household **last_hh, Plug **last_p, char *my_string){
	int i;
	char **tokens_stderr = NULL;
	char *purged_string = NULL;
	int num_token_stderr = 0;

	//Now I purge and tokenize what I've read
	purged_string = purgeInfo (my_string);
	num_token_stderr = split (purged_string, ',', &tokens_stderr);
	for(i = 0; i<num_token_stderr; i++){
		if (is_a_house (tokens_stderr[i] ) ){
			insert_house(start_h, last_h, tokens_stderr[i]);
		} else if (is_a_household (tokens_stderr[i] ) ){
			insert_household(*last_h, last_hh, tokens_stderr[i]);
		} else if (is_a_plug (tokens_stderr[i] ) ){
			insert_plug(*last_hh, last_p, tokens_stderr[i]);
		} else {
			printf("Cannot recognize this string:\n[%s]\n", tokens_stderr[i]);
			exit(0);
		}
	}
	printf("Finished to create an house\n");
}

// Inserts a house inside the initial structure
void insert_house(House **start_house, House **last_appended, char *string_id){
	//Declaration and allocation
	House *h;
	h = (House *) malloc (sizeof(House));
	int id = -1;

	//Getting id
	id = extract_id (string_id, HOUSE);
	if (id == -1 ){
		printf("Cannot find a valid ID");
		exit(1);
	}

	//Security checks
	if (h == NULL ){
		printf("Not enough memory to allocate a new House!\n");
		exit(0);
	}

	//Initialization
	h->id = id;
	h->h_households = NULL;
	h->next = NULL;

	//Appending to the end of the list
	if (*start_house == NULL ){
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
void insert_household(House *my_house, Household **last_appended_household, char *string_id){
	//Declaration and allocation
	Household *hh;
	hh = (Household *) malloc (sizeof(Household));
	int id = -1;

	//Getting id
	id = extract_id (string_id, HOUSEHOLD);
	if (id == -1 ){
		printf("Cannot find a valid ID");
		exit(1);
	}

	//Security checks
	if (hh == NULL ){
		printf("Not enough memory to allocate a new Household!\n");
		exit(0);
	}
	//Insertion of an household before the creation of an house - Something wrong
	if (my_house == NULL ){
		printf("Error: No House - Cannot insert a new Household\n");
		exit(0);
	}

	//Initialization
	hh->id = id;
	hh->hh_plugs = NULL;
	hh->next = NULL;

	//Updating references
	if (my_house->h_households == NULL ){
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
void insert_plug(Household *my_household, Plug **last_appended_plug, char *string_id){
	//Declaration and allocation
	Plug *p;
	p = (Plug *) malloc (sizeof(Plug));
	int id = -1;

	//Getting id
	id = extract_id (string_id, PLUG);
	if (id == -1 ){
		printf("Cannot find a valid ID");
		exit(1);
	}
	//Security Checks
	if (p == NULL ){
			printf("Not enough memory to allocate a new Plug!\n");
			exit(0);
	}
	//Insertion of a plug before the creation of an household - something wrong
	if (my_household == NULL ){
		printf("Error: No Household - Cannot insert a new Plug\n");
		exit(0);
	}

	//Initialization
	p->id = id;
	p->next = NULL;
	//TODO maybe to be removed and insert the structure measutements
	p->valore = 0;

	//Updating references
	if (my_household->hh_plugs == NULL ){
		//Last Plug references to an other household
		my_household->hh_plugs = p;
	} else {
		//Normally append to the last plug
		(*last_appended_plug)->next = p;
	}
	//Update the reference to the last Plug
	*last_appended_plug = p;
}

/*
 * Print all the houses
 */
void print_houses(House *start_house){
	printf("Start printing...\n");
	while (start_house != NULL ){
		print_house(start_house);
		start_house = start_house->next;
	}
	printf("Printing completed\n");
}

/*
 * Prints just the passed house.
 */
void print_house(House *my_house){
	printf("House id: %d\n", my_house->id);
}

/*
 * Extracts the id from the string
 */
int extract_id (char *full_string, char *to_remove){
	int id = -1;
	char *extracted_id;
	//Now I remove to remove from full string
	extracted_id = remove_substring(full_string, to_remove);

//	printf("Extracted_id: [%s]\tOriginal string is: [%s]\n", extracted_id, full_string);
	id = atoi (extracted_id);
	if (id < 0 ){
		return -1;
	}
	return id;
}

char *remove_substring(char *full_string, char * to_remove){
	int i = 0, j= 0;
	char *extracted_id;
	extracted_id = (char *) malloc (sizeof ((full_string[0])*strlen(full_string)));
	for(i=0; i < strlen(to_remove); i++){
		if(full_string[i] == to_remove[i]){
			//Ok - continue counting
		} else {
		printf("Second string isn't a substring of the first one\nTerminate execution\n");
			exit(1);
		}
	}
	j = i;
	for(i=0; i+j<strlen(full_string) ;i++){
		if(full_string[i+j] != ' ' && full_string[i+j] != '\n')
		extracted_id[i] = full_string[i+j];
	}
	return extracted_id;
}

int is_a_house(char *my_string){
	to_lowercase (my_string);
	return startsWith(HOUSE, my_string);
}

int is_a_household(char *my_string){
	to_lowercase (my_string);
	return startsWith(HOUSEHOLD, my_string);
}

int is_a_plug(char *my_string){
	to_lowercase (my_string);
	return startsWith(PLUG, my_string);
}

/*
 * Count lines of stdout
 */
int count_lines_stdout (char *file ){
	FILE *file_lines_stdout = NULL;
	int lines = 0, temp = 0;
	char *temp_line;
	size_t temp2;

	file_lines_stdout = open_file(file);
	temp = getline (&temp_line, &temp2, file_lines_stdout);
	if (temp == -1 ){
		printf("Cannot read anything\nTerminate execution\n");
		exit(1);
	}
	lines = atoi (temp_line);
	if (lines < 1 ){
		printf("Error converting line\nTerminate execution\n");
		exit(1);
	}
	return lines;
}

/*
 * Gives the reference to the opened file.
 */
FILE *open_file(char *my_filepath){
	FILE *stream_stderr = NULL;
	stream_stderr = fopen (my_filepath, "r");
	if (stream_stderr == NULL ){
		printf("Cannot read from: [%s]\nTerminate execution\n", my_filepath);
		exit(0);
	}
	return stream_stderr;
}

void to_lowercase(char *my_string){
	int i;
	char x;
	for(i = 0; my_string[i]; i++){
		x = tolower (my_string[i]);
		my_string[i] = x;
	}
}

/*
 * Modify the original string in order to manage that as a string coming from stdout
 *
 * NB: Remember to free the return value!
 */
char *purgeInfo (char info[] ){
	char *result = info;
	char *old;
	//Each time I free the memory dynamically allocated when I don't use it anymore
	result = str_replace (result, ", }, }", "");
	old = result;
	result = str_replace (result, "}, ", "");
	free (old);
	old = result;
	result = str_replace (result, "}", "");
	free (old);
	old = result;
	result = str_replace (result, ", ", ",");
	free (old);
	old = result;
	result = str_replace (result, " {", ",");
	free (old);
	return result;
}

/*
 * str 		-> 	Original string to split (not modified)
 * delimiter->	The delimiters
 * tokens	->	All the parts of the string splitted in an array.
 *
 * returns 	->	The number of tokens
 */
int split (char *str, char delimiter, char ***tokens){
	int count = 1;
	int token_len = 1;
	int i = 0;
	char *p;
	char *t;

	p = str;
	while (*p != '\0'){
		if (*p == delimiter){
			count++;
		}
		p++;
	}

	*tokens = (char**) malloc(sizeof(char*) *count);
	if (*tokens == NULL){
		exit(1);
	}

	p = str;
	while (*p != '\0'){
		if (*p == delimiter){
			(*tokens)[i] = (char*) malloc (sizeof(char) *token_len);
			if ((*tokens)[i] == NULL){
				exit(1);
			}
			token_len = 0;
			i++;
		}
		p++;
		token_len++;
	}
	(*tokens)[i] = (char*) malloc (sizeof(char) *token_len);
	if ((*tokens)[i] == NULL){
		exit(1);
	}

	i = 0;
	p = str;
	t = ((*tokens)[i]);
	while (*p != '\0'){
		if (*p != delimiter && *p != '\0'){
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
	tmp = result = malloc(strlen(orig) + (len_with - len_rep) *count + 1);

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
int startsWith(char *pre, char *str){
	size_t lenpre = strlen(pre),
		   lenstr = strlen(str);
	return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

