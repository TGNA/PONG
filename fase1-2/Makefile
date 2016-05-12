all : winsuport.o tennis0

winsuport.o : winsuport.c winsuport.h
	gcc -Wall -c winsuport.c -o winsuport.o

tennis0 : tennis0.c winsuport.o winsuport.h
	gcc -Wall tennis0.c winsuport.o -o tennis0 -lcurses

tennis1 : tennis1.c winsuport.o winsuport.h
	gcc -Wall tennis1.c winsuport.o -o tennis1 -lcurses -lpthread

tennis2 : tennis2.c winsuport.o winsuport.h
	gcc -Wall tennis2.c winsuport.o -o tennis2 -lcurses -lpthread

clean:
	rm winsuport.o tennis0
