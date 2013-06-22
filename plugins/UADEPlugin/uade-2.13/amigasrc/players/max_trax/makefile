CFLAGS = -pacp0t -mcd -bs -qf -r6 -hi include:amiga.pre
AFLAGS = -sa -i include:asm2.0 -c -d

all: player

max.o: driver.i shared.asm

player: player.o max.o
	ln -o player player.o max.o -lc
