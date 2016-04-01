/*
 * read_stderr.c
 *
 *  Created on: 31/mar/2016
 *      Author: timmy
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Definition of plug
struct plug{
    int id;
    int valore;
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

void insert_house( House **, House **, int );
void insert_household( House *, Household **, int );
void insert_plug( Household *, Plug **, int );
void print_houses( House * );
void print_house( House * );

int main(int argc, char *argv[]){
	int i;
	House *start_house = NULL;
	House *last_appended_house = NULL;
	Household *last_appended_household = NULL;
	Plug *last_appended_plug = NULL;

	for(i = 0; i < 5; i++){
		insert_house( &start_house, &last_appended_house, i);
	}
	print_houses(start_house);

	return 0;
}

//TODO Recognize Strings

void insert_house(House **start_house, House **last_appended, int id){
	//Declaration and allocation
	House *h;
	h = (House *) malloc( sizeof(House) );

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

void insert_household(House *my_house, Household **last_appended_household, int id){
	//Declaration and allocation
	Household *hh;
	hh = (Household *) malloc( sizeof(Household) );

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

void insert_plug(Household *my_household, Plug **last_appended_plug, int id){
	//Declaration and allocation
	Plug *p;
	p = (Plug *) malloc( sizeof(Plug) );

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


