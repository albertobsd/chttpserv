CC = gcc
CFLAGS = -lpthread

default: serv

serv: serv.c
	clear
	$(CC) $(CFLAGS) -o serv serv.c

clean:
	clear
	rm serv
	rm logs.txt
	touch logs.txt

