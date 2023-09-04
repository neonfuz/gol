CFLAGS := -O3 $(CFLAGS)

# macos
export CPATH=/opt/homebrew/opt/sdl2/include

gol: gol.c *.h
	$(CC) $(CFLAGS) gol.c -o $@ `sdl2-config --cflags --libs`

web/gol.html: gol.c *.h
	mkdir -p web
	touch web/.nojekyll
	emcc gol.c -o $@ -g -lm --bind -s USE_SDL=2 -pthread -s PTHREAD_POOL_SIZE=4

clean:
	rm -f gol

.PHONY: clean
