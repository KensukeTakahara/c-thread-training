#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int varA;

void processFunc(int n) {
  int varB;

  varB = 4 * n;
  printf("threadFunc-%d-1: varA=%d, varB=%d\n", n, varA, varB);
  varA = 5 * n;
  printf("threadFunc-%d-2: varA=%d, varB=%d\n", n, varA, varB);
  sleep(2);
  printf("threadFunc-%d-3: varA=%d, varB=%d\n", n, varA, varB);
  varB = 6 * n;
  printf("threadFunc-%d-4: varA=%d, varB=%d\n", n, varA, varB);

  exit(1);
}

int main(void) {
  pid_t process1, process2;
  int varB;

  varA = 1;
  varB = 2;
  printf("main-1: varA=%d, varB=%d\n", varA, varB);
  if ((process1 = fork()) == 0) {
    processFunc(1);
  }
  sleep(1);
  varB = 3;
  printf("main-2: varA=%d, varB=%d\n", varA, varB);
  if ((process2 = fork()) == 0) {
    processFunc(2);
  }
  waitpid(process1, NULL, 0);
  waitpid(process2, NULL, 0);
  printf("main-3: varA=%d, varB=%d\n", varA, varB);

  return 0;
}