#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ANSI_BLACK "\e[0;30m"
#define ANSI_RED "\e[0;31m"
#define ANSI_GREEN "\e[0;32m"
#define ANSI_YELLOW "\e[0;33m"
#define ANSI_BLUE "\e[0;34m"
#define ANSI_MAGENTA "\e[0;35m"
#define ANSI_CYAN "\e[0;36m"
#define ANSI_WHITE "\e[0;37m"

float A, B, C;

float cubeWidth = 20;
const int width = 100, height = 50;
float zBuffer[width * height];
char buffer[width * height];
int backgroundASCIICode = ' ';
int distanceFromCam = 50;
float horizontalOffset;
float K1 = 40;

float incrementSpeed = 0.5;

float x, y, z;
float ooz;
int xp, yp;
int idx;

float calculateX(int i, int j, int k) {
  return j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C) +
         j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C);
}

float calculateY(int i, int j, int k) {
  return j * cos(A) * cos(C) + k * sin(A) * cos(C) -
         j * sin(A) * sin(B) * sin(C) + k * cos(A) * sin(B) * sin(C) -
         i * cos(B) * sin(C);
}

float calculateZ(int i, int j, int k) {
  return k * cos(A) * cos(B) - j * sin(A) * cos(B) + i * sin(B);
}

void calculateForSurface(float cubeX, float cubeY, float cubeZ, int ch) {
  x = calculateX(cubeX, cubeY, cubeZ);
  y = calculateY(cubeX, cubeY, cubeZ);
  z = calculateZ(cubeX, cubeY, cubeZ) + distanceFromCam;

  ooz = 1 / z;

  xp = (int)(width / 2 + horizontalOffset + K1 * ooz * x * 2);
  yp = (int)(height / 2 + K1 * ooz * y);

  idx = xp + yp * width;
  if (idx >= 0 && idx < width * height) {
    if (ooz > zBuffer[idx]) {
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

int main(int argc, char* argv[]) {
  if(argc > 2){
    fprintf(stderr, "Usage: ./a.out [color]\n\tcolor(optional): [black, red, green, yellow, blue, magenta, cyan]\n");
    return -1;
  }

  if(argc == 2){
        if(!strcmp(argv[1], "black"))
          printf(ANSI_BLACK);
        else if(!strcmp(argv[1], "red"))
          printf(ANSI_RED);
        else if(!strcmp(argv[1], "green"))
          printf(ANSI_GREEN);
        else if(!strcmp(argv[1], "yellow"))
          printf(ANSI_YELLOW);
        else if(!strcmp(argv[1], "blue"))
          printf(ANSI_BLUE);
        else if(!strcmp(argv[1], "magenta"))
          printf(ANSI_MAGENTA);
        else if(!strcmp(argv[1], "cyan"))
          printf(ANSI_CYAN);
        else{
          fprintf(stderr, "Usage: ./a.out [color]\n\tcolor(optional): [black, red, green, yellow, blue, magenta, cyan]\n");
          return -1;
        }
          
  }

  printf("\x1b[2J"); // clears terminal screen
  while (1) {
    memset(buffer, backgroundASCIICode, width * height);
    memset(zBuffer, 0, width * height * 4);
    cubeWidth = 10;
    horizontalOffset = 0;
    // first cube
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed) {
      for (float cubeY = -cubeWidth; cubeY < cubeWidth;
           cubeY += incrementSpeed) {
        calculateForSurface(cubeX, cubeY, -cubeWidth, '@');
        calculateForSurface(cubeWidth, cubeY, cubeX, '$');
        calculateForSurface(-cubeWidth, cubeY, -cubeX, '~');
        calculateForSurface(-cubeX, cubeY, cubeWidth, '#');
        calculateForSurface(cubeX, -cubeWidth, -cubeY, ';');
        calculateForSurface(cubeX, cubeWidth, cubeY, '+');
      }
    }
    printf("\x1b[H"); // sets stdout cursor back to home position
    for (int k = 0; k < width * height; k++) {
      putchar(k % width ? buffer[k] : 10);
    }

    A += 0.05;
    B += 0.05;
    C += 0.05;
    usleep(8000 * 2);
  }
  return 0;
}