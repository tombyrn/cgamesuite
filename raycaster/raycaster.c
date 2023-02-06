#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>

void restore_keyboard_blocking(struct termios *initial_settings)
{
	tcsetattr(0, TCSANOW, initial_settings);
}

void set_keyboard_blocking(struct termios *initial_settings)
{

    struct termios new_settings;
    tcgetattr(0,initial_settings);

    new_settings = *initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    // new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 0;

    tcsetattr(0, TCSANOW, &new_settings);
}

#define SCREEN_WIDTH 120
#define SCREEN_HEIGHT 40
wchar_t screen_bufffer[SCREEN_HEIGHT][SCREEN_WIDTH] = {0};

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define CLEAR_SCREEN printf("\x1b[2J\x1b[H")
char map[MAP_HEIGHT][MAP_WIDTH] = 
{
    {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#'},
    {'#', '#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', '#'},
    {'#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#'},
};

double player_pos_x =  5.0;
double player_pos_y =  6.0;
double player_angle =  0.0;
double fov = 3.14159 / 4.0;
double depth = 16.0;
double speed = 5.0;
clock_t time_point1;
clock_t time_point2;
clock_t elapsedTime;

void draw_screen(){
    CLEAR_SCREEN;
    for(int i = 0; i < SCREEN_HEIGHT; i++){
        for(int j = 0; j < SCREEN_WIDTH; j++){
            printf("%lc", screen_bufffer[i][j]);
        }
        if(i < MAP_HEIGHT+1){
            putchar('\t');
            for(int j = 0; j < MAP_WIDTH && i != MAP_HEIGHT; j++){
                printf("%c",(j == (int)player_pos_x &&  i == (int)player_pos_y) ? '*' : map[i][j]);
            }
            if(i == MAP_HEIGHT)
                printf("X: %d Y: %d A: %lf", (int)player_pos_x, (int)  player_pos_y, player_angle);
        }
        
        putchar('\n');
    }
    usleep(100000);
}

void move_player_forward(){
        // double new_pos_x = player_pos_x + (sinf(player_angle) *  2.0);
        // double new_pos_y = player_pos_y + cosf(player_angle);
        // if(map[(int)new_pos_y][(int)new_pos_x] != '#' && ((int)new_pos_x < MAP_WIDTH && (int)new_pos_y  < MAP_HEIGHT)){
        //     player_pos_x = new_pos_x;
        //     player_pos_y = new_pos_y;
        // }
    double direction = player_angle /(2.0 * 3.14159);

    int n = 0, e = 0, s = 0, w = 0;
    double new_pos_x = player_pos_x, new_pos_y = player_pos_y;
    if(direction < 3.14159/2.0){
        new_pos_x = player_pos_x + 1.0; //* elapsedTime);
        e = 1;
    }
    if(direction >= 3.14159/2.0){
        new_pos_y = player_pos_y - 1.0; //* elapsedTime);
        n = 1;
    }
    if(direction > 3.14159){
        new_pos_x = player_pos_x - 1.0; //* elapsedTime);
        w = 1;
    }
    if(direction > (3.14159*3.0)/4.0){
        new_pos_y = player_pos_y + 1.0; //* elapsedTime);
        s = 1;
    }

    if(map[(int)new_pos_y][(int)new_pos_x] != '#' && ((int)new_pos_x < MAP_WIDTH && (int)new_pos_y  < MAP_HEIGHT) && new_pos_x > 0.0 && new_pos_y > 0.0){
        player_pos_x = new_pos_x;
        player_pos_y = new_pos_y;
    }

}

void check_input(char* c){

    // read one character non-canonically
    int read_bytes = read(STDIN_FILENO, c, 1);

    // if less than zero bytes have been read, an error has occured
    if(read_bytes < 0){
        fprintf(stderr, "Input Error: %d\n", read_bytes);
        exit(1);
    }

    // if no characters are read from input clear input character value
    if(read_bytes == 0){
        *c = ' ';
    }

    char input = *c;

    // W -> Move Forward (broken)
    if(input == 'w' || input == 'W'){
        move_player_forward();
        // double new_pos_x = player_pos_x + (sinf(player_angle) *  2.0);
        // double new_pos_y = player_pos_y + cosf(player_angle);
        // if(map[(int)new_pos_y][(int)new_pos_x] != '#' && ((int)new_pos_x < MAP_WIDTH && (int)new_pos_y  < MAP_HEIGHT)){
        //     player_pos_x = new_pos_x;
        //     player_pos_y = new_pos_y;
        // }
    }
    // S -> Move Backward (broken)
    else if(input == 's' || input == 'S'){
        double new_pos_x = player_pos_x - (sinf(player_angle) *  2.0);
        double new_pos_y = player_pos_y - (cosf(player_angle)*2.0);
        if(map[(int)new_pos_y][(int)new_pos_x] != '#' && ((int)new_pos_x < MAP_WIDTH && (int)new_pos_y < MAP_HEIGHT)){
            player_pos_x = new_pos_x;
            player_pos_y = new_pos_y;
        }
    }
    // A -> Rotate left
    else if(input == 'a' || input == 'A'){
        player_angle-=1.0;//  * elapsedTime;
    }
    // D -> Rotate right
    else if(input == 'd' || input == 'D'){
        player_angle+=1.0;// * elapsedTime;
    }
}

void raycasting(){
    for(int i = 0; i < SCREEN_WIDTH; i++){
        // calculate projected ray angle into world space
        double ray_angle = fov*((double)i/(double)SCREEN_WIDTH) + (player_angle - (fov / 2.0)); 
        
        // calculate distance from player to wall
        double distance_to_wall = 0.0;
        int  hit_wall = 0;

        double fov_y = sinf(ray_angle);
        double fov_x = cosf(ray_angle);

        while(!hit_wall && distance_to_wall < depth){
            distance_to_wall += 0.1;

            int test_x = (int)(player_pos_x + fov_x * distance_to_wall);
            int test_y = (int)(player_pos_y + fov_y * distance_to_wall);

            // ray is out of bounds
            if(test_x < 0 || test_x >= MAP_WIDTH || test_y < 0 || test_y >=  MAP_HEIGHT){
                hit_wall = 1;
                distance_to_wall = depth;
            }
            // ray is in bounds
            else{
                if(map[test_y][test_x] == '#'){
                    hit_wall = 1;
                }
            }
        }

        int ceiling = ((double)SCREEN_HEIGHT / 2.0) -  ((double)SCREEN_HEIGHT / (double) distance_to_wall);
        int floor = SCREEN_HEIGHT - ceiling;

        char shade  =  '#';
        
        if(distance_to_wall <= depth / 4.0) // closest
            shade = '@'; //L'\u2588';
        else if(distance_to_wall < depth / 3.0)
            shade = '$'; //L'\u2593';
        else if(distance_to_wall < depth / 2.0)
            shade = '%'; //L'\u2592';
        else if(distance_to_wall < depth )
            shade = '^'; //L'\u2591';
        else   // furthest
            shade = '.';

        for(int j = 0; j < SCREEN_HEIGHT; j++){
            if(j <= ceiling){
                screen_bufffer[j][i] = ' ';
            }
            // ray has hit a wall
            else if(j > ceiling && j <= floor){
                screen_bufffer[j][i] =  shade;
            }
            else{
                screen_bufffer[j][i] = '_';
            }
        }
    }
}



int main(){
    struct termios term_settings;
    char c = 0;

    set_keyboard_blocking(&term_settings);

    // time_point1 = clock();
    // time_point2 = time_point1;

    while(c != 'Q' && c != 'q'){
        // time_point2 = clock();
        // elapsedTime = time_point2 - time_point1;
        // time_point1 = time_point2;

        raycasting();
        // player_angle += 0.05;

        draw_screen();
        check_input(&c);

    }

    restore_keyboard_blocking(&term_settings);
    CLEAR_SCREEN;
    return 0;
}