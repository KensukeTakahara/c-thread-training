#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *varA;
#define STRINGSIZE 32

void *threadFunc(void *arg) {
  int n = (int)arg;

  snprintf(varA, STRINGSIZE, "Hello I'm No.%d", n);
  printf("threadFunc-%d: Sets varA as '%s'\n", n, varA);
  sleep(2);
  printf("threadFunc-%d: After 2secs. varA is '%s'\n", n, varA);

  return NULL;
}

void anotherFunc() {
  char varB[STRINGSIZE];
  varA = varB;
}

int main(void) {
  pthread_t thread1, thread2;

  anotherFunc();
  strcpy(varA, "Good morning");
  printf("main-1: varA is '%s'\n", varA);
  pthread_create(&thread1, NULL, threadFunc, (void *)1);
  sleep(1);
  printf("main-2: varA is '%s'\n", varA);
  pthread_create(&thread2, NULL, threadFunc, (void *)2);
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  printf("main-3: varA is '%s'\n", varA);

  return 0;
}