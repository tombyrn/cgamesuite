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

typedef struct brick {
    char symbol;
    unsigned int x, y;
} brick;

typedef struct tetronimo {
    int variation;
    bool dead;
    struct tetronimo* prev;
    struct tetronimo* next;

    /* CHANGE: replaced linked list with array */
    brick bricks[MAX_BRICKS];
    int brick_count;

    char* color;
} tetronimo;

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
tetronimo* head = NULL;
tetronimo* current_block = NULL;

int score = 0;

pthread_t input_t;
pthread_attr_t input_t_attr;
pthread_mutex_t input_mutex;


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
// free linked list of tetronimos
void cleanup() {
    tetronimo* current = head;
    while(current != NULL) {
        tetronimo* temp = current;
        current = current->next;
        free(temp);
    }
    disable_raw_input();
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
    tetronimo* current = head;
    while(current != NULL) {
        for(int i = 0; i < current->brick_count; i++) {
            brick b = current->bricks[i];
            board[b.y][b.x].symbol = b.symbol;
            board[b.y][b.x].color = current->color;
        }
        current = current->next;
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

// creates new tetronimo block for the user to control
tetronimo* initialize_block(tetronimo* prev) {
    tetronimo* block = malloc(sizeof(tetronimo));
    block->dead = false;
    block->variation = rand() % 2;

    block->color = random_color();
    // make sure same color is never chosen twice
    if(current_block)
        while(!strncmp(current_block->color, block->color, 7))
            block->color = random_color();
            


    /*
        variation 1 (todo:rotations)
        @           !@          $!           $
        !$          $            @          @!
    */
   if(block->variation){
       block->brick_count = 3;
       
       block->bricks[0] = (brick){'@', BOARD_WIDTH/2, 0};
       block->bricks[1] = (brick){'!', BOARD_WIDTH/2, 1};
       block->bricks[2] = (brick){'$', BOARD_WIDTH/2 + 1, 1};
       
       if(current_block && current_block->bricks[0].x+1 < BOARD_WIDTH-1) {
           int x = current_block->bricks[0].x;
           block->bricks[0].x = x;
           block->bricks[1].x = x;
           block->bricks[2].x = x+1;
        }
    } 
    
    /*
        variation 0
        %       
        &
    */
    else {
        block->brick_count = 2;

        block->bricks[0] = (brick){'%', BOARD_WIDTH/2, 0};
        block->bricks[1] = (brick){'&', BOARD_WIDTH/2, 1};

        if(current_block) {
            int x = current_block->bricks[0].x;
            block->bricks[0].x = x;
            block->bricks[1].x = x;
        }
    }

    block->prev = NULL;
    block->next = NULL;
    if(prev){
        block->prev = prev;
        prev->next = block;
    }

    current_block = block;
    return block;
}

// move all bricks in tetronimo down
void move_down(tetronimo* current) {
    for(int i = 0; i < current->brick_count; i++) {
        if(current->bricks[i].y+1 < BOARD_HEIGHT-1)
            current->bricks[i].y++;
    }
}

// called every frame on each block so bricks fall independently
void evaluate_block(tetronimo* current) {

    // check if any bricks have killed the player
    for(int i = 0; i < current->brick_count; i++) {
        if(current->bricks[i].x < 0 || current->bricks[i].x >= BOARD_WIDTH ||
           current->bricks[i].y < 0 || current->bricks[i].y >= BOARD_HEIGHT) {
            alive = false;
            return;
        }
    }

    // find lowest positioned brick (which is technically the highest y value)
    int lowestY = current->bricks[0].y;
    for(int i = 0; i < current->brick_count; i++)
        if(current->bricks[i].y > lowestY)
            lowestY = current->bricks[i].y;

    bool shouldMove = true;

    // set current block to stop moving when reaches bottom border or another brick
    for(int i = 0; i < current->brick_count; i++) {
        brick b = current->bricks[i];

        if(b.y+1 >= BOARD_HEIGHT || 
            (b.y == lowestY && board[b.y+1][b.x].symbol != ' '))
            shouldMove = false;
    }

    if(shouldMove)
        move_down(current);

    // time to create new block for user to pilot
    if(!shouldMove && current == current_block){

        for(int i = 0; i < current->brick_count; i++) {
            if(current->bricks[i].y == 0) {
                alive = false;
                return;
            }
        }

        current->dead = true;
        initialize_block(current);
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

    tetronimo* current = head;
    while(current != NULL) {
        for(int i = 0; i < current->brick_count; i++) {
            if(current->bricks[i].y == row && current->bricks[i].x == col) {
                remove_brick_at(current, i);
                i--;
            }
        }

        // remove empty tetronimo
        if(current->brick_count == 0) {

            // if(current == current_block)
            //     current_block = NULL;

            tetronimo* next = current->next;

            if(current->prev)
                current->prev->next = next;
            else
                head = next;

            if(next)
                next->prev = current->prev;

            free(current);
            current = next;
            continue;
        }

        current = current->next;
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
    tetronimo* current = head;
    while(current != NULL) {
        evaluate_block(current);
        current = current->next;
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

// determine if a block can move in a specific direction
bool can_move(tetronimo* block, direction dir) {
    int left = block->bricks[0].x;
    int right = block->bricks[0].x;
    int bottom = block->bricks[0].y;

    // find leftmost, rightmost, and lowest bricks
    for(int i = 0; i < block->brick_count; i++) {
        if(block->bricks[i].x < left) left = block->bricks[i].x;
        if(block->bricks[i].x > right) right = block->bricks[i].x;
        if(block->bricks[i].y > bottom) bottom = block->bricks[i].y;
    }

    // check if bricks have reached a boundary
    for(int i = 0; i < block->brick_count; i++) {
        brick b = block->bricks[i];

        if(dir == LEFT && b.x == left &&
           (b.x-1 < 1 || board[b.y][b.x-1].symbol != ' '))
            return false;

        if(dir == RIGHT && b.x == right &&
           (b.x+1 == BOARD_WIDTH-1 || board[b.y][b.x+1].symbol != ' '))
            return false;

        if(dir == DOWN && b.y == bottom &&
           (b.y+1 == BOARD_HEIGHT-1 || board[b.y+1][b.x].symbol != ' '))
            return false;
    }

    return true;
}

// move every brick in a block in a certain direction
void move_block(tetronimo* block, direction dir) {
    if(!can_move(block, dir) || dir == NONE) return;

    for(int i = 0; i < block->brick_count; i++) {
        if(dir == LEFT) block->bricks[i].x--;
        if(dir == RIGHT) block->bricks[i].x++;
        if(dir == DOWN) block->bricks[i].y++;
    }
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
            move_block(current_block, input_dir);
        pthread_mutex_unlock(&input_mutex);
    }
    pthread_exit(NULL);
}

int main() {
    // init random and pthreads
    pthread_attr_init(&input_t_attr);
    srand(time(NULL));

    // install signal handler and change input mode
    signal(SIGINT, cleanup);
    enable_raw_input();

    // create separate input thread and run it
    pthread_create(&input_t, &input_t_attr, process_input, NULL);
    pthread_detach(input_t);

    pthread_mutex_init(&input_mutex, NULL);

    head = initialize_block(NULL);

    while(alive){
        RESET_SCREEN;
        initialize_board();
        
        draw_blocks_to_board();
        
        draw_board();
        
        pthread_mutex_lock(&input_mutex);
            iterate();
        pthread_mutex_unlock(&input_mutex);

        usleep(200000);
    }

    printf("You lost!");
    return 0;
}