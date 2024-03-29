CFLAGS := -O3 $(CFLAGS) `sdl2-config --cflags`
LDFLAGS := `sdl2-config --libs`

# macos
export CPATH=/opt/homebrew/opt/sdl2/include

gol: gol.c *.h
	$(CC) $(CFLAGS) $(LDFLAGS) gol.c -o $@

docs/gol.html: gol.c *.h
	mkdir -p docs
	touch docs/.nojekyll
	emcc gol.c -o $@ -g -lm --bind -s USE_SDL=2
# -pthread -s PTHREAD_POOL_SIZE=4

clean:
	rm -f gol

.PHONY: clean
