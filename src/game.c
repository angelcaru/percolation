#include "game.h"

#ifdef HOTRELOAD
    #define NOB_IMPLEMENTATION
#endif // HOTRELOAD
#define NOB_STRIP_PREFIX
#include "nob.h"

#include "raylib.h"
#define MACRO_VAR(name) _##name##__LINE__
#define BEGIN_END_NAMED(begin, end, i) for (int i = (begin, 0); i < 1; i++, end)
#define BEGIN_END(begin, end) BEGIN_END_NAMED(begin, end, MACRO_VAR(i))
#define Drawing() BEGIN_END(BeginDrawing(), EndDrawing())

float randfloat(void) {
	return rand() / (float)RAND_MAX;
}

#define GRID_WIDTH 400
#define GRID_HEIGHT 400

struct Game {
    size_t size;
	int grid_width;
	int grid_height;

	float horz_edges[GRID_WIDTH-1][GRID_HEIGHT];
	float vert_edges[GRID_WIDTH][GRID_HEIGHT-1];
	Color pixel_colors[GRID_WIDTH][GRID_HEIGHT];

	float p;
};

Game *g;

typedef struct {
	int x, y;
} IntVector2;

typedef struct {
	IntVector2 *items;
	size_t count, capacity;
} Propagation_Stack;

void propagate_color(int x, int y, Color c) {
	static Propagation_Stack stack = {0};
	da_append(&stack, (CLITERAL(IntVector2){x, y}));

	while (stack.count > 0) {
		IntVector2 cell = stack.items[--stack.count];
		int x = cell.x;
		int y = cell.y;
		if (g->pixel_colors[x][y].a != 0) continue;
		g->pixel_colors[x][y] = c;

		if (x < GRID_WIDTH - 1 && g->horz_edges[x][y] < g->p) da_append(&stack, (CLITERAL(IntVector2){x+1, y}));
		if (x > 0 && g->horz_edges[x-1][y] < g->p) da_append(&stack, (CLITERAL(IntVector2){x-1, y}));

		if (y < GRID_HEIGHT - 1 && g->vert_edges[x][y] < g->p) da_append(&stack, (CLITERAL(IntVector2){x, y+1}));
		if (y > 0 && g->vert_edges[x][y-1] < g->p) da_append(&stack, (CLITERAL(IntVector2){x, y-1}));
	}
}

void percolate(void) {
	memset(g->pixel_colors, 0, sizeof(g->pixel_colors));

	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			Color c = {GetRandomValue(0, 255), GetRandomValue(0, 255), GetRandomValue(0, 255), 255};
			propagate_color(x, y, c);
		}
	}
}

void reset_pixels(void) {
	g->grid_width = GRID_WIDTH;
	g->grid_height = GRID_HEIGHT;
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			if (x < GRID_WIDTH - 1) g->horz_edges[x][y] = randfloat();
			if (y < GRID_HEIGHT - 1) g->vert_edges[x][y] = randfloat();
		}
	}
}

void game_init(void) {
    g = malloc(sizeof(*g));
    memset(g, 0, sizeof(*g));
    g->size = sizeof(*g);

	reset_pixels();
	g->p = 0.5f;
}

Game* game_pre_reload(void) {
    return g;
}
void game_post_reload(Game *new_g) {
    g = new_g;

    if (g->size > sizeof(*g)) {
        nob_log(INFO, "Migrating struct Game (%zu bytes -> %zu bytes)", g->size, sizeof(*g));
        g = realloc(g, sizeof(*g));
        memset((char*)g + g->size, 0, sizeof(*g) - g->size);
        g->size = sizeof(*g);
    }

	if (g->grid_width != GRID_WIDTH || g->grid_height != GRID_HEIGHT) {
		reset_pixels();
	}
}

void game_update(void) {
	int slider_width = 40;
	int width = GetScreenWidth() - slider_width;
	int height = GetScreenHeight();

	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		g->p = 1 - GetMouseY() / (float)height;
		if (g->p > 1.0f) g->p = 1.0f;
		if (g->p < 0.0f) g->p = 0.0f;
	}

	SetRandomSeed(69);
	percolate();
    Drawing() {
        ClearBackground(BLACK);

		float pixel_width = width/(float)GRID_WIDTH;
		float pixel_height = height/(float)GRID_HEIGHT;
		for (int x = 0; x < GRID_WIDTH; x++) {
			for (int y = 0; y < GRID_HEIGHT; y++) {
#if 1
				int truew = (int)pixel_width;
				int trueh = (int)pixel_height;
				if ((int)((x + 1) * pixel_width) > x + truew) truew++;
				if ((int)((y + 1) * pixel_height) > y + trueh) trueh++;
				DrawRectangle(x*pixel_width, y*pixel_height, truew, trueh, g->pixel_colors[x][y]);
#else
				int truex = x * pixel_width + pixel_width/2;
				int truey = y * pixel_height + pixel_height/2;
				DrawCircle(truex, truey, 10, g->pixel_colors[x][y]);
				if (x < GRID_WIDTH - 1 && g->horz_edges[x][y] < g->p) DrawLine(truex, truey, (x+1) * pixel_width + pixel_width/2, truey, WHITE);
				if (y < GRID_HEIGHT - 1 && g->vert_edges[x][y] < g->p) DrawLine(truex, truey, truex, (y+1) * pixel_height + pixel_height/2, WHITE);
#endif
			}
		}

		DrawRectangle(width, height * (1 - g->p), slider_width, height, WHITE);
		DrawLine(width, height/2, width+slider_width, height/2, RED);
    }
}
