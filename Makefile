CC = g++
CFLAGS = -g  -Wall 
O_FILES = ppu.o main.o  mmc1.o cpu.o rom.o ram.o


all: $(O_FILES)
	$(CC) $(CFLAGS) -lpthread -framework GLUT -framework OpenGL $(O_FILES) -o main
	ctags -f tags ppu.cpp ppu.h ram.cpp ram.h  main.cpp rom.cpp rom.h  cpu.cpp cpu.h 2> /dev/null > /dev/null

ppu.o : ppu.cpp
	$(CC) $(CFLAGS) -c ppu.cpp -o ppu.o

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp -o main.o

mmc1.o: mmc1.cpp
	$(CC) $(CFLAGS) -c mmc1.cpp -o mmc1.o

rom.o : rom.cpp
	$(CC) $(CFLAGS) -c rom.cpp -o rom.o

cpu.o: cpu.cpp
	$(CC) $(CFLAGS) -c cpu.cpp -o cpu.o

ram.o : ram.cpp
	$(CC) $(CFLAGS) -c ram.cpp -o ram.o


clean:
	rm -f *.o *.gch



test:
	./main nestest.nes run > mytest.log
