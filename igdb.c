#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "database.h"
#include <errno.h>
#include <limits.h>

//simple printing of prompt
void print_prompt() {
        printf("> ");
}
/*
 *properly formats the timestamp in a record to be readable by a human
 */
void format_date(char* buffer, size_t buffer_size, time_t date) {
    struct tm* timeinfo = localtime(&date);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M", timeinfo);
}


/*
 * returns current time in timestamp form 
 */
time_t current_time(){
  time_t cur_time = time(NULL);
  if(cur_time == (time_t)-1){
	  fprintf(stderr, "Failed to get current time.\n");
  }

  return cur_time;
}

/*
 * lists the database
 */
void db_list(Database* db) {
    printf("HANDLE               | FOLLOWERS  | LAST MODIFIED       | COMMENT\n"); //column names
    printf("-----------------------------------------------------------------------------\n");

    for (size_t i = 0; i < db->size; i++) { //loops over database
        Record record = db->records[i];
        char dateStr[20];
        format_date(dateStr, sizeof(dateStr), record.dateLastModified); //formats date for each record

        // Truncate handle and comment if they exceed their column widths
        printf("%-20.20s | %-10lu | %-19s | %-30.30s\n", //truncates handle to only 20 characters, long integers should only be 10 digits, date should be 19, and comment will be truncated after 30 characters
               record.handle,
               record.followerCount,
               dateStr,
               record.comment);
    }
}


/* checks if string has commas or whitespace
 * @param const char* str is a string that will be validated 
 * @return 1 is the string contains commas or whitespace 0 if not
 *
 */
int validate_string(const char* str){
	while(*str != '\0'){
		if(*str == ',' || isspace((unsigned char)*str)){
			return 1;
		}
		str++;
	}
	return 0;
}

/* checks if the comment is valid
 * fgets works, comment is not a new line or null terminator or has commas or is empty
 * @param comment takes in comment and size of the comment
 * @return 0 if comment is valid and 1 is comment is valid
 *
 */
int read_validated_comment(char* comment, size_t size) {
    printf("Comment> "); // Prompt user for comment
    if (fgets(comment, size, stdin) == NULL) { // Get user input
        fprintf(stderr, "Error reading comment.\n");
        return 0; // Indicate failure
    }

    if (comment[0] == '\n' || comment[0] == '\0' || comment[0] == ' ') { // Comment is not newline or null terminator
        fprintf(stderr, "Error: Comment cannot be empty.\n");
        return 0; // Failure
    }

    // Remove newline character if present
    if(strchr(comment, '\n') != NULL) {
        comment[strcspn(comment, "\n")] = '\0';
    } else {
        // Flush the input buffer if the comment is too long and doesn't end with a newline
        int c;
        while((c = getchar()) != '\n' && c != EOF);
    }

    // Check for commas in the comment
    if (strchr(comment, ',') != NULL) {
        fprintf(stderr, "Error: Comment cannot contain commas.\n");
        return 0; // Failure
    }

    // Check for valid ASCII range (32 to 127)
    for(size_t i = 0; comment[i] != '\0'; i++) {
        if ((unsigned char)comment[i] < 32 || (unsigned char)comment[i] > 126) {
            fprintf(stderr, "Error: Comment contains invalid characters.\n");
            return 0; // Failure
        }
    }

    return 1; // Success
}

/*
 * adds a new record to the database with a handle and follower count
 */
void db_add(Database *db, const char* handle, unsigned long followerCount, int *flag){
    //checks if handle already exists
    if (db_lookup(db, handle)) {
        fprintf(stderr, "Error: Handle '%s' already exists.\n", handle);
        return;
    }

     if (handle[1] == '\0') {
        fprintf(stderr, "Error: Handle cannot be empty.\n");
        return;
    }
    //if handle does not start with '@' error message
     if (handle[0] != '@') {
        fprintf(stderr, "Error: handle must start with '@'.\n");
        return;
    }
    
    //casts integer of 0 to pointer of Record just to get the size of the handle member of Record
    //check if the handle is too long
    if (strlen(handle) >= sizeof(((Record *)0)->handle)) {
        fprintf(stderr, "Error: handle is too long.\n");
        return;
    }
     
    //checks if handle is valid
    if(validate_string(handle)){
	    fprintf(stderr, "Error: handle cannot contain commas or whitespace.\n");
	    return;
    }

    Record newRecord = {0};

    char comment[256];
    if(!read_validated_comment(comment, sizeof(comment))){
	    return;
    }
    
    //updating members of the newRecord
    strncpy(newRecord.handle, handle, sizeof(newRecord.handle) - 1);
    newRecord.handle[sizeof(newRecord.handle) - 1] = '\0';
    newRecord.followerCount = followerCount;
    strncpy(newRecord.comment, comment, sizeof(newRecord.comment) - 1);
    newRecord.comment[sizeof(newRecord.comment) - 1] = '\0';
    newRecord.dateLastModified = current_time();

    // Add the new record to the database
    db_append(db, &newRecord);
    *flag =1; //flag is one since database was modified
}
/*
 * updates existing handle in the database
 */
void db_update(Database *db, const char *handle, unsigned long follower, int *flag) {
    Record *rec = db_lookup(db, handle); // looking for handle

    if (rec == NULL) { // Check if the lookup was unsuccessful
        fprintf(stderr, "Error: no entry with handle %s\n", handle);
        return;
    }
    
    char comment[256];
    if(!read_validated_comment(comment, sizeof(comment))){
            return;
    }

    // copy comment into comment field (first 63 characters of the comment)
    strncpy(rec->comment, comment, sizeof(rec->comment) - 1);
    rec->comment[sizeof(rec->comment) - 1] = '\0';

    // Update the followerCount
    rec->followerCount = follower;
    //Update time
    rec->dateLastModified = current_time();
    *flag = 1; //flag set to 1 since database was modified
}

/*
 * saves database
 */
void db_save(Database * db){
    db_write_csv(db, "database.csv");
    printf("Wrote %d records.\n", db->size);
};

/*
 * handles exit
 * @param *db the database
 * @param int *should exit pointer to see if the program needs to quit
 * @param int *flag pointer to see if database was modified
 */
void handle_exit_command(Database *db, int *should_exit, int *flag) {
    char *arg = strtok(NULL, " \n"); // Attempt to get the next argument.

     if (arg == NULL && !*flag) {
        db_free(db); // Free database resources.
        *should_exit = 1; // Signal the main loop to exit.
        return;
    }
    // Check if there are unsaved changes and the user did not specify "exit fr"
    if (*flag) {
        if (arg == NULL || strcmp(arg, "fr") != 0) {
            fprintf(stderr, "Error: You did not save your changes. Use 'exit fr' to force exiting anyway.\n");
            return; // Return early to avoid exiting.
        }
    } else if ((strcmp(arg, "fr") != 0) && (arg != NULL)) {
        // If there are no unsaved changes but an argument is provided, it's an incorrect usage.
        fprintf(stderr, "Error: 'exit' command does not take any arguments.\n");
        return; // Return early to avoid exiting.
    }
    db_free(db); // Free database resources.
    *should_exit = 1; // Signal the main loop to exit.
}

/* processes command for save, list, update, exit, add
 */
void process_command(Database *db, char *input, int *should_exit, int *flag) {
    char *command = strtok(input, " \n"); // Extract the command.

    //if command is missing
    if (command == NULL) {
        printf("Error: Command missing.\n");
        return;
    }

    //find which command it is and act accordingly
    if (strcmp(command, "list") == 0) {
        //"list" command.
        if (strtok(NULL, " \n") != NULL) { //make sure no arguments following list
            fprintf(stderr, "Error: 'list' command does not take any arguments.\n");
	    return;
        } else {
            db_list(db);
        }
    } else if (strcmp(command, "save") == 0) {
        //"save" command.
	 if (strtok(NULL, " \n") != NULL) { //make sure no arguments following save
            fprintf(stderr, "Error: 'save' command does not take any arguments.\n");
            return;
        }
        db_save(db);
        *flag = 0; //flag is 0 since saved 
    } else if (strcmp(command, "exit") == 0) {
	 handle_exit_command(db, should_exit, flag);
    } else if (strcmp(command, "add") == 0 || strcmp(command, "update") == 0) {
        // Proceed only if command is "add" or "update".
        char *handle = strtok(NULL, " \n"); //get the handle
        char *followerCountStr = strtok(NULL, " \n"); //the follower count
        char *extra = strtok(NULL, " \n"); //and any extra args following handle and follower count
	char *endptr; //to check if string to long conversion was successful
	errno = 0;

        if (!handle || !followerCountStr || extra) { //should have handle followercount and no args after
            fprintf(stderr, "Error: usage: %s HANDLE FOLLOWERS.\n", command);
            return;
        }
        
	if(followerCountStr[0] == '-'){ //check if follower count is negative
		fprintf(stderr, "Error: follower count cannot be negative.\n");
		return;
	}
	unsigned long followerCount = strtoul(followerCountStr, &endptr, 10); //convert followerCount form string to long

       // Check for conversion errors
       if (errno == ERANGE && followerCount == ULONG_MAX) {
            // Overflow occurred
            fprintf(stderr, "Error: Follower count is too large.\n");
	    return;
        } else if (endptr == followerCountStr) {
            // No digits were found
            fprintf(stderr, "Error: follower count must be an integer.\n");
	    return;
       } else if (*endptr != '\0') {
        // The whole string wasn't converted, additional characters after number
           fprintf(stderr, "Warning: Extra characters after number: \"%s\"\n", endptr);
	   return;
       }

        if (strcmp(command, "add") == 0) { //if the command was add then call the add function
            db_add(db, handle, followerCount, flag);
        } else if (strcmp(command, "update") == 0) {
            db_update(db, handle, followerCount, flag); //if the commad was update then call the update function
        }
    } else {
        // Handle unrecognized commands.
        fprintf(stderr, "Error: Unrecognized command.\n");
    }
}


//main loop
int main_loop(Database *db) {
    char *input = NULL; // Buffer for input, getline will allocate memory
    size_t input_size = 0; // Size of the input buffer
    int should_exit = 0; //track when should exit the program
    int flag = 0; //track when modified


    printf("Loaded %d records.\n", db->size); // Prints how many records in database

    while (!should_exit) { // Loop to keep going until exit
        print_prompt();
        // Gets user input, automatically allocates memory as needed
        ssize_t characters_read = getline(&input, &input_size, stdin);
        if (characters_read == -1) { // Check for read error or EOF and entire program terminates since while loop is broken
            printf("Error reading input or EOF encountered.\n");
	    db_free(db); //makes sure memory is freed anyways
            break;
        }
        process_command(db, input, &should_exit, &flag); // Call to process command
    }
    free(input); // Free the allocated buffer
    return 0; // Returns 0 upon successful execution
}
//main 
int main()
{
    Database db = db_create();
    db_load_csv(&db, "database.csv");
    return main_loop(&db);
}
