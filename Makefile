CFLAGS := -O3 $(CFLAGS)

gol: *.c *.h
	$(CC) $(CFLAGS) -o gol gol.c `sdl2-config --cflags --libs`

clean:
	rm -f gol

.PHONY: clean
