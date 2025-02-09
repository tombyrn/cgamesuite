#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>


#define BOARD_WIDTH 20
#define BOARD_HEIGHT 25
#define BORDER_COLOR "\e[1;34m"
#define RESET_SCREEN printf("\x1b[2J\x1b[H")

#define HBLK "\e[0;90m"
#define HRED "\e[0;91m"
#define HGRN "\e[0;92m"
#define HYEL "\e[0;93m"
#define HBLU "\e[0;94m"
#define HMAG "\e[0;95m"
#define HCYN "\e[0;96m"
#define HWHT "\e[0;97m"

typedef struct brick {
    char symbol;
    unsigned int x, y;
    struct brick* prev;
    struct brick* next;
} brick;

typedef struct tetronimo {
    int variation;
    bool dead;
    struct tetronimo* prev;
    struct tetronimo* next;
    brick* bricks;
    char* color;

} tetronimo;

typedef struct pixel {
    char symbol;
    char* color;
} pixel;

pixel board[BOARD_HEIGHT][BOARD_WIDTH];
struct termios orig_termios;
char input[3] = {0}; // input buffer (3 bytes for arrow keys)

bool alive = true;
tetronimo* head = NULL; // first block generated
tetronimo* current_block = NULL; // current user-controlled block

pthread_t input_t;
pthread_attr_t input_t_attr;
pthread_mutex_t input_mutex;


void enable_raw_input() {
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw_termios = orig_termios;
    raw_termios.c_lflag &= ~(ECHO | ICANON);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
}

void draw_board() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            printf("%s", board[i][j].color);
            putchar(board[i][j].symbol);
        }
        putchar('\n');
    }
}

void draw_blocks_to_board() {
    tetronimo* current = head;
    // go through each block
    while(current != NULL) {
        brick* curr_brick = current->bricks;
        // go through each brick in block
        while(curr_brick != NULL) {
            board[curr_brick->y][curr_brick->x].symbol = curr_brick->symbol;
            board[curr_brick->y][curr_brick->x].color = current->color;
            curr_brick = curr_brick->next;
        }

        current = current->next;
    }
}

void disable_raw_input() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void cleanup() {
    tetronimo* current = head;
    while(current != NULL) {
        brick* curr_brick = current->bricks;
        while(curr_brick != NULL) {
            brick* temp = curr_brick;
            curr_brick = curr_brick->next;
            free(temp);
        }

        tetronimo* temp = current;
        current = current->next;
        free(temp);
    }
    disable_raw_input();
    exit(0);
}

void initialize_board() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j].color = BORDER_COLOR;
            // draw borders
            if(i == BOARD_HEIGHT - 1 || j == 0 || j == BOARD_WIDTH - 1)
                board[i][j].symbol = '#';
            else
                board[i][j].symbol = ' ';
            
        }
    }
}

char* random_color() {
    int rand_color = rand() % 7;

    switch(rand_color) {
        case 0:
            return HRED;
        case 1:
            return HGRN;
        case 2:
            return HYEL;
        case 3:
            return HBLU;
        case 4:
            return HMAG;
        case 5:
            return HCYN;
        case 6:
            return HWHT;
    }

    return HBLU;
}

// Creates a new tetronimo block and returns a pointer to it
// also sets the global current_block* to the address of this block
tetronimo* initialize_block(tetronimo* prev) {
    tetronimo* block = malloc(sizeof(tetronimo));
    block->dead = false;
    block->variation = rand() % 2;
    block->color = random_color();
    
    /*
        Variation is 1:
            @
            !$
        Variation is 0:
            %
            &
    */
   // initialize bricks linked list
    if(block->variation){
        block->bricks = malloc(sizeof(brick));
        block->bricks->symbol = '@';
        block->bricks->x = BOARD_WIDTH/2;
        block->bricks->y = 0;
        block->bricks->prev = NULL;
        block->bricks->next = malloc(sizeof(brick));


        block->bricks->next->symbol = '!';
        block->bricks->next->x = BOARD_WIDTH/2;
        block->bricks->next->y = 1;
        block->bricks->next->prev = block->bricks;
        block->bricks->next->next = malloc(sizeof(brick));


        block->bricks->next->next->symbol = '$';
        block->bricks->next->next->x = BOARD_WIDTH/2 + 1;
        block->bricks->next->next->y = 1;
        block->bricks->next->next->prev = block->bricks->next;
        block->bricks->next->next->next = NULL;

        if(current_block && current_block->bricks->x+1 < BOARD_WIDTH-1) {
            block->bricks->x = current_block->bricks->x;
            block->bricks->next->x = current_block->bricks->x;
            block->bricks->next->next->x = current_block->bricks->x+1;
        }
    }
    else{
        block->bricks = malloc(sizeof(brick));

        block->bricks->symbol = '%';
        block->bricks->x = BOARD_WIDTH/2;
        block->bricks->y = 0;
        block->bricks->prev = NULL;
        block->bricks->next = malloc(sizeof(brick));


        block->bricks->next->symbol = '&';
        block->bricks->next->x = BOARD_WIDTH/2;
        block->bricks->next->y = 1;
        block->bricks->next->prev = block->bricks;
        block->bricks->next->next = NULL;

        if(current_block) {
            block->bricks->x = current_block->bricks->x;
            block->bricks->next->x = current_block->bricks->x;
        }
    }

    // Set up double linked list
    block->prev = NULL;
    block->next = NULL;
    if(prev){
        block->prev = prev;
        prev->next = block;
    }

    current_block = block;
    return block;
}

void move_down(tetronimo* current) {
    brick* current_brick = current->bricks;

    while(current_brick != NULL) {
        if(current_brick->y+1 < BOARD_HEIGHT-1)
            current_brick->y++;

        current_brick = current_brick->next;
    }
}

void evaluate_block(tetronimo* current) {
    // iterate through all bricks
    // find lowest brick
    brick* lowest_brick = current->bricks;
    brick* curr_brick = current->bricks;
    while(curr_brick != NULL){
        if(curr_brick->y >= lowest_brick->y)
            lowest_brick = curr_brick;

        curr_brick = curr_brick->next;
    }

    // iterate again through all bricks
    // to determine if block should move down
    // only move if all lowest blocks are above empty space
    bool shouldMove = true;
    curr_brick = current->bricks;
    while(curr_brick != NULL) {
        bool lowest = false;
        if(curr_brick->y >= lowest_brick->y)
            lowest = true;
        
        // if any brick is just above bottom the block should not move down
        if(curr_brick->y+1 >= BOARD_HEIGHT){
            shouldMove = false;
            break;
        }

        // if any of the lowest bricks aren't above a blank tile(' ') then the block should not move down
        if(lowest && board[curr_brick->y+1][curr_brick->x].symbol != ' ')
            shouldMove = false;

        curr_brick = curr_brick->next;
    }

    if(shouldMove){
        move_down(current);
    }

    //If we determine the current user controlled block hasn't moved then the block is dead
    if(!shouldMove && current == current_block){
        current->dead = true;

        // todo: player lost logic

        initialize_block(current); // create new block
    }

}


void find_and_free_brick(int row, int col) {
    // go through every block 
    tetronimo* current = head;
    while(current != NULL) {

        brick* curr_brick = current->bricks;

        // remove block from linked list if it has no bricks left
        if(curr_brick == NULL) {
            tetronimo* next = current->next;

            if(current->prev)
                current->prev->next = next;
            else head = next;

            if(next)
                next->prev = current->prev;

            free(current);
            current = next;
            continue;
        }

        // go through every brick in that block
        while(curr_brick != NULL) {

            // brick has been found
            if(curr_brick->y == row && curr_brick->x == col) {
                // remove from double linked list
                if(curr_brick->prev) 
                    curr_brick->prev->next = curr_brick->next;

                if(curr_brick->next)
                    curr_brick->next->prev = curr_brick->prev;

                if(current->bricks == curr_brick) 
                    current->bricks = curr_brick->next;
                

                brick* save = curr_brick;
                curr_brick = curr_brick->next;
                free(save);
            }
            else
                curr_brick = curr_brick->next;
        }

        current = current->next;
    }

}

void clear_row(int row) {
    for(int i = 0; i < BOARD_WIDTH; i++) {
        find_and_free_brick(row, i);
    }
}

void iterate() {
    tetronimo* current = head;
    // go through each block
    while(current != NULL) {
        evaluate_block(current);
        current = current->next;
    }

    // check for rows to clear
    for(int i = 0; i < BOARD_HEIGHT; i++) {
        bool canClear = true;   
        for(int j = 0; j < BOARD_WIDTH; j++) {
            if(board[i][j].symbol == ' '){
                canClear = false;
                break;
            }
        }
        if(canClear)
            clear_row(i);
    }
}

bool can_move(tetronimo* block, char direction) {
    bool movable = true;
    brick* curr_brick = block->bricks;
    int leftmostX = curr_brick->x;
    int rightmostX = curr_brick->x;
    int lowestY = curr_brick->y;
    switch (direction) {
        case 'l':
                // find leftmost brick x position;
                while(curr_brick != NULL) {
                    if(curr_brick->x < leftmostX)
                        leftmostX = curr_brick->x;
                    curr_brick = curr_brick->next;
                }
                // if any of the leftmost bricks can't be moved movable is false
                curr_brick = block->bricks;
                while(curr_brick != NULL) {
                    if(curr_brick->x == leftmostX && (curr_brick->x-1 < 1 || board[curr_brick->y][curr_brick->x-1].symbol != ' '))
                        movable = false;
                    curr_brick = curr_brick->next;
                }
            break;
        case 'r':
                // find rightmost brick x position;
                while(curr_brick != NULL) {
                    if(curr_brick->x > rightmostX)
                        rightmostX = curr_brick->x;
                    curr_brick = curr_brick->next;
                }
                // if any of the rightmost bricks can't be moved movable is false
                curr_brick = block->bricks;
                while(curr_brick != NULL) {
                    if(curr_brick->x == rightmostX && (curr_brick->x+1 == BOARD_WIDTH-1 || board[curr_brick->y][curr_brick->x+1].symbol != ' '))
                        movable = false;
                    curr_brick = curr_brick->next;
                }
            break;
        case 'd':
                // find lowest brick y position;
                while(curr_brick != NULL) {
                    if(curr_brick->y > lowestY)
                        lowestY = curr_brick->y;
                    curr_brick = curr_brick->next;
                }
                // if any of the lowest bricks can't be moved movable is false
                curr_brick = block->bricks;
                while(curr_brick != NULL) {
                    if(curr_brick->y == lowestY && (curr_brick->y+1 == BOARD_HEIGHT-1 || board[curr_brick->y+1][curr_brick->x].symbol != ' '))
                        movable = false;
                    curr_brick = curr_brick->next;
                }
            
            break;
        default:
            break;
    }
    return movable;

}

void move_block(tetronimo* block, char direction) {
        if(!can_move(block, direction))
            return;

        brick* curr_brick = block->bricks;
        while(curr_brick != NULL) {
            switch (direction) {
                case 'l':
                    curr_brick->x--;        
                    break;
                case 'r':
                    curr_brick->x++; 
                    break;       
                case 'd':
                    curr_brick->y++;        
                    break;
                case 'n':
                    break;
                default:
                    break;
            }
            curr_brick = curr_brick->next;
        }
}

// running in separate thread
void* process_input() {

    while(alive) {
        memset(input, 0, sizeof(input));
        char inputDir = 'n'; // Will be either 'n', 'l', 'r', 'd'
                                /*
                                    none
                                    left
                                    right
                                    down
                                */

        size_t bytes_read = read(0, &input, sizeof(input));

        //Read character key
        if (bytes_read == 1) {

            // Single character input, handle regular keys
            char inputChar = input[0];
            switch(inputChar) {
                case 'a':
                    inputDir = 'l';
                    break;
                case 'A':
                    inputDir = 'l';
                    break;
                case 's':
                    inputDir = 'd';
                    break;
                case 'S':
                    inputDir = 'd';
                    break;
                case 'd':
                    inputDir = 'r';
                    break;
                case 'D':
                    inputDir = 'r';
                    break;
            }

        }
        //Read Arrow Key 
        else if (bytes_read == 3 && input[0] == '\033' && input[1] == '[') {
            // Arrow key detected, buffer[2] contains the specific arrow key code
            char arrowKey = input[2];

            switch (arrowKey) {
                // Down arrow key
                case 'B':
                    inputDir = 'd';
                    break;
                // Right arrow key
                case 'C':
                    inputDir = 'r';
                    break;
                // Left arrow key
                case 'D':
                    inputDir = 'l';
                    break;
            }
        }
        pthread_mutex_lock(&input_mutex);
        move_block(current_block, inputDir);
        pthread_mutex_unlock(&input_mutex);
    }

    pthread_exit(NULL);
}


int main(int argc, char** argv) {
    pthread_attr_init(&input_t_attr);


    srand(time(NULL));

    if(signal(SIGINT, cleanup) == SIG_ERR){
        fprintf(stderr, "Cannot catch SIGINT\n");
        return -1;
    }

    enable_raw_input();

    if(pthread_create(&input_t, &input_t_attr, process_input, NULL) != 0) {
        printf("Error: Thread creation failed\n");
        return -1;
    }
    pthread_detach(input_t);

    if(pthread_mutex_init(&input_mutex, NULL) != 0) {
        printf("Error: Mutex creation failed\n");
        return -1;
    };

    head = initialize_block(NULL);

    
    while(alive){
        RESET_SCREEN;
        initialize_board();

        pthread_mutex_lock(&input_mutex);
            draw_blocks_to_board();
        pthread_mutex_unlock(&input_mutex);

        draw_board();

        pthread_mutex_lock(&input_mutex);
            iterate(); //check clearings and move bricks down
        pthread_mutex_unlock(&input_mutex);

        usleep(200000);
    }
    printf("You lost");

    return 0;
}