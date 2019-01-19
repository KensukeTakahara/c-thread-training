#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WIDTH 70
#define HEIGHT 23
#define MAX_FLY 1
#define DRAW_CYCLE 50

int stopRequest;

pthread_mutex_t mutex;

void mSleep(int msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

void clearScreen() { fputs("\033[2J", stdout); }

void moveCursor(int x, int y) { printf("\033[%d;%dH", x, y); }

void saveCursor() { printf("\0337"); }

void restoreCursor() { printf("\0338"); }

typedef struct {
  char mark;
  double x, y;
  double angle;
  double speed;
  double destX, destY;
  int busy;
  pthread_mutex_t mutex;
} Fly;

Fly flyList[MAX_FLY];

void FlyInitCenter(Fly* fly, char mark_) {
  fly->mark = mark_;
  pthread_mutex_init(&fly->mutex, NULL);
  fly->x = (double)(WIDTH / 2.0);
  fly->y = (double)(HEIGHT / 2.0);
  fly->angle = 0;
  fly->speed = 2;
  fly->destX = fly->x;
  fly->destY = fly->y;
  fly->busy = 0;
}

void FlyDestroy(Fly* fly) { pthread_mutex_destroy(&fly->mutex); }

void FlyMove(Fly* fly) {
  pthread_mutex_lock(&fly->mutex);
  fly->x += cos(fly->angle);
  fly->y += sin(fly->angle);
  pthread_mutex_unlock(&fly->mutex);
}

int FlyIsAt(Fly* fly, int x, int y) {
  int res;
  pthread_mutex_lock(&fly->mutex);
  res = ((int)(fly->x) == x) && ((int)(fly->y) == y);
  pthread_mutex_unlock(&fly->mutex);
  return res;
}

void FlySetDirecetion(Fly* fly) {
  pthread_mutex_lock(&fly->mutex);
  double dx = fly->destX - fly->x;
  double dy = fly->destY - fly->y;
  fly->angle = atan2(dy, dx);
  fly->speed = sqrt(dx * dx + dy * dy) / 5.0;
  if (fly->speed < 2) {
    fly->speed = 2;
  }
  pthread_mutex_unlock(&fly->mutex);
}

double FlyDistenceToDestination(Fly* fly) {
  double dx, dy, res;
  pthread_mutex_lock(&fly->mutex);
  dx = fly->destX - fly->x;
  dy = fly->destY - fly->y;
  res = sqrt(dx * dx + dy * dy);
  pthread_mutex_unlock(&fly->mutex);
  return res;
}

int FlySetDestination(Fly* fly, double x, double y) {
  if (fly->busy) {
    return 0;
  }
  pthread_mutex_lock(&fly->mutex);
  fly->destX = x;
  fly->destY = y;
  pthread_mutex_unlock(&fly->mutex);
  return 1;
}

void* doMove(void* arg) {
  Fly* fly = (Fly*)arg;
  while (!stopRequest) {
    fly->busy = 0;
    while ((FlyDistenceToDestination(fly) < 1) && !stopRequest) {
      mSleep(100);
    }
    fly->busy = 1;
    FlySetDirecetion(fly);
    while ((FlyDistenceToDestination(fly) >= 1) && !stopRequest) {
      FlyMove(fly);
      mSleep((int)(1000.0 / fly->speed));
    }
  }
  return NULL;
}

void drawScreen() {
  int x, y;
  char ch;
  int i;

  saveCursor();
  moveCursor(0, 0);
  for (y = 0; y < HEIGHT; ++y) {
    for (x = 0; x < WIDTH; ++x) {
      ch = 0;
      for (i = 0; i < MAX_FLY; ++i) {
        if (FlyIsAt(&flyList[i], x, y)) {
          ch = flyList[i].mark;
          break;
        }
      }
      if (ch != 0) {
        putchar(ch);
      } else if ((y == 0) || (y == HEIGHT - 1)) {
        putchar('-');
      } else if ((x == 0) || (x == WIDTH - 1)) {
        putchar('|');
      } else {
        putchar(' ');
      }
    }
    putchar('\n');
  }
  restoreCursor();
  fflush(stdout);
}

void* doDraw(void* arg) {
  while (!stopRequest) {
    drawScreen();
    mSleep(DRAW_CYCLE);
  }
  return NULL;
}

int main() {
  pthread_t drawThread;
  pthread_t moveThread;

  char buf[40], *cp;
  double destX, destY;

  clearScreen();
  FlyInitCenter(&flyList[0], '@');

  pthread_create(&moveThread, NULL, doMove, (void*)&flyList[0]);
  pthread_create(&drawThread, NULL, doDraw, NULL);

  while (1) {
    printf("Destionation? ");
    fflush(stdout);
    fgets(buf, sizeof(buf), stdin);
    if (strncmp(buf, "stop", 4) == 0) {
      break;
    }
    destX = strtod(buf, &cp);
    destY = strtod(cp, &cp);
    if (!FlySetDestination(&flyList[0], destX, destY)) {
      printf("The fly is busy now. Try later.\n");
    }
  }

  stopRequest = 1;

  pthread_join(drawThread, NULL);
  pthread_join(moveThread, NULL);
  FlyDestroy(&flyList[0]);

  return 0;
}