GCCFLAGS=-g -Os -Wall -mmcu=atmega168 
LINKFLAGS=-Wl,-u,vfprintf -lprintf_flt -Wl,-u,vfscanf -lscanf_flt -lm
AVRDUDEFLAGS=-c avr109 -p m168 -b 115200 -P /dev/ttyUSB0
LINKOBJECTS=../avr_apple_remote.o

all:	avr_apple_remote_application-upload

avr_apple_remote_application.hex:	avr_apple_remote_application.c	
	make -C ../
	avr-gcc ${GCCFLAGS} ${LINKFLAGS} -o avr_apple_remote_application.o avr_apple_remote_application.c  ${LINKOBJECTS}
	avr-objcopy -j .text -O ihex avr_apple_remote_application.o avr_apple_remote_application.hex
	
avr_apple_remote_application.ass:	avr_apple_remote_application.hex
	avr-objdump -S -d avr_apple_remote_application.o > avr_apple_remote_application.ass
	
avr_apple_remote_application-upload:	avr_apple_remote_application.hex
	avrdude ${AVRDUDEFLAGS} -U flash:w:avr_apple_remote_application.hex:a

build:	avr_apple_remote_application.c
	make -C ../
	avr-gcc ${GCCFLAGS} ${LINKFLAGS} -o avr_apple_remote_application.o avr_apple_remote_application.c  ${LINKOBJECTS}
