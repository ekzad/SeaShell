// this code is NOT written by me

#include <ncurses/ncurses.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SNAKE_LEN 1000

typedef struct {
    int x, y;
} Point;

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;

Point snake[MAX_SNAKE_LEN];
int snake_len;
Direction dir;
Point food;
int score;
int width, height;
int game_over;

void spawn_food(void) {
    int valid;
    do {
        valid = 1;
        food.x = rand() % (width - 2) + 1;
        food.y = rand() % (height - 2) + 1;
        for (int i = 0; i < snake_len; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                valid = 0;
                break;
            }
        }
    } while (!valid);
}

void init_game(void) {
    getmaxyx(stdscr, height, width);

    snake_len = 3;
    dir = DIR_RIGHT;
    score = 0;
    game_over = 0;

    int start_x = width / 2;
    int start_y = height / 2;
    for (int i = 0; i < snake_len; i++) {
        snake[i].x = start_x - i;
        snake[i].y = start_y;
    }

    spawn_food();
}

void draw(void) {
    erase();

    for (int x = 0; x < width; x++) {
        mvaddch(0, x, '#');
        mvaddch(height - 1, x, '#');
    }
    for (int y = 0; y < height; y++) {
        mvaddch(y, 0, '#');
        mvaddch(y, width - 1, '#');
    }

    mvaddch(food.y, food.x, '*');

    for (int i = 0; i < snake_len; i++) {
        mvaddch(snake[i].y, snake[i].x, i == 0 ? 'O' : 'o');
    }

    mvprintw(0, 2, " Score: %d ", score);

    refresh();
}

void handle_input(void) {
    int ch = getch();
    switch (ch) {
        case 'w': case KEY_UP:
            if (dir != DIR_DOWN) dir = DIR_UP;
            break;
        case 's': case KEY_DOWN:
            if (dir != DIR_UP) dir = DIR_DOWN;
            break;
        case 'a': case KEY_LEFT:
            if (dir != DIR_RIGHT) dir = DIR_LEFT;
            break;
        case 'd': case KEY_RIGHT:
            if (dir != DIR_LEFT) dir = DIR_RIGHT;
            break;
        case 'q':
            game_over = 1;
            break;
        default:
            break;
    }
}

void update(void) {
    Point new_head = snake[0];

    switch (dir) {
        case DIR_UP:    new_head.y--; break;
        case DIR_DOWN:  new_head.y++; break;
        case DIR_LEFT:  new_head.x--; break;
        case DIR_RIGHT: new_head.x++; break;
    }

    if (new_head.x <= 0 || new_head.x >= width - 1 ||
        new_head.y <= 0 || new_head.y >= height - 1) {
        game_over = 1;
        return;
    }

    for (int i = 0; i < snake_len; i++) {
        if (snake[i].x == new_head.x && snake[i].y == new_head.y) {
            game_over = 1;
            return;
        }
    }

    int ate = (new_head.x == food.x && new_head.y == food.y);

    for (int i = snake_len; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = new_head;

    if (ate) {
        if (snake_len < MAX_SNAKE_LEN - 1) snake_len++;
        score += 10;
        spawn_food();
    }
}

void game_over_screen(void) {
    nodelay(stdscr, FALSE);
    erase();
    mvprintw(height / 2 - 1, width / 2 - 5, "GAME OVER");
    mvprintw(height / 2, width / 2 - 8, "Final Score: %d", score);
    mvprintw(height / 2 + 1, width / 2 - 12, "Press any key to exit...");
    refresh();
    getch();
}

int main(void) {
    srand(time(NULL));

    initscr();              /* start ncurses */
    cbreak();                /* disable line buffering */
    noecho();                 /* don't echo typed keys */
    keypad(stdscr, TRUE);      /* enable arrow keys */
    curs_set(0);                /* hide cursor */
    nodelay(stdscr, TRUE);        /* non-blocking getch */

    if (LINES < 10 || COLS < 20) {
        endwin();
        printf("Terminal window too small. Resize it and try again.\n");
        return 1;
    }

    init_game();

    while (!game_over) {
        handle_input();
        update();
        draw();
        napms(120);
    }

    game_over_screen();
    endwin();
    return 0;
}