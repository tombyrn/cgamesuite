#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>

#define SCALE_AMT 8000
#define SNAKE_CHAR 'o'
#define FRUIT_CHAR '@'
#define RESET_SCREEN printf("\x1b[2J\x1b[H")

int height, width;
struct termios orig_termios;
char input = '#';
int running  = 1;
int total = 0;

typedef struct scale{
    int x, y;
    int direction;
    int number;
    struct scale* next;
} scale;

typedef struct fruit{
    int x,y;
    int eaten;
} fruit;

void enable_raw_input(){
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw_termios = orig_termios;
    raw_termios.c_lflag &= ~(ECHO | ICANON);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
}

void disable_raw_input(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}


void sigint_handler(){
    disable_raw_input();
    exit(0);
}

void move_scale(scale* scale){
    if(scale->x < 1 || scale->x>=width-1 || scale->y < 1 || scale->y>=height-1){
        running = 0;
        return;
    }

    switch (scale->direction){
        case 1:
            scale->y--;
            break;
        case 2:
            scale->x++;
            break;
        case 3:
            scale->y++;
            break;
        case 4:
            scale->x--;
            break;
        default:
            break;
    }

    if(scale->next != NULL){
        
        move_scale(scale->next);
        scale->next->direction = scale->direction;
    }
}

void handle_input(scale* snake){
    size_t bytes_read = read(STDIN_FILENO, &input, 1);
    if(bytes_read == 1){
        if(input == 'w' || input == 'W'){
            snake->direction = 1;
        }
        if(input == 'a' || input == 'A'){
            snake->direction = 4;
        }
        if(input == 's' || input == 'S'){
            snake->direction = 3;
        }
        if(input == 'd' || input == 'D'){
            snake->direction = 2;
        }
        if(input == 'Q'){
            running = 0;
        }
    }
}

void check_fruit(scale* snake, fruit* fruit){
    if(fruit->x == snake->x && fruit->y == snake->y){
        fruit->eaten = 1;
        total++;
    }
}

scale* create_new_scale(scale* head){
    scale* s = head;
    while(s->next != NULL)
        s = s->next;

    int x,y;
    switch (s->direction){
        case 1:
            x = s->x;
            y = s->y+1;
            break;
        case 2:
            x = s->x-1;
            y = s->y;
            break;
        case 3:
            x = s->x;
            y = s->y-1;
            break;
        case 4:
            x = s->x+1;
            y = s->y;
            break;
        
    }

    scale* new_scale = malloc(sizeof(scale));
    new_scale->direction = s->direction;

    new_scale->x = s->x;
    new_scale->y = y;
    new_scale->next = NULL;
    new_scale->number = s->number+1;
    s->next = new_scale;
    return new_scale;
}

void init_board(char board[height][width] ){
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(i == 0 || i == height-1 || j == 0 || j == width-1)
                board[i][j] = '#';
            else
                board[i][j] = ' ';
        }
    }
}

void draw_snake(scale* snake, char board[height][width]){
    board[snake->y][snake->x] = SNAKE_CHAR;
    if(snake->next != NULL)
        draw_snake(snake->next, board);
}

void draw_fruit(fruit* fruit, char board[height][width]){
    if(fruit->eaten){
        do{
            fruit->x = rand() % width;
            fruit->y = rand() % height;
        }while(board[fruit->y][fruit->x] != ' ');
        fruit->eaten = 0;
    }
    board[fruit->y][fruit->x] = FRUIT_CHAR;
}

void draw_board(char board[height][width] ){
    RESET_SCREEN;
    board[0][0] = input;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            putchar(board[i][j]);
        }
        putchar('\n');
    }

}

int main(int argc, char* argv[]){
    int onearg = 0;

    if(argc != 1 && argc != 3){
        fprintf(stderr, "Usage: ./snake [width] [height]\n");
        return -1;
    }

    if(argc == 1)
        onearg = 1;

    if(argc == 3){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }

    if(onearg || width < 10 || height < 10){
        width = 30;
        height = 20;
    }

    // install sigint signal handler
    if(signal(SIGINT, sigint_handler) == SIG_ERR){
        fprintf(stderr, "Cannot catch SIGINT\n");
        return -1;
    }

    // seed RNG
    srand(time(NULL)); 
    
    char board[height][width];

    // create snake
    scale snake;
    snake.x = width/2;
    snake.y = height/2;
    snake.direction = 1;
    snake.next = NULL;
    snake.number = 0;
    create_new_scale(&snake);
    create_new_scale(&snake);
    create_new_scale(&snake);

    fruit fruit;
    fruit.eaten = 1;


    enable_raw_input();
    while(running){
        init_board(board); // resets the board, with just the boundaries
        draw_snake(&snake, board); // adds all snake scales the the board
        draw_fruit(&fruit, board); // adds fruit to random blank tile in board
        draw_board(board); // clear the screen and draw the entire board
        
        handle_input(&snake);
        move_scale(&snake);
        check_fruit(&snake, &fruit);
        usleep(150000);
    }
    

}