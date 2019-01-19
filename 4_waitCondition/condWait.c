#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;

void* threadFunc(void* arg) {
  printf("threadFunc: start\n");
  sleep(2);
  pthread_mutex_lock(&mutex);
  printf("threadFunc: Wait for stgnal\n");
  if (pthread_cond_wait(&cond, &mutex) != 0) {
    printf("threadFunc: Error on pthread_cond_wait\n");
    exit(1);
  }
  printf("threadFunc: Get signal\n");
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main() {
  pthread_t thread;

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);

  pthread_create(&thread, NULL, threadFunc, NULL);
  sleep(3);
  printf("main: Signal\n");
  pthread_cond_signal(&cond);
  pthread_join(thread, NULL);

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  return 0;
}
