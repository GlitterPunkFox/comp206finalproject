igdb: igdb.o database.o
	gcc -Wall -o igdb igdb.o database.o

igdb.o: igdb.c database.h
	gcc -Wall -c igdb.c

database.o: database.c database.h
	gcc -Wall -c database.c
