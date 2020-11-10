all: 1m-block

1m-block : main.o
	gcc -o 1m-block main.o -lnetfilter_queue -lnet

main.o : main.c
	gcc -c -o main.o main.c

clean:
	rm -r 1m-block *.o
	
