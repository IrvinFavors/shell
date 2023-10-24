CC=gcc
# CC=gcc -Wall

mysh: get_path.o which.o shell-with-builtin.o watchuser.o
	$(CC) -g -pthread shell-with-builtin.c get_path.o which.o watchuser.o -o mysh

shell-with-builtin.o: shell-with-builtin.c sh.h
	$(CC) -g -c shell-with-builtin.c

get_path.o: get_path.c get_path.h
	$(CC) -g -c get_path.c

which.o: which.c get_path.h
	$(CC) -g -c which.c

watchuser.o: watchuser.c watchuser.h
	$(CC) -g -c watchuser.c

clean:
	rm -rf shell-with-builtin.o get_path.o which.o watchuser.o mysh

