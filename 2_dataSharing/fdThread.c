#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void* threadFunc() {
  const char* msg = "threadFunc writes to standard output\n";
  write(1, msg, strlen(msg));
  close(1);
  open("fdThread.txt", O_WRONLY | O_CREAT | O_TRUNC, 0744);
  msg = "threadFunc writes to fdThread.txt\n";
  write(1, msg, strlen(msg));
  sleep(2);
  pthread_exit(NULL);
}

int main(void) {
  pthread_t thread;

  pthread_create(&thread, NULL, threadFunc, NULL);

  sleep(1);

  const char* msg = "main write to standard output\n";
  write(1, msg, strlen(msg));

  pthread_join(thread, NULL);

  return 0;
}