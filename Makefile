# Makefile, versao 02
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS= -g -Wall -pedantic -std=gnu99
CC=gcc

heatSim: main.o matrix2d.o  
	$(CC) $(CFLAGS) -pthread -o heatSim main.o matrix2d.o 

main.o: main.c matrix2d.h  
	$(CC) $(CFLAGS) -c main.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c

clean:
	rm -f *.o heatSim

zip:
	zip projeto3.zip main.c matrix2d.c matrix2d.h Makefile

run:
	./heatSim 10 10.0 10.0 0.0 0 60 5 0 
