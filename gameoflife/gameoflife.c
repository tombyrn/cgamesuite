#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#define true 1
#define false 0
#define bool int

#define SLEEP_TIME 1
#define WIDTH 100
#define HEIGHT 30

#define RED "\x1b[;31m"
#define GREEN "\x1b[;32m"
#define YELLOW "\x1b[;33m"
#define BLUE "\x1b[;34m"
#define MAGENTA "\x1b[;35m"
#define CYAN "\x1b[;36m"
#define WHITE "\x1b[;37m"
#define BLACK "\x1b[;30m"
#define CLEAR_SCREEN printf("\x1b[2J\x1b[H")

typedef struct cell{
    char value;
    bool alive;
    char* color_code;
    // cells neighbors (null if out of bounds);
    /*
        tl tm tr
        ml __ mr
        bl bm br
    */
    struct cell *tl, *tm, *tr, *ml, *mr, *bl, *bm, *br;
} cell;

/* Global Variables */
cell** cell_matrix;
int width, height;

const char bgChar = ' ';

/* Cell Functions */
// set cell to alive
void bring_cell_to_life(cell* c){
    int rand_char = (rand() % 93)+33; // generate 33-126 in ascii
    int rand_color = (rand() % 7) + 31;

    char* color_code;
    switch (rand_color){
    case 31:
        color_code = RED;
        break;
    case 32:
        color_code = GREEN;
        break;
    case 33:
        color_code = YELLOW;
        break;
    case 34:
        color_code = BLUE;
        break;
    case 35:
        color_code = MAGENTA;
        break;
    case 36:
        color_code = CYAN;
        break;
    case 37:
        color_code = WHITE;
        break;
    default:
        color_code  =  BLACK;
        break;
    }

    // assign cell random color and character
    c->value = rand_char;
    c->color_code = color_code;
    c->alive = true;
}

// set cell to dead
void kill_cell(cell* c){
    c->value = bgChar;
    c->alive = false;
}

// checks if neigboring cells around *c exist, if they don't set their pointer to NULL
void init_neighbor_cells(cell* c, int i, int j){
    /* TOP NEIGHBORS */ 
    // set top left neighbor cell
    if(i-1 < 0 || j-1 < 0)
        c->tl  = NULL;
    else
        c->tl = &(cell_matrix[i-1][j-1]);

    // set top middle neighbor cell
    if(i-1 < 0)
        c->tm  = NULL;
    else
        c->tm = &(cell_matrix[i-1][j]);

    // set top right neighbor cell
    if(i-1 < 0 || j+1 >= width)
        c->tr  = NULL;
    else
        c->tr = &(cell_matrix[i-1][j+1]);

    /* MIDDLE NEIGHBORS */ 
    // set middle left neighbor cell
    if(j-1 < 0)
        c->ml  = NULL;
    else
        c->ml = &(cell_matrix[i][j-1]);

    // set middle right neighbor cell
    if(j+1 >= width)
        c->mr  = NULL;
    else
        c->mr = &(cell_matrix[i][j+1]);

    /* BOTTOM NEIGHBORS */ 
    // set bottom left neighbor cell
    if(i+1 >= height || j-1 < 0)
        c->bl  = NULL;
    else
        c->bl = &(cell_matrix[i+1][j-1]);

    // set bottom middle neighbor cell
    if(i+1 >= height)
        c->bm  = NULL;
    else
        c->bm = &(cell_matrix[i+1][j]);

    // set bottom right neighbor cell
    if(i+1 >= height || j+1 >= width)
        c->br  = NULL;
    else
        c->br = &(cell_matrix[i+1][j+1]);
    
}

// count the number of live neighbors around cell *c
int count_living_neighbors(cell* c, int i, int j){
    int count =  0;

    // count upper row of live neighbors
    if(c->tl && c->tl->alive)
        count++;
    if(c->tm && c->tm->alive)
        count++;
    if(c->tr && c->tr->alive)
        count++;

    // count live cells to left or right
    if(c->ml && c->ml->alive)
        count++;
    if(c->mr && c->mr->alive)
        count++;

    // count bottom row of live neighbors
    if(c->bl && c->bl->alive)
        count++;
    if(c->bm && c->bm->alive)
        count++;
    if(c->br && c->br->alive)
        count++;

    return count;
}

/* Grid Functions */

// initialize every cell in cell_matrix to dead and set their neighbors
void init_grid(){
    // allocate memory for cell_matrix
    cell_matrix = calloc(height, sizeof(cell*));
    for(int i = 0; i < height; i++)
        cell_matrix[i] = calloc(width, sizeof(cell));

    // initialize cells
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            kill_cell(&(cell_matrix[i][j]));
            init_neighbor_cells(&(cell_matrix[i][j]), i, j);
        }
    }
}

// draw the grid to the terminal
void draw_grid(){
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(cell_matrix[i][j].alive)
                printf("%s",cell_matrix[i][j].color_code);
            putchar(cell_matrix[i][j].value);
        }
        putchar('\n');
    }
    sleep(SLEEP_TIME);
}

// randomly seed the grid with living cells
void seed(){
    int seed_amt = (width * height) / 8;
    for(int c = 0; c < seed_amt; c++){
        int i = rand() % height;
        int j = rand() % width;

        bring_cell_to_life(&(cell_matrix[i][j]));
    }
}

/*
    1. Any live cell with fewer than two live neighbours dies, as if by underpopulation.
    2. Any live cell with two or three live neighbours lives on to the next generation.
    3. Any live cell with more than three live neighbours dies, as if by overpopulation.
    4. Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
*/
void next_generation(){
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            int live_neighbors = count_living_neighbors(&(cell_matrix[i][j]), i, j);
            if(cell_matrix[i][j].alive){
                // Rules 1-3
                if(live_neighbors < 2 || live_neighbors > 3)
                    kill_cell(&(cell_matrix[i][j]));
            }
            // Rule 4   
            else if(live_neighbors == 3){
                    bring_cell_to_life(&(cell_matrix[i][j]));
            }
        }
    }
}

void parse_arguments(int argc, char** argv){
    if(argc == 3){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        if(width < 1 || height < 1){
            printf("Width and height must be integers greater than 0\n");
            exit(1);
        }
    }
    else if(argc == 1){
        width = WIDTH;
        height = HEIGHT;
    }
    else {
        printf("Usage: ./gameoflife [width] [height]\n");
        exit(1);
    }
}

void sigint_handler(){
    for(int i = 0; i < height; i++)
        free(cell_matrix[i]);
    free(cell_matrix);
    // printf("Freed & Exiting...\n");
    exit(0);
}

int main(int argc, char** argv) {

    if(signal(SIGINT, sigint_handler) == SIG_ERR){
        fprintf(stderr, "Error: Cannot catch SIGINT\n");
        return -1;
    }

    srand(time(NULL)); // seed random number generator

    parse_arguments(argc, argv);
    init_grid();
    seed();

    while(1){
        CLEAR_SCREEN;
        draw_grid();
        next_generation();
    }

}