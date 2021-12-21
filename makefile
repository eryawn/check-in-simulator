ACS:Main.c
	gcc Queue.c -c -o Queue.o 
	gcc Main.c -c -o Main.o
	gcc Main.o Queue.o -pthread -o ACS