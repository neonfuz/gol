gol: *.c
	$(CC) $(CFLAGS) -o gol gol.c `sdl2-config --cflags --libs`

clean:
	rm -f gol

.PHONY: clean
