#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include <time.h>

/* 
 * initializes Database
 * 
 */

Database db_create() {
    Database db;

    db.size = 0;
    db.capacity = 4;

    //allocating memory for dynamic records array
    db.records = (Record *)malloc(db.capacity * sizeof(Record));
    if (db.records == NULL) {
        fprintf(stderr, "Failed to allocate memory for records.\n");
        exit(1);
    }

    return db;
}
/*
 * Copies the record pointed to by iten to the end of the database
 * @param *db pointer to database
 * @param *item pointer to item that will be appended to database
 *
 */
void db_append(Database *db, const Record *item){
	if(db->size == db->capacity){
		int newCapacity = db->capacity*2;
		Record *newRecords = (Record *)malloc(newCapacity * sizeof(Record));
	
		
	if (newRecords == NULL){
		fprintf(stderr, "Failed to allocate memory for expanding records.\n");                 
                exit(1);
	}

	for (int i = 0; i < db->size; i++){
		newRecords[i] = db->records[i];
	}

	free(db->records);

	db->records = newRecords;
	db->capacity = newCapacity; 
	}
  db->records[db->size++] = *item;	
}
/* Returns a pointer to the item in the database at the given index
 * @param *db pointer to database
 * @param int index integer value of desired index
 * @return returns pointer of item in the database at index
 */

Record * db_index(Database *db, int index){
	//if index is invalid (too big or negative then return NULL pointer)
	if(index < 0 || index >= db->size){
		return NULL;
	}

	return &(db->records[index]);
}


/* 
 * @param *db pointer to database
 * @param *handle string literal of desired hangle
 * @return a pointer to the first item in the database whose handle field equals the given value
 */
Record *db_lookup(Database * db, char const *handle){
	for(int i = 0; i < db->size; i++){
		if(strcmp(db->records[i].handle, handle) == 0){
			return &(db->records[i]);
		}
	}
	return NULL; //no matching handle found
}

/* Releases the memory held by the underlying array
 * @param *db takes in a pointer to a database as an argument
 *
 */
void db_free(Database *db){
	if(db == NULL) return; //database is null return
	
	if(db->records != NULL){			
		free(db->records); //free allocated memory
		db->records = NULL; //make sure no dangling pointers
	}

	db->capacity = 0;
	db->size =0;
	
}
/*
 * @param **token takes in a pointer to a string literal of the handle name
 * @param *record takes in a pointer to a record which the string literal will be written into the handle member of that record
 */

void parse_handle(char **token, Record *record){
	if(*token != NULL){ 
		if(strlen(*token) < sizeof(record->handle)){//checks if string is too long
			strcpy(record->handle, *token); 
		}else{
			fprintf(stderr, "Your handle is too long it will be truncated.\n");
			int lastIndex = sizeof(record->handle)-1; //to keep track of index where null terminator should be
			strncpy(record->handle, *token, lastIndex); //copy first 31 chars into handle member
		        record->handle[lastIndex] = '\0'; //last char is null terminator
		}

	}
}

/*
 * @param **token takes in a pointer to a string literal of the comment name
 * @param *record takes in a pointer to a record which the string literal will be written into the comment member of that record
 */

void parse_comment(char **token, Record *record){
	if(*token != NULL){
		if(strlen(*token) < sizeof(record->comment)){
                     strcpy(record->comment, *token);
		}else{
			fprintf(stderr, "Your comment is too long it will be truncated.\n");
			int lastIndex = sizeof(record->comment)-1;
			strncpy(record->comment, *token, lastIndex);
			record->comment[lastIndex] = '\0';
		}

	}
}
/* @param **token takes in a pointer to a string literal of the followerCount
 * @param *record takes in a pointer to a record which the string literal will be written into the followerCount member of that record (the string will be converted into an unsigned long first)
 */
void parse_followerCount(char **token, Record *record){
	if(*token != NULL){
		char *endPtr;

		unsigned long followers = strtoul(*token, &endPtr, 10);

		if(endPtr == *token){
                  fprintf(stderr, "Error: No digits were found in followerCount.\n");
		}else{
			record->followerCount = followers;
		}
        }
}

 /*
 * @param **token takes in a pointer to the string literal of the dateLastModified
 * @param *record tkaes in a pointer to a record which the string literal will be written into the followerCount member of that record (the string will be converted into an unsigned long first)
 */
void parse_dateLastModified(char **token, Record *record){
	if(*token != NULL){
		char *endPtr;
		unsigned long date = strtoul(*token, &endPtr, 10);

		if(endPtr == *token){
			fprintf(stderr, "Error: No digits were found in dateLastModified.\n");
		}else{
			record->dateLastModified = date;
		}
	}
}
/*
 * @param char const *line string literal to be parsed into a type Record
 * parses a single line of CSV data into one Record 
 */
Record parse_record(char const * line){
	Record record = {0}; //initialize all members of record to 0

	char *lineCopy = malloc(strlen(line) + 1); //allocates memory to copy line of csv file into
	if(lineCopy == NULL){
		fprintf(stderr, "Failed memory allocation.\n");
		exit(1);
	}

	strcpy(lineCopy, line); //copies line of csv into lineCopy
        
	int len = strlen(lineCopy);

	if(len > 0 && (lineCopy[len -1] == '\n')){ //if the line ends in a null terminator trim the new line
		lineCopy[len -1] = '\0'; //add a null terminator
	}

	char *token = strtok(lineCopy, ","); //the delimiter is , in our csv file

	if(token != NULL){ //will parse and store every field in the csv file into a new record
		parse_handle(&token, &record); //until the first , is the handle
		token = strtok(NULL, ","); 
		parse_followerCount(&token, &record); //until the second , is the followerCount
		token = strtok(NULL, ",");
		parse_comment(&token, &record); //until the third , is the comment
		token = strtok(NULL, ",");
		parse_dateLastModified(&token, &record); //until the end of the line is the dateLastModified
        }

	free(lineCopy); //avoid memory leak
	return record; //return the new record
}
/*
 * @param *db pointer to already initialized dtabase that the records will be read from
 * Appends the records read from the file at 'path' into the already intialized database 'db'
 */

void db_load_csv(Database *db, char const *path){
    FILE *file = fopen(path, "rt"); 
    
    if(file == NULL){
       fprintf(stderr, "No file %s exists, failed to read from %s, returning early\n", path, path);
       return; //Return early if the file cannot be opened
    }
    //copied from man pages
    char *line = NULL;
    size_t len =0;
    ssize_t nread;

    while(((nread = getline(&line, &len, file)) != -1)){
	    Record record = parse_record(line);
	    db_append(db, &record);
    }

    free(line); //avoid memory leak
    fclose(file); 
}

/*
 * @param *db pointer to already initialized database that the records will written into
 *  Overwrites the file located at 'path' with the contents of the database, represented in CSV format
 */


void db_write_csv(Database *db, const char *path) {
    FILE *file = fopen(path, "wt");

    if (file == NULL) {
        // If the initial attempt to open the specified file fails,
        // try to create or open a fallback file named "database.csv".
        printf("Unable to open or create file '%s'. Trying to create 'database.csv' instead.\n", path);
        file = fopen("database.csv", "wt");
        if (file == NULL) {
            // If opening the fallback file also fails, report the error and exit.
            fprintf(stderr, "Failed to create fallback file 'database.csv'.\n");
            exit(1);
        }
    }

    // Loop over the database and write each record to the file in CSV format.
    for (int i = 0; i < db->size; i++) {
        Record *record = &db->records[i];
        fprintf(file, "%s,%lu,%s,%lu\n",
                record->handle,
                record->followerCount,
                record->comment,
                record->dateLastModified);
    }

    // Properly close the file to avoid resource leaks.
    fclose(file);
}


