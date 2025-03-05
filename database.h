#ifndef DB_H
#define DB_H

typedef struct Record { 
char handle[32];
char comment[64];
long unsigned int followerCount;
long unsigned int dateLastModified;
} Record;

typedef struct Database { 
 Record *records;
 int capacity; //length of underlying array
 int size; //number of elements inside array
} Database;

Database db_create();

void db_append(Database * db, Record const * item);

Record *db_index(Database * db, int index);

Record *db_lookup(Database * db, char const * handle);

void db_free(Database * db);

void db_load_csv(Database * db, char const * path);

void db_write_csv(Database * db, char const * path);

#endif
