all: netfilter-test

netfilter-test : main.o
	gcc -o netfilter-test main.o -lnetfilter_queue -lnet

main.o : main.c
	gcc -c -o main.o main.c

clean:
	rm -r netfilter-test *.o
	
