#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int varA;

void *threadFunc(void *arg) {
  int n = (int)arg;
  int varB;
  static int varC = 0;

  varB = 4 * n;
  printf("threadFunc-%d-1: varA=%d, varB=%d\n", n, varA, varB);
  varA = 5 * n;
  printf("threadFunc-%d-2: varA=%d, varB=%d\n", n, varA, varB);
  sleep(2);
  printf("threadFunc-%d-3: varA=%d, varB=%d\n", n, varA, varB);
  varB = 6 * n;
  printf("threadFunc-%d-4: varA=%d, varB=%d\n", n, varA, varB);

  varC += 2 * n;
  printf("varC=%d\n", varC);

  return NULL;
}

int main(void) {
  pthread_t thread1, thread2;
  int varB;

  varA = 1;
  varB = 2;
  printf("main-1: varA=%d, varB=%d\n", varA, varB);
  pthread_create(&thread1, NULL, threadFunc, (void *)1);
  sleep(1);
  varB = 3;
  printf("main-2: varA=%d, varB=%d\n", varA, varB);
  pthread_create(&thread2, NULL, threadFunc, (void *)2);
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  printf("main-3: varA=%d, varB=%d\n", varA, varB);

  return 0;
}