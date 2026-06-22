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

#define MAX_BRICKS 4
#define L_PIECE 0
#define I_PIECE 1

#define CURRENT_BLOCK &block_array.data[block_array.size-1]

// [rotation][brick][x/y]
int I_SHAPE_ROTATION[4][2][2] = {
    // rotation 0
    {
        {0,0},
        {0,1}
    },
    // rotation 1
    {
        {1,0},
        {0,0}
    },
    // rotation 2
    {
        {1,1},
        {1,0}
    },
    // rotation 3
    {
        {0,1},
        {1,1}
    }
};

int L_SHAPE_ROTATION[4][3][2] = {
        // rotation 0
        {
            {0,0},
            // {1,0},
            {0,1},
            {1,1}
        },
        // rotation 1
        {
            {1,0},
            // {1,1},
            {0,0},
            {0,1}
        },
        // rotation 2
        {
            {1,1},
            // {0,1},
            {1,0},
            {0,0}
        },
        // rotation 3
        {
            {0,1},
            // {0,0},
            {1,1},
            {1,0}
        } 
};

void initialize_block();

typedef struct brick {
    char symbol;
    unsigned int x, y;
} brick;

typedef struct tetronimo {
    int variation;
    int rotation;
    int x, y;

    bool dead;

    brick bricks[MAX_BRICKS];
    int brick_count;

    char* color;
} tetronimo;

typedef struct tetronimo_array {
    int capacity;
    int size;
    tetronimo* data;
} tetronimo_array;

typedef struct pixel {
    char symbol;
    char* color;
} pixel;

typedef enum direction {
    NONE,
    LEFT,
    RIGHT,
    DOWN,
    ROTATE
} direction;

pixel board[BOARD_HEIGHT][BOARD_WIDTH];
struct termios orig_termios;
char input[3] = {0};

_Atomic bool alive = true;
_Atomic bool rotate = false;

tetronimo_array block_array; // array of tetonimo blocks being drawn to the board

int score = 0;


pthread_t input_t;
pthread_attr_t input_t_attr;
pthread_mutex_t input_mutex;

void update_bricks(tetronimo* t);
void move_block(tetronimo* block, direction dir);

// allow terminal to read user input immediately
void enable_raw_input() {
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw_termios = orig_termios;
    raw_termios.c_lflag &= ~(ECHO | ICANON);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
}

// set it back to normal
void disable_raw_input() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// signal handler function for SIGINT
void sig_int(int sig) {
    alive = false;
}

void cleanup() {
    if(block_array.data)
        free(block_array.data);
    disable_raw_input();
    pthread_mutex_destroy(&input_mutex);
    pthread_attr_destroy(&input_t_attr);
    exit(0);
}

// wipe board every frame so blocks can be drawn to it
void initialize_board() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j].color = BORDER_COLOR;
            if(i == BOARD_HEIGHT - 1 || j == 0 || j == BOARD_WIDTH - 1)
                board[i][j].symbol = '#';
            else
                board[i][j].symbol = ' ';
        }
    }
}

// print the board to the terminal
void draw_board() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            printf("%s", board[i][j].color);
            putchar(board[i][j].symbol);
        }
        if(i == 1)
            printf("\tSCORE");
        if(i == 2)
            printf("\t%5d", score);
        putchar('\n');
    }

}

// places the linked list of tetronimos on the board that gets drawn on screen
void draw_blocks_to_board() {
    for(int i = 0; i < block_array.size; i++) {
        tetronimo* current = &block_array.data[i];
        for(int i = 0; i < current->brick_count; i++) {
            brick b = current->bricks[i];
            board[b.y][b.x].symbol = b.symbol;
            board[b.y][b.x].color = current->color;
        }
    }
}

// returns random color as ANSI escape code
char* random_color() {
    int rand_color = rand() % 7;
    switch(rand_color) {
        case 0: return HRED;
        case 1: return HGRN;
        case 2: return HYEL;
        case 3: return HBLU;
        case 4: return HMAG;
        case 5: return HCYN;
        case 6: return HWHT;
    }
    return HBLU;
}

void setup() {
    block_array.capacity = 256;
    block_array.size = 0;
    block_array.data = calloc(block_array.capacity, sizeof(tetronimo));
    if(block_array.data == NULL) {
        printf("block array cannot be allocated\n");
        exit(1);
    }
    initialize_block();
}

// creates new tetronimo block for the user to control
void initialize_block() {
    int i = block_array.size;

    block_array.data[i].dead = false;
    block_array.data[i].variation = rand() % 2;
    block_array.data[i].rotation = 0;
    
    block_array.data[i].x = BOARD_WIDTH / 2;
    block_array.data[i].y = 0;
    
    block_array.data[i].brick_count = 4;
    
    
    block_array.data[i].color = random_color();
    // make sure same color is never chosen twice
    if(i > 0 && !strcmp(block_array.data[i-1].color, block_array.data[i].color))
        block_array.data[i].color = random_color();
    // while(!strncmp(block_array.data[0].color, block_array.data[0].color, 7))
    // block_array.data[i].color = random_color();
    
    
    // L shape
    if(block_array.data[i].variation) {
        block_array.data[i].brick_count = 3;
        block_array.data[i].bricks[0].symbol = '@';
        block_array.data[i].bricks[1].symbol = '^';
        block_array.data[i].bricks[2].symbol = '&';
    }
    // I shape
    else {
        block_array.data[i].brick_count = 2;
        block_array.data[i].bricks[0].symbol = 'a';
        block_array.data[i].bricks[1].symbol = 'd';
        
    }
    update_bricks(&block_array.data[i]);
    block_array.size++;
    if(block_array.size == block_array.capacity) {
        block_array.capacity *= 2;
        block_array.data = realloc(block_array.data, block_array.capacity);
    }

}

// sets new x and y coords of each brick in a given tetronimo based on current rotation
void update_bricks(tetronimo* t) {
    for(int i = 0; i < t->brick_count; i++) {
        if(t->variation) {
            t->bricks[i].x = t->x + L_SHAPE_ROTATION[t->rotation][i][0];
            t->bricks[i].y = t->y + L_SHAPE_ROTATION[t->rotation][i][1];
        }
        else {
            t->bricks[i].x = t->x + I_SHAPE_ROTATION[t->rotation][i][0];
            t->bricks[i].y = t->y + I_SHAPE_ROTATION[t->rotation][i][1];

        }
    }
}

// move all bricks in tetronimo down
void move_down(tetronimo* current) {
    current->y++;
    update_bricks(current);
}

// called every frame on each block so bricks fall independently
void evaluate_block(tetronimo* t) {

    // find lowest positioned brick (which is technically the highest y value)
    int lowestY = t->bricks[0].y;
    for(int i = 0; i < t->brick_count; i++)
        if(t->bricks[i].y > lowestY)
            lowestY = t->bricks[i].y;

    bool shouldMove = true;

    // set current block to stop moving when reaches bottom border or another brick
    // if(t == current_block)
        for(int i = 0; i < t->brick_count; i++) {

            brick b = t->bricks[i];

            // if(b.symbol == ' ') continue;

            bool aboveOwn = false;
            for(int j = 0; j < t->brick_count; j++) {
                if(i == j) continue;

                brick bb = t->bricks[j];
                if(bb.x == b.x && bb.y == b.y+1)
                    aboveOwn = true;
            }

            if(b.y+1 >= BOARD_HEIGHT || 
                (board[b.y+1][b.x].symbol != ' ' && !aboveOwn))
                shouldMove = false;
        }

    if(shouldMove)
        move_block(t, DOWN);

    // time to create new block for user to pilot
    if(!shouldMove && t == CURRENT_BLOCK) {
        for(int i = 0; i < t->brick_count; i++) {
            if(t->bricks[i].y == 0) {
                alive = false;
                return;
            }
        }

        t->dead = true;
        initialize_block();
    }
}

// remove single brick from a tetronimos array of bricks
void remove_brick_at(tetronimo* t, int index) {
    for(int i = index; i < t->brick_count-1; i++)
        t->bricks[i] = t->bricks[i+1];
    t->brick_count--;
}

// removes specific brick from the board by finding corresponding brick in linked list of tetronimos
void find_and_free_brick(int row, int col) {

    for(int i = 0; i < block_array.size; i++) {
        tetronimo* current = &block_array.data[i];
        for(int i = 0; i < current->brick_count; i++) {
            if(current->bricks[i].y == row && current->bricks[i].x == col) {
                remove_brick_at(current, i);
                i--;
            }
        }

        // remove empty tetronimo
        if(current->brick_count == 0)
            memccpy(current, &block_array.data[i+1], '\0', (block_array.size * (sizeof(tetronimo))) - ((i+1) * (sizeof(tetronimo))));

    }
}

// free every brick in a row
void clear_row(int row) {
    for(int i = 0; i < BOARD_WIDTH; i++)
        find_and_free_brick(row, i);
}


// updates all tetronimo blocks 
void iterate() {

    // evaluate every tetronimo in linked list
    for(int i = 0; i < block_array.size; i++) {
        tetronimo* t = &block_array.data[i];
        evaluate_block(t);
    }


    int cleared_rows = 0;
    // check if any rows can be cleared
    for(int i = 0; i < BOARD_HEIGHT-1; i++) {
        bool clearable = true;
        for(int j = 0; j < BOARD_WIDTH; j++) {
            if(board[i][j].symbol == ' ') {
                clearable = false;
                break;
            }
        }
        if(clearable){
            cleared_rows++;
            clear_row(i);
        }
    }

    score += (cleared_rows * 10);
}

bool can_rotate(tetronimo* t) {
    int next_rotation = (t->rotation+1) % 4;
    for(int i = 0; i < 4; i++) {
        int x, y;
        if(t->variation) {
            x = t->x +
                L_SHAPE_ROTATION[next_rotation][i][0];
            y = t->y +
                L_SHAPE_ROTATION[next_rotation][i][1];
            }
        else {
            x = t->x +
                I_SHAPE_ROTATION[next_rotation][i][0];
            y = t->y +
                I_SHAPE_ROTATION[next_rotation][i][1];

        }
        if(x <= 0 || x >= BOARD_WIDTH-1)
            return false;
        if(y >= BOARD_HEIGHT-1)
            return false;
        // if(board[y][x].symbol != ' ')
        //     return false;
    }   
    return true;
}

// determine if a block can move in a specific direction
bool can_move(tetronimo* t, int dx, int dy){
    for(int i = 0; i < t->brick_count; i++) {
        int x = t->bricks[i].x + dx;
        int y = t->bricks[i].y + dy;
        if(x <= 0 || x >= BOARD_WIDTH-1)
            return false;
        if(y >= BOARD_HEIGHT-1)
            return false;
    }
    return true;
}

// move every brick in a block in a certain direction
void move_block(tetronimo* block, direction dir) {

    if(dir == ROTATE) {
        if(!can_rotate(block)) return;
        block->rotation = (block->rotation+1) % 4;
        update_bricks(block);
        return;
    }
    int dx = 0, dy = 0;
    switch(dir) {
        case LEFT:
            dx = -1;
            break;
        case RIGHT:
            dx = 1;
            break;
        case DOWN:
            dy = 1;
            break;
        default:
            break;
    }
    if(!can_move(block, dx, dy)) return;
    switch(dir) {
        case LEFT:
            block->x--;
            break;
        case RIGHT:
            block->x++;
            break;
        case DOWN:
            block->y++;
            break;
        default:
            break;
    }

    update_bricks(block);
   
}


// runs in separate thread
// reads user keyboard input and moves current block
void* process_input() {
    while(alive) {
        memset(input, 0, sizeof(input));
        direction input_dir = NONE;
        
        size_t bytes_read = read(0, &input, sizeof(input));
        
        // wasd
        if(bytes_read == 1) {
            char c = input[0];
            if(c=='a'||c=='A') input_dir = LEFT;
            if(c=='d'||c=='D') input_dir = RIGHT;
            if(c=='s'||c=='S') input_dir = DOWN;
            if(c=='r'||c=='R') input_dir = ROTATE;
        }
        // arrow keys
        else if(bytes_read == 3 && input[0]=='\033' && input[1]=='[') {
            if(input[2]=='B') input_dir = DOWN;
            if(input[2]=='C') input_dir = RIGHT;
            if(input[2]=='D') input_dir = LEFT;
        }
        
        if(input_dir == NONE)
        continue;
        
        pthread_mutex_lock(&input_mutex);
            move_block(CURRENT_BLOCK, input_dir);
        pthread_mutex_unlock(&input_mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

int main() {
    // init random and pthreads
    pthread_attr_init(&input_t_attr);
    srand(time(NULL));

    // install signal handler and change input mode
    signal(SIGINT, sig_int);
    enable_raw_input();

    // create separate input thread and run it
    pthread_create(&input_t, &input_t_attr, process_input, NULL);
    pthread_detach(input_t);

    pthread_mutex_init(&input_mutex, NULL);


    setup();

    while(alive){
        RESET_SCREEN;

        initialize_board();
        
        pthread_mutex_lock(&input_mutex);
            draw_blocks_to_board();
        
            draw_board();
        
            iterate();
        pthread_mutex_unlock(&input_mutex);

        usleep(200000);
    }

    printf("You lost!");
    cleanup();
    return 0;
}