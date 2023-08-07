CFLAGS = -Wall -lncurses -lm 

all: micro 

micro: main.o micro.o
	@echo "Linking and producing the final application"
	gcc $(CFLAGS) main.o micro.o -o micro	
	chmod +x micro 

main.o: main.c
	@echo "Compiling main file"
	gcc $(CFLAGS) -c main.c

micro.o: micro.c
	@echo "Compiling micro file"
	gcc $(CFLAGS) -c micro.c

clean:
	@echo "Removing everything but the source files"
	rm main.o micro.o micro 
