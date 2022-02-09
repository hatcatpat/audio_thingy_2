# cc := tcc
cc := gcc

W := -pedantic -Wall -Wno-missing-braces
l := -lm -ldl -lpthread

#O := -O3
O := -O0
debug := -g

all: ca

ca: ca.c ca.h ma.h
	$(cc) -std=c99 -o ca ca.c $(W) $(l) -g $(O)

run: run.c ca.h
	$(cc) -o run.so run.c $(W) -std=c99 -shared -fPIC $(debug) $(O)

jack: ca.c ca.h jack.h
	$(cc) -std=c99 -o ca ca.c $(W) $(l) -ljack -g $(O) -DCA_JACK

run_jack: run.c ca.h
	$(cc) -o run.so run.c $(W) -std=c99 -shared -fPIC $(debug) $(O) -DCA_JACK

clean:
	rm ca
	rm *.so
	rm *.o
