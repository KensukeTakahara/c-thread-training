#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "xyqueue.h"

#define WIDTH 70
#define HEIGHT 23
#define MAX_FLY 1

#define QUEUE_SIZE 10

static int stopRequest;
static int drawRequest;
pthread_mutex_t drawMutex;
pthread_cond_t drawCond;

static void requestDraw(void);

void mSleep(int msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

int pthread_cond_timedwait_msec(pthread_cond_t* cond, pthread_mutex_t* mutex,
                                long msec) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += msec / 1e3;
  ts.tv_nsec += (msec / 1e3) * 1e6;
  if (ts.tv_nsec >= 1e9) {
    ts.tv_sec++;
    ts.tv_nsec -= 1e9;
  }
  return pthread_cond_timedwait(cond, mutex, &ts);
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
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  XYQueue* destQue;
} Fly;

Fly flyList[MAX_FLY];

void FlyInitCenter(Fly* fly, char mark_) {
  fly->mark = mark_;
  pthread_mutex_init(&fly->mutex, NULL);
  pthread_cond_init(&fly->cond, NULL);
  fly->x = (double)(WIDTH / 2.0);
  fly->y = (double)(HEIGHT / 2.0);
  fly->angle = 0;
  fly->speed = 2;
  fly->destQue = XYQueueCreate(QUEUE_SIZE);
}

void FlyDestroy(Fly* fly) {
  pthread_mutex_destroy(&fly->mutex);
  pthread_cond_destroy(&fly->cond);
  XYQueueDestroy(fly->destQue);
}

void FlyMove(Fly* fly) {
  pthread_mutex_lock(&fly->mutex);
  fly->x += cos(fly->angle);
  fly->y += sin(fly->angle);
  pthread_mutex_unlock(&fly->mutex);
  requestDraw();
}

int FlyIsAt(Fly* fly, int x, int y) {
  int res;
  pthread_mutex_lock(&fly->mutex);
  res = ((int)(fly->x) == x) && ((int)(fly->y) == y);
  pthread_mutex_unlock(&fly->mutex);
  return res;
}

void FlySetDirecetion(Fly* fly, double destX, double destY) {
  pthread_mutex_lock(&fly->mutex);
  double dx = destX - fly->x;
  double dy = destY - fly->y;
  fly->angle = atan2(dy, dx);
  fly->speed = sqrt(dx * dx + dy * dy) / 5.0;
  if (fly->speed < 2) {
    fly->speed = 2;
  }
  pthread_mutex_unlock(&fly->mutex);
}

double FlyDistence(Fly* fly, double x, double y) {
  double dx, dy, res;
  pthread_mutex_lock(&fly->mutex);
  dx = x - fly->x;
  dy = y - fly->y;
  res = sqrt(dx * dx + dy * dy);
  pthread_mutex_unlock(&fly->mutex);
  return res;
}

int FlySetDestination(Fly* fly, double x, double y) {
  return XYQueueAdd(fly->destQue, x, y);
}

int FlyWaitForSetDestination(Fly* fly, double* destX, double* destY,
                             long msec) {
  if (!XYQueueWait(fly->destQue, msec)) {
    return 0;
  }
  if (!XYQueueGet(fly->destQue, destX, destY)) {
    return 0;
  }
  return 1;
}

void* doMove(void* arg) {
  Fly* fly = (Fly*)arg;
  double destX, destY;
  while (!stopRequest) {
    if (!FlyWaitForSetDestination(fly, &destX, &destY, 100)) {
      continue;
    }
    FlySetDirecetion(fly, destX, destY);
    while ((FlyDistence(fly, destX, destY) >= 1) && !stopRequest) {
      FlyMove(fly);
      mSleep((int)(1000.0 / fly->speed));
    }
  }
  return NULL;
}

void requestDraw() {
  pthread_mutex_lock(&drawMutex);
  drawRequest = 1;
  pthread_cond_signal(&drawCond);
  pthread_mutex_unlock(&drawMutex);
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
  int err;
  pthread_mutex_lock(&drawMutex);
  while (!stopRequest) {
    err = pthread_cond_timedwait_msec(&drawCond, &drawMutex, 1e3);
    if (err != 0 && err != ETIMEDOUT) {
      printf("Error on pthread_cond_wait\n");
      exit(1);
    }
    while (drawRequest && !stopRequest) {
      drawRequest = 0;
      pthread_mutex_unlock(&drawMutex);
      drawScreen();
      pthread_mutex_lock(&drawMutex);
    }
  }
  pthread_mutex_unlock(&drawMutex);
  return NULL;
}

int main() {
  pthread_t drawThread;
  pthread_t moveThread;

  char buf[40], *cp;
  double destX, destY;

  pthread_mutex_init(&drawMutex, NULL);
  pthread_cond_init(&drawCond, NULL);

  clearScreen();
  FlyInitCenter(&flyList[0], '@');

  pthread_create(&moveThread, NULL, doMove, (void*)&flyList[0]);
  pthread_create(&drawThread, NULL, doDraw, NULL);
  requestDraw();

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