CC=avr-gcc
OBJCOPY=avr-objcopy

CFLAGS=-g -mmcu=atmega8

all : demo.out
	$(OBJCOPY) -j .text -O ihex demo.out rom.hex
	rm -r demo.o demo.out demo.map
	# - раскомментировать если нужно сразу и прошить: avrdude -p m8 -c usbasp -U flash:w:rom.hex -F

demo.out : demo.o
	$(CC) $(CFLAGS) -o demo.out -Wl,-Map,demo.map demo.o

demo.o :
	$(CC) $(CFLAGS) -Os -c main.c -o demo.o
