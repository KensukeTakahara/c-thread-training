#include "xyqueue.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  double x;
  double y;
} XYQueueItem;

struct XYQueue_ {
  XYQueueItem *data;
  size_t size;
  size_t wp;
  size_t rp;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
};

static void mSleep(int msec) {
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

static int pthread_cond_timedwait_msec(pthread_cond_t *cond,
                                       pthread_mutex_t *mutex, long msec) {
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

XYQueue *XYQueueCreate(size_t sz) {
  if (sz == 0) {
    return NULL;
  }
  XYQueue *que = (XYQueue *)malloc(sizeof(XYQueue));
  if (que == NULL) {
    return NULL;
  }
  que->size = sz + 1;
  que->data = (XYQueueItem *)malloc(que->size * sizeof(XYQueue));
  if (que->data == NULL) {
    free(que);
    return NULL;
  }
  que->wp = que->rp = 0;
  pthread_mutex_init(&que->mutex, NULL);
  pthread_cond_init(&que->cond, NULL);
  return que;
}

void XYQueueDestroy(XYQueue *que) {
  if (que == NULL) {
    return;
  }
  pthread_mutex_destroy(&que->mutex);
  pthread_cond_destroy(&que->cond);
  free(que->data);
  free(que);
}

size_t XYQueueGetSize(XYQueue *que) {
  if (que == NULL) {
    return 0;
  }
  size_t ret;
  ret = que->size - 1;
  return ret;
}

size_t XYQueueGetCount(XYQueue *que) {
  if (que == NULL) {
    return 0;
  }
  size_t ret;
  pthread_mutex_lock(&que->mutex);
  if (que->wp < que->rp) {
    ret = que->size + que->wp - que->rp;
  } else {
    ret = que->wp - que->rp;
  }
  pthread_mutex_unlock(&que->mutex);
  return ret;
}

size_t XYQueueGetFreeCount(XYQueue *que) {
  if (que == NULL) {
    return 0;
  }
  size_t ret;
  ret = que->size - 1 - XYQueueGetCount(que);
  return ret;
}

int XYQueueAdd(XYQueue *que, double x, double y) {
  if (que == NULL) {
    return 0;
  }
  pthread_mutex_lock(&que->mutex);
  size_t next_wp = que->wp + 1;
  if (next_wp >= que->size) {
    next_wp = que->size;
  }
  if (next_wp == que->rp) {
    return 0;
  }
  que->data[que->wp].x = x;
  que->data[que->wp].y = y;
  que->wp = next_wp;
  pthread_cond_signal(&que->cond);
  pthread_mutex_unlock(&que->mutex);
  return 1;
}

int XYQueueGet(XYQueue *que, double *x, double *y) {
  if (que == NULL) {
    return 0;
  }
  int ret;
  pthread_mutex_lock(&que->mutex);
  if (que->rp == que->wp) {
    pthread_mutex_unlock(&que->mutex);
    return 0;
  }
  if (x != NULL) {
    *x = que->data[que->rp].x;
  }
  if (y != NULL) {
    *y = que->data[que->rp].y;
  }
  if (++(que->rp) >= que->size) {
    ret = que->rp -= que->size;
  } else {
    ret = 1;
  }
  pthread_mutex_unlock(&que->mutex);
  return ret;
}

int XYQueueWait(XYQueue *que, long msec) {
  if (que == NULL) {
    return 0;
  }
  pthread_mutex_lock(&que->mutex);
  int err = pthread_cond_timedwait_msec(&que->cond, &que->mutex, msec);
  int ret;
  switch (err) {
    case 0: {
      ret = que->wp != que->rp ? 1 : 0;
    } break;
    case ETIMEDOUT: {
      ret = 0;
    } break;
    default: {
      // error
      printf("error.\n");
      ret = 0;
    } break;
  }
  pthread_mutex_unlock(&que->mutex);
  return ret;
}
