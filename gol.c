#include <SDL2/SDL.h>

#include "config.h"

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Surface *bkg;
static FILE *urandom;
static Uint8 *cells;
static Uint8 *count;
static Uint8 *last_cells;
static Uint8 *last_count;
static struct ThreadData {
	SDL_Rect rect;
	SDL_mutex *prev, *next;
} thread_data[THREADS];
static int paused;
static int view;
static int framestep;

#define WINS ( (WINW + 2) * (WINH + 2) )
#define CELL_REF(cells, x, y) *((cells) + ((x) + 1) + ((y) + 1) * (WINW + 2))
#define LEN(a) ( sizeof(a) / sizeof(a[0]) )

static
SDL_Color colors_1[] = {
	{0x00, 0x00, 0x00, 0xFF},
	{0xFF, 0xFF, 0xFF, 0xFF},
};

static
SDL_Color colors_2[] = {
	{0x00, 0x00, 0x00, 0xFF},
	{0x00, 0x00, 0x7F, 0xFF},
	{0x00, 0x7F, 0x00, 0xFF},
	{0x00, 0xCF, 0x00, 0xFF},
	{0x7F, 0x00, 0x00, 0xFF},
	{0x9F, 0x00, 0x00, 0xFF},
	{0xBF, 0x00, 0x00, 0xFF},
	{0xDF, 0x00, 0x00, 0xFF},
	{0xFF, 0x00, 0x00, 0xFF},
};

static inline
void cell_on(const int x, const int y)
{
	CELL_REF(cells, x, y) = 1;

	++CELL_REF(count, x-1, y-1); ++CELL_REF(count, x+0, y-1); ++CELL_REF(count, x+1, y-1);
	++CELL_REF(count, x-1, y+0);                              ++CELL_REF(count, x+1, y+0);
	++CELL_REF(count, x-1, y+1); ++CELL_REF(count, x+0, y+1); ++CELL_REF(count, x+1, y+1);
}

static inline
void cell_off(const int x, const int y)
{
	CELL_REF(cells, x, y) = 0;

	--CELL_REF(count, x-1, y-1); --CELL_REF(count, x+0, y-1); --CELL_REF(count, x+1, y-1);
	--CELL_REF(count, x-1, y+0);                              --CELL_REF(count, x+1, y+0);
	--CELL_REF(count, x-1, y+1); --CELL_REF(count, x+0, y+1); --CELL_REF(count, x+1, y+1);
}

static
void sum_cells(void)
{
	int x, y;
	for(x=0; x<WINW; ++x)
		for(y=0; y<WINH; ++y)
			CELL_REF(count, x, y) =
				CELL_REF(cells, x-1, y-1) + CELL_REF(cells, x+0, y-1) + CELL_REF(cells, x+1, y-1) +
				CELL_REF(cells, x-1, y+0) +                             CELL_REF(cells, x+1, y+0) +
				CELL_REF(cells, x-1, y+1) + CELL_REF(cells, x+0, y+1) + CELL_REF(cells, x+1, y+1);
}

static
Uint8 rand1(void)
{
	static Uint8 c;
	static Uint16 b = 0;

	b >>= 1;
	if(b == 0) {
		c = fgetc( urandom );
		b = 128;
	}

	return c & b ? 1 : 0;
}

static
void fill_random(void)
{
	int x, y;
	for(x=0; x<WINW; ++x)
		for(y=0; y<WINH; ++y)
			CELL_REF(cells, x, y) = rand1();

	sum_cells();
}

static
void viewswitch(void)
{
	view = !view;

	bkg->pixels = view ? count : cells;

	SDL_SetPaletteColors(
		bkg->format->palette,
		view ? colors_2 : colors_1,
		0,
		view ? 9 : 2);
}

static
void drawbuf(void)
{
	static const SDL_Rect src = {1, 1, WINW, WINH};

	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bkg);
	SDL_RenderCopy(ren, tex, &src, NULL);
	SDL_DestroyTexture(tex);

#ifdef DEBUG
    SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0x00, 0xFF);
    int i;
    for(i=0; i<THREADS; ++i) {
        SDL_Rect r;

        memcpy(&r, &thread_data[i], sizeof(SDL_Rect));
        r.x *= SCALE;
        r.y *= SCALE;
        r.w *= SCALE;
        r.h *= SCALE;

        SDL_RenderDrawRect(ren, &r);
    }
#endif
}

static
void create_thread_data()
{
	int i;
	for(i=0; i<THREADS; ++i) {
		thread_data[i].rect.x = 0;
		thread_data[i].rect.w = WINW;

		thread_data[i].rect.y = WINH*i/THREADS;
		thread_data[i].rect.h = WINH - thread_data[i].rect.y;

		if( i != 0 ) {
			thread_data[i-1].rect.h -= thread_data[i].rect.h;
			thread_data[i-1].next = thread_data[i].prev = SDL_CreateMutex();
		}
	}
}

static inline
void game_logic(const int x, const int y) {
	const int count = CELL_REF(last_count, x, y);

	if( CELL_REF(last_cells, x, y) ) {
		if( count != 2 && count != 3 )
			cell_off(x, y);
	} else if( count == 3 ) {
		cell_on(x, y);
	}
}

static
int worker(void *data)
{
	struct ThreadData *thread_data = (struct ThreadData*)data;
	SDL_Rect rect = thread_data->rect;

	// x y w h -> x y x2 y2
	rect.w += rect.x;
	rect.h += rect.y;

	if (thread_data->prev) SDL_LockMutex(thread_data->prev);
	for(int x = rect.x; x<rect.w; ++x) {
		game_logic(x, rect.y);
		game_logic(x, rect.y+1);
	}
	if (thread_data->prev) SDL_UnlockMutex(thread_data->prev);
	for(int x = rect.x; x<rect.w; ++x) {
		for(int y = rect.y + 2; y<rect.h - 2; ++y) {
			game_logic(x, y);
		}
	}
	if (thread_data->next) SDL_LockMutex(thread_data->next);
	for(int x = rect.x; x<rect.w; ++x) {
		game_logic(x, rect.h-1);
		game_logic(x, rect.h-2);
	}
	if (thread_data->next) SDL_UnlockMutex(thread_data->next);

    return 0;
}

static
void clear_borders(void)
{
    int x, y;
    for(x=-1; x<WINW+2; ++x) CELL_REF(cells, x, -1) =      CELL_REF(count, x, -1) = 0;
    for(y=-1; y<WINH+2; ++y) CELL_REF(cells, -1, y) =      CELL_REF(count, -1, y) = 0;
    for(x=-1; x<WINW+2; ++x) CELL_REF(cells, x, WINH+1) =  CELL_REF(count, x, WINH+1) = 0;
    for(y=-1; y<WINH+2; ++y) CELL_REF(cells, -1, WINW+1) = CELL_REF(count, -1, WINW+1) = 0;
}

static
void calcframe(void)
{
    clear_borders();

	memcpy( last_cells, cells, WINS * 2 );

	SDL_Thread *threads[THREADS];

	int i;
	for(i=0; i<THREADS; ++i)
		threads[i] = SDL_CreateThread(worker, "worker", &thread_data[i]);
	for(i=0; i<THREADS; ++i)
		SDL_WaitThread(threads[i], NULL);
}

static
int input(void)
{
	SDL_Event e;
	int x, y;
	while( SDL_PollEvent( &e ) ) {
		switch( e.type) {
		case SDL_QUIT:
			return 0;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
			switch( SDL_GetMouseState(&x, &y) ) {
			case SDL_BUTTON_LMASK:
				if( !CELL_REF(cells, x/SCALE, y/SCALE) )
					cell_on(x/SCALE, y/SCALE);
				break;
			case SDL_BUTTON_RMASK:
				if( CELL_REF(cells, x/SCALE, y/SCALE) )
					cell_off(x/SCALE, y/SCALE);
				break;
			}
			break;
		case SDL_KEYDOWN:
			switch( e.key.keysym.sym ) {
			case SDLK_r: fill_random( ); break;
			case SDLK_p:
				paused = !paused;
				SDL_SetWindowTitle( win, paused ? "Game of Life *PAUSED*" : "Game of Life" );
				break;
			case SDLK_c: memset( cells, 0, WINS * 2 ); break;
			case SDLK_v: viewswitch(); break;
			case SDLK_s: ++framestep; break;
			}
			break;
		}
	}

	return 1;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

	win = SDL_CreateWindow( "Game of Life", 0, 0, WINW*SCALE, WINH*SCALE, 0 );
	ren = SDL_CreateRenderer( win, -1, SDL_RENDERER_PRESENTVSYNC );

	urandom = fopen( "/dev/urandom", "r" );

    /* Allocate a continuous block of memory */
	Uint8 *memblock = malloc( WINS * 4 );
	memset( memblock, 0, WINS * 4 );

    /* Set buffer pointers */
	cells =      memblock + (WINS * 0);
	count =      memblock + (WINS * 1);
	last_cells = memblock + (WINS * 2);
	last_count = memblock + (WINS * 3);

	create_thread_data();

	paused = 0;
	view = 0;
	framestep = 0;

	bkg = SDL_CreateRGBSurfaceFrom(
		view ? count : cells,
		WINW+2,
		WINH+2,
		8,
		WINW+2,
		0, 0, 0, 0);

	SDL_SetPaletteColors(
		bkg->format->palette,
		view ? colors_2 : colors_1,
		0,
		view ? LEN(colors_2) : LEN(colors_1) );

	while( input() ) {
		drawbuf();
		SDL_RenderPresent( ren );
		if( framestep || !paused ) {
			calcframe();
			if( framestep )
				--framestep;
		}
	}

	SDL_Quit();
    return 0;
}
