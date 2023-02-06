#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define true 1
#define false 0
#define bool int

#define SLEEP_TIME 1
#define WIDTH 100
#define HEIGHT 40
#define SEED_AMT (WIDTH*HEIGHT)/2

#define ANSI_RED     "\x1b[31m"
#define ANSI_BLUE    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"

typedef struct cell{
    char value;
    bool alive;
    int team;
    // cells neighbors (null if out of bounds);
    /*
        tl tm tr
        ml __ mr
        bl bm br
    */
    struct cell *tl, *tm, *tr, *ml, *mr, *bl, *bm, *br;
} cell;

cell cell_matrix[HEIGHT][WIDTH] = {0};

const char bgChar = '#';

// set cell to alive
void bring_cell_to_life(cell* c){
    int rand_char = (rand() % 93)+33; // generate 33-126 in ascii

    (*c).value = rand_char;
    (*c).alive = true;
    (*c).team = (rand_char < 79) ? 1 : 0;
}

// set cell to dead
void kill_cell(cell* c){
    (*c).value = ' ';
    (*c).alive = false;
}

// sets amt amount of cells to alive
void seed(int amt){
    for(int c = 0; c < amt; c++){
        int i, j;
        do{
            i = rand() % HEIGHT;
            j = rand() % WIDTH;

        } while(cell_matrix[i][j].alive);
        bring_cell_to_life(&(cell_matrix[i][j]));
    }
}

// checks if neigboring cells around *c exist, if they don't set their pointer to NULL
void handle_neighbors(cell* c, int i, int j){
    // set top left neighbor cell
    if(i-1 < 0 && j-1 < 0)
        (*c).tl  = NULL;
    else
        (*c).tl = &(cell_matrix[i-1][j-1]);

    // set top middle neighbor cell
    if(i-1 < 0)
        (*c).tm  = NULL;
    else
        (*c).tm = &(cell_matrix[i-1][j]);

    // set top right neighbor cell
    if(i-1 < 0 && j+1 >= WIDTH)
        (*c).tr  = NULL;
    else
        (*c).tr = &(cell_matrix[i-1][j+1]);

    // set middle left neighbor cell
    if(j-1 < 0)
        (*c).ml  = NULL;
    else
        (*c).ml = &(cell_matrix[i][j-1]);

    // set middle right neighbor cell
    if(j+1 >= WIDTH)
        (*c).mr  = NULL;
    else
        (*c).mr = &(cell_matrix[i][j+1]);

    // set bottom left neighbor cell
    if(i+1 >= HEIGHT && j-1 < 0)
        (*c).bl  = NULL;
    else
        (*c).bl = &(cell_matrix[i+1][j-1]);

    // set top middle neighbor cell
    if(i+1 >= HEIGHT)
        (*c).bm  = NULL;
    else
        (*c).bm = &(cell_matrix[i+1][j]);

    // set top right neighbor cell
    if(i+1 >= HEIGHT && j+1 >= WIDTH)
        (*c).br  = NULL;
    else
        (*c).br = &(cell_matrix[i+1][j+1]);
}

int count_team_neighbors(cell* c){
    int count =  0;
    cell curr_cell = *c;

    int curr_team = curr_cell.team;

    // count upper row of live neighbors
    if(curr_cell.tl && curr_cell.tl->alive && curr_cell.tl->team == curr_team)
        count++;
    if(curr_cell.tm && curr_cell.tm->alive && curr_cell.tm->team == curr_team)
        count++;
    if(curr_cell.tr && curr_cell.tr->alive && curr_cell.tr->team == curr_team)
        count++;

    // count live cells to left or right
    if(curr_cell.ml && curr_cell.ml->alive && curr_cell.ml->team == curr_team)
        count++;
    if(curr_cell.mr && curr_cell.mr->alive && curr_cell.mr->team == curr_team)
        count++;

    // count bottom row of live neighbors
    if(curr_cell.bl && curr_cell.bl->alive && curr_cell.bl->team == curr_team)
        count++;
    if(curr_cell.bm && curr_cell.bm->alive && curr_cell.bm->team == curr_team)
        count++;
    if(curr_cell.br && curr_cell.br->alive && curr_cell.br->team == curr_team)
        count++;

    return count;
    
}

int count_opposing_neighbors(cell* c){
    int count =  0;
    cell curr_cell = *c;


    int curr_team = curr_cell.team;

    // count upper row of live neighbors
    if(curr_cell.tl && curr_cell.tl->alive && curr_cell.tl->team != curr_team)
        count++;
    if(curr_cell.tm && curr_cell.tm->alive && curr_cell.tm->team != curr_team)
        count++;
    if(curr_cell.tr && curr_cell.tr->alive && curr_cell.tr->team != curr_team)
        count++;

    // count live cells to left or right
    if(curr_cell.ml && curr_cell.ml->alive && curr_cell.ml->team != curr_team)
        count++;
    if(curr_cell.mr && curr_cell.mr->alive && curr_cell.mr->team != curr_team)
        count++;

    // count bottom row of live neighbors
    if(curr_cell.bl && curr_cell.bl->alive && curr_cell.bl->team != curr_team)
        count++;
    if(curr_cell.bm && curr_cell.bm->alive && curr_cell.bm->team != curr_team)
        count++;
    if(curr_cell.br && curr_cell.br->alive && curr_cell.br->team != curr_team)
        count++;

    return count;
    
}

int count_living_neighbors(cell* c){
    int count =  0;
    cell curr_cell = *c;

    // count upper row of live neighbors
    if(curr_cell.tl && curr_cell.tl->alive)
        count++;
    if(curr_cell.tm && curr_cell.tm->alive)
        count++;
    if(curr_cell.tr && curr_cell.tr->alive)
        count++;

    // count live cells to left or right
    if(curr_cell.ml && curr_cell.ml->alive)
        count++;
    if(curr_cell.mr && curr_cell.mr->alive)
        count++;

    // count bottom row of live neighbors
    if(curr_cell.bl && curr_cell.bl->alive)
        count++;
    if(curr_cell.bm && curr_cell.bm->alive)
        count++;
    if(curr_cell.br && curr_cell.br->alive)
        count++;

    return count;
}

// initialize every cell struct in cell_matrix to dead and set their neighbors
void init_grid(){
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            kill_cell(&(cell_matrix[i][j]));
            handle_neighbors(&(cell_matrix[i][j]), i, j);
        }
    }
    seed(SEED_AMT); // bring SEED_AMT number of cells to life
}

void draw_grid(){
    printf("\x1b[2J\x1b[H");
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            printf(cell_matrix[i][j].team ? ANSI_BLUE : ANSI_RED);
            putchar(cell_matrix[i][j].value);
        }
        printf(ANSI_RESET);
        putchar('\n');
    }
    sleep(SLEEP_TIME);
}

void check_winner(){
    int countBlue = 0, countRed = 0, totalCount = 0;
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            if(cell_matrix[i][j].alive){
                cell_matrix[i][j].team ? countBlue++ : countRed++;
                totalCount++;
            }
        }
    }

    if(totalCount == 0){
        draw_grid();
        printf("war has won\n");
        exit(0);
    }
    if(totalCount == countBlue){
        draw_grid();
        printf(ANSI_BLUE "blue has won\n" ANSI_RED);
        exit(0);
    }
    if(totalCount == countRed){
        draw_grid();
        printf(ANSI_RED "red has won\n" ANSI_RESET);
        exit(0);
    }
}


// Rules for the Game of Life
/*
    1. Any live cell with fewer than two live neighbours dies, as if by underpopulation.
    2. Any live cell with two or three live neighbours lives on to the next generation.
    3. Any live cell with more than three live neighbours dies, as if by overpopulation.
    4. Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
*/
void next_generation(){
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            int live_neighbors = count_living_neighbors(&(cell_matrix[i][j]));
            int team_neighbors = count_team_neighbors(&(cell_matrix[i][j]));
            int opposing_neighbors = count_opposing_neighbors(&(cell_matrix[i][j]));
            if(cell_matrix[i][j].alive){
                // Rules 1-3 of GoL
                // If:
                // - there are less than two live neighors OR
                // - there are more than three live neighbors, one of which are on the opposing team OR
                // - there are more neighbors on opposing team than the same team
                // then the cell is killed
                if(live_neighbors < 2 || (live_neighbors > 3 &&  opposing_neighbors > 0) || (opposing_neighbors > 0 && team_neighbors < opposing_neighbors))
                    kill_cell(&(cell_matrix[i][j]));
                
                // Squeeze rule:    
                // If: 
                // the top middle cell and the bottom middle cells are both on the opposing team OR
                // the middle left and middle right cells are both on the opposing team
                // then the cell is killed
                if(cell_matrix[i][j].tm && cell_matrix[i][j].bm)
                    if((cell_matrix[i][j].tm->team != cell_matrix[i][j].team && cell_matrix[i][j].bm->team != cell_matrix[i][j].team))
                        kill_cell(&(cell_matrix[i][j]));
                
                if(cell_matrix[i][j].ml && cell_matrix[i][j].mr)
                    if((cell_matrix[i][j].ml->team != cell_matrix[i][j].team && cell_matrix[i][j].mr->team != cell_matrix[i][j].team))
                        kill_cell(&(cell_matrix[i][j]));
            }
            else{
                // Rule 4 of GoL
                if(live_neighbors == 3){
                    bring_cell_to_life(&(cell_matrix[i][j]));
                }
            }
        }
    }
}

int main() {

    srand(time(NULL));

    init_grid();

    while(1){
        draw_grid();
        next_generation();
        check_winner();
    }

}