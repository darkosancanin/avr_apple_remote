GCCFLAGS=-g -Os -Wall -mmcu=atmega168

all:	avr_apple_remote.o

avr_apple_remote.o:	avr_apple_remote.c
	avr-gcc ${GCCFLAGS} -o avr_apple_remote.o -c avr_apple_remote.c 
