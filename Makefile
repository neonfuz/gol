all:
	gcc -o gol gol.c `sdl2-config --cflags --libs`
dbg:
	gcc -g -o gol gol.c `sdl2-config --cflags --libs`
prof:
	gcc -pg -o gol gol.c `sdl2-config --cflags --libs`
