#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

SDL_Window *win = NULL;
SDL_Renderer *renderer = NULL;

SDL_Texture *dungeonTex;
SDL_Texture *screen;

unsigned char *map, mapW, mapH;
unsigned char submap[3*4];

int playerX, playerY, playerR;

void f_sdlAssert(bool cond, const int line) {
	if(cond)
		return;
	printf("%d: %s\n", line, SDL_GetError());
	exit(1);
}
#define sdlAssert(C) f_sdlAssert(C, __LINE__)

SDL_Texture *loadTexture(const char *filename) {
	SDL_Surface *surf = SDL_LoadBMP(filename);
	sdlAssert(surf);
	SDL_SetColorKey(surf, SDL_TRUE, SDL_MapRGB(surf->format,
				0, 0xff, 0xff));
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
	sdlAssert(tex);
	SDL_FreeSurface(surf);
	return tex;
}

void initSDL() {
	sdlAssert(win = SDL_CreateWindow("HAIRDRESSER",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		640, 480, SDL_WINDOW_RESIZABLE));

	sdlAssert(renderer = SDL_CreateRenderer(win, -1,
			SDL_RENDERER_SOFTWARE));

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);

	/* create screen */
	screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_TARGET, 160, 120);
	SDL_SetRenderTarget(renderer, screen);
}

SDL_Rect getScreenRect() {
	int w, h;
	SDL_GetWindowSize(win, &w, &h);
	float xs = (float)w/160.0, ys = (float)h/120.0;
	float scale = (xs > ys) ? ys : xs;
	SDL_Rect r = {w/2-80.0*scale, h/2-60.0*scale, 160*scale, 120*scale};
	return r;
}

void presentScreen() {
	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderClear(renderer);
	SDL_Rect r = getScreenRect();
	SDL_RenderCopy(renderer, screen, NULL, &r);
	SDL_RenderPresent(renderer);
	SDL_SetRenderTarget(renderer, screen);
}

void endSDL() {
	SDL_DestroyTexture(screen);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

void loadMedia() {
	dungeonTex = loadTexture("img/dungeon.bmp");
}

void freeMedia() {
	SDL_DestroyTexture(dungeonTex);
}

void loadMap(const char *filename) {
	FILE *fp = fopen(filename, "rb");
	assert(fp);
	fread(&mapW, 1, 1, fp);
	fread(&mapH, 1, 1, fp);
	map = malloc(mapW*mapH);
	fread(map, 1, mapW*mapH, fp);
	fclose(fp);
}

void printMap() {
	for(int y = 0; y < mapH; y++) {
		for(int x = 0; x < mapW; x++)
			switch(map[y*mapW+x]) {
			case 0:
				printf(".");
				break;
			case 1:
				printf("#");
				break;
			default:
				printf("?");
				break;
			}
		printf("\n");
	}
}

void freeMap() {
	free(map);
}

int getTile(int x, int y) {
	if(x < 0 || y < 0 || x >= mapW || y >= mapH)
		return 2;
	return map[y*mapW+x];
}

void rotate(int *x, int *y, int n) {
	for(int i = 0; i < n; i++) {
		int ox = *x;
		*x = *y;
		*y = -ox;
	}
}

void getSubmap() {
	for(int i = 0; i < 3*4; i++) {
		int dx = i%3, dy = i/3;
		int x = dx-1, y = dy;
		rotate(&x, &y, playerR);
		x += playerX;
		y += playerY;
		submap[i] = getTile(x, y);
		if(x == playerX && y == playerY)
			submap[i] = 3;
	}
}

void printSubmap() {
	for(int i = 0; i < 3*4; i++) {
		printf("%d", submap[i]);
		if(!((i+1)%3))
			printf("\n");
	}
}

void drawView() {
	SDL_Rect src, dst;
	src = (SDL_Rect){64, 192, 64, 64};
	dst = (SDL_Rect){0, 0, 64, 64};
	SDL_RenderCopy(renderer, dungeonTex, &src, &dst);
	src = (SDL_Rect){64*((3-playerR)%2), 256+((3-playerR)/2)*64, 64, 64};
	SDL_RenderCopy(renderer, dungeonTex, &src, &dst);

	for(int y = 4-1; y >= 0; y--) {
		int r = 3-y;
		unsigned char *m = submap+y*3;
		if(m[0] == 1) {
			src = (SDL_Rect){0, r*64, 32, 64};
			dst = (SDL_Rect){0, 0, 32, 64};
			SDL_RenderCopy(renderer, dungeonTex, &src, &dst);
		}
		if(m[2] == 1) {
			src = (SDL_Rect){32, r*64, 32, 64};
			dst = (SDL_Rect){32, 0, 32, 64};
			SDL_RenderCopy(renderer, dungeonTex, &src, &dst);
		}
		if(m[1] == 1) {
			src = (SDL_Rect){64, r*64, 64, 64};
			dst = (SDL_Rect){0, 0, 64, 64};
			SDL_RenderCopy(renderer, dungeonTex, &src, &dst);
		}
	}
}

void draw() {
	SDL_RenderClear(renderer);
	drawView();
	//SDL_RenderPresent(renderer);
	presentScreen();
}

void playerMove(int x, int y) {
	int dx = x, dy = y;
	rotate(&dx, &dy, playerR);
	dx += playerX;
	dy += playerY;
	if(getTile(dx, dy) != 0)
		return;
	playerX = dx;
	playerY = dy;
}

int main() {
	initSDL();
	loadMedia();

	loadMap("lvl/0");
	playerX = 1;
	playerY = 1;
	playerR = 1;
	printMap();
	getSubmap();
	printSubmap();

	bool quit = false;
	while(!quit) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev))
			switch(ev.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				switch(ev.key.keysym.sym) {
				case SDLK_LEFT:
					playerR--;
					if(playerR < 0)
						playerR += 4;
					break;
				case SDLK_RIGHT:
					playerR++;
					playerR %= 4;
					break;
				case SDLK_UP:
					playerMove(0, 1);
					break;
				case SDLK_DOWN:
					playerMove(0, -1);
					break;
				}
				getSubmap();
				break;
			}
		draw();
	}

	freeMap();

	endSDL();
	freeMedia();

	return 0;
}
