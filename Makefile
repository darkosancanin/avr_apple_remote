GCCFLAGS=-g -Os -Wall -mmcu=atmega168 
LINKFLAGS=-Wl,-u,vfprintf -lprintf_flt -Wl,-u,vfscanf -lscanf_flt -lm
AVRDUDEFLAGS=-c avr109 -p m168 -b 115200 -P /dev/ttyUSB0
LINKOBJECTS=

all:	avr_apple_remote-upload

avr_apple_remote.hex:	avr_apple_remote.c	
	avr-gcc ${GCCFLAGS} ${LINKFLAGS} -o avr_apple_remote.o avr_apple_remote.c  ${LINKOBJECTS}
	avr-objcopy -j .text -O ihex avr_apple_remote.o avr_apple_remote.hex
	
avr_apple_remote.ass:	avr_apple_remote.hex
	avr-objdump -S -d avr_apple_remote.o > avr_apple_remote.ass
	
avr_apple_remote-upload:	avr_apple_remote.hex
	avrdude ${AVRDUDEFLAGS} -U flash:w:avr_apple_remote.hex:a

build:	avr_apple_remote.c
	avr-gcc ${GCCFLAGS} ${LINKFLAGS} -o avr_apple_remote.o avr_apple_remote.c  ${LINKOBJECTS}
