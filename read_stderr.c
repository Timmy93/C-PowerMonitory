/*
 * read_stderr.c
 *
 *  Created on: 31/mar/2016
 *      Author: timmy
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_STDERR "/home/timmy/Scrivania/stderr-dataGenerator.txt"
#define PATH_STDOUT "/home/timmy/Scrivania/stdout-dataGenerator.txt"
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

void insert_house( House **, House **, char * );
void insert_household( House *, Household **, char * );
void insert_plug( Household *, Plug **, char * );
void print_houses( House * );
void print_house( House * );
int extract_id( char * );
int is_a_house( char * );
int is_a_household( char * );
int is_a_plug( char * );
int to_lowercase( char * );
char *purgeInfo( char info[] );
int split (char *str, char delimiter, char ***tokens);
char *str_replace(char *orig, char *rep, char *with) ;
int startsWith(char *pre, char *str);

int main(int argc, char *argv[]){
	int i = 0, num_token_stdout = 0, num_token_stderr = 0;
	FILE *my_stderr = NULL;
	FILE *my_stdout = NULL;
	size_t len;
	ssize_t read;
	House *start_house = NULL;
	House *last_appended_house = NULL;
	Household *last_appended_household = NULL;
	Plug *last_appended_plug = NULL;
	char **tokens_stdout = NULL;
	char **tokens_stderr = NULL;
//	char *string_stdout = "11,1459365287,0,1,4,68";
//	char *string_stderr = "house 0 {household 0 {plug 0, plug 1, plug 2, plug 3, plug 4, plug 5, plug 6, }, household 1 {plug 0, plug 1, plug 2, plug 3, plug 4, plug 5, plug 6, plug 7, plug 8, }, household 2 {plug 0, plug 1, plug 2, plug 3, plug 4, }, household 3 {plug 0, plug 1, plug 2, plug 3, plug 4, plug 5, plug 6, plug 7, plug 8, }, household 4 {plug 0, plug 1, plug 2, plug 3, plug 4, plug 5, plug 6, plug 7, }, household 5 {plug 0, plug 1, plug 2, plug 3, plug 4, plug 5, plug 6, plug 7, plug 8, }, }";
	char *string_stdout = NULL;
	char *string_stderr = NULL;	char *purged_string;

	//Read from stderr
	my_stderr = fopen( PATH_STDERR, "r" );
	if( my_stderr == NULL ){
		printf("Cannot read from: [%s]\nTerminate execution", PATH_STDERR);
		exit(0);
	}
	while ( getline(&string_stderr, &len, my_stderr) != -1) {
		if( is_a_house( string_stderr ) ){
			//TODO insert_house();
		} else if ( is_a_household( string_stderr ) ){
			//TODO
		} else if ( is_a_plug( my_stderr ) ){
			//TODO
		} else {
			printf("Cannot recognize this string:\n[%s]\n");
			exit(0);
		}
	}

	//Read from stdout
	my_stdout = fopen( PATH_STDOUT, "r" );
	if( my_stdout == NULL ){
		printf("Cannot read from: [%s]\nTerminate execution", PATH_STDOUT);
		exit(0);
	}
	while ( getline(&string_stdout, &len, my_stdout) != -1) {
		printf("%s", string_stdout);
	}

	//Clear the string received from stderr
	purged_string = purgeInfo( string_stderr );
	num_token_stderr = split( purged_string, ',', &tokens_stderr );
	for (i = 0; i < num_token_stderr; i++){
		if( startsWith( "household ", tokens_stderr[i] ) ){
			printf("token #%d: %s\n", i, tokens_stderr[i]);
		} else {
			printf("Not an household\n");
		}

	}

	//Extract data as received from stdout
	num_token_stdout = split(string_stdout, ',', &tokens_stdout);
	for (i = 0; i < num_token_stdout; i++){
		printf("token #%d: %s\n", i, tokens_stdout[i]);
	}


	for(i = 0; i < 5; i++){
		insert_house( &start_house, &last_appended_house, i);
	}
	print_houses(start_house);

	//Clearung unused memory
	free(string_stderr);
	free(string_stdout);

	return 0;
}

//TODO Remember to free memory allocated by the array of tokens.

void insert_house(House **start_house, House **last_appended, char *string_id){
	//Declaration and allocation
	House *h;
	h = (House *) malloc( sizeof(House) );
	int id = -1;

	//Getting id
	id = extract_id( string_id );
	if( id == -1 ){
		printf("Cannot find a valid ID");
		exit(1);
	}

	//Security checks
	if( h == NULL ){
		printf("Not enough memory to allocate a new House!\n");
		exit(0);
	}

	//Initialization
	h->id = id;
	h->h_households = NULL;
	h->next = NULL;

	//Appending to the end of the list
	if( *start_house == NULL ){
		//First House of the list
		*start_house = h;
	} else {
		//The last element of the list points to the new element
		(*last_appended)->next = h;
	}
	//Updating the reference to the last element
	*last_appended = h;
}

void insert_household(House *my_house, Household **last_appended_household, char *string_id){
	//Declaration and allocation
	Household *hh;
	hh = (Household *) malloc( sizeof(Household) );
	int id = -1;

	//Getting id
	id = extract_id( string_id );
	if( id == -1 ){
		printf("Cannot find a valid ID");
		exit(1);
	}

	//Security checks
	if( hh == NULL ){
		printf("Not enough memory to allocate a new Household!\n");
		exit(0);
	}
	//Insertion of an household before the creation of an house - Something wrong
	if( my_house == NULL ){
		printf("Error: No House - Cannot insert a new Household\n");
		exit(0);
	}

	//Initialization
	hh->id = id;
	hh->hh_plugs = NULL;
	hh->next = NULL;

	//Updating references
	if( my_house->h_households == NULL ){
		//Last household refers to an other House
		my_house->h_households = hh;
	} else {
		//Normally append to the last household
		(*last_appended_household)->next = hh;
	}
	//Update the reference to the last household
	*last_appended_household = hh;
}

void insert_plug(Household *my_household, Plug **last_appended_plug, char *string_id){
	//Declaration and allocation
	Plug *p;
	p = (Plug *) malloc( sizeof(Plug) );
	int id = -1;

	//Getting id
	id = extract_id( string_id );
	if( id == -1 ){
		printf("Cannot find a valid ID");
		exit(1);
	}
	//Security Checks
	if( p == NULL ){
			printf("Not enough memory to allocate a new Plug!\n");
			exit(0);
	}
	//Insertion of a plug before the creation of an household - something wrong
	if( my_household == NULL ){
		printf("Error: No Household - Cannot insert a new Plug\n");
		exit(0);
	}

	//Initialization
	p->id = id;
	p->next = NULL;
	//TODO Do something with the value
	p->valore = 0;

	//Updating references
	if( my_household->hh_plugs == NULL ){
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
	while( start_house != NULL ){
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
int extract_id( char * ){
	//TODO Write the function to extract id - see strstr to extract the id
	return 1;
}

int is_a_house(char *my_string){
	to_lowercase( my_string );
	return startsWith(HOUSE, my_string);
}

int is_a_household(char *my_string){
	to_lowercase( my_string );
	return startsWith(HOUSEHOLD, my_string);
}

int is_a_plug(char *my_string){
	to_lowercase( my_string );
	return startsWith(PLUG, my_string);
}

void to_lowercase(char *my_string){
	for(int i = 0; my_string[i]; i++){
		my_string[i] = tolower(my_string[i]);
	}
}

/*
 * Modify the original string in order to manage that as a string coming from stdout
 *
 * NB: Remember to free the return value!
 */
char *purgeInfo( char info[] ){
	char *result = info;
	char *old;
	//Each time I free the memory dynamically allocated when I don't use it anymore
	result = str_replace( result, ", }, }", "");
	old = result;
	result = str_replace( result, "}, ", "");
	free( old );
	old = result;
	result = str_replace( result, "}", "");
	free( old );
	old = result;
	result = str_replace( result, ", ", ",");
	free( old );
	old = result;
	result = str_replace( result, " {", ",");
	free( old );
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
    while (*p != '\0')
    {
        if (*p == delimiter)
            count++;
        p++;
    }

    *tokens = (char**) malloc(sizeof(char*) * count);
    if (*tokens == NULL)
        exit(1);

    p = str;
    while (*p != '\0')
    {
        if (*p == delimiter)
        {
            (*tokens)[i] = (char*) malloc( sizeof(char) * token_len );
            if ((*tokens)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*tokens)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*tokens)[i] == NULL)
        exit(1);

    i = 0;
    p = str;
    t = ((*tokens)[i]);
    while (*p != '\0')
    {
        if (*p != delimiter && *p != '\0')
        {
            *t = *p;
            t++;
        }
        else
        {
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
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

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
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
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
int startsWith(char *pre, char *str){
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

