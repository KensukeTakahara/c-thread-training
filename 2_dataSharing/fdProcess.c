#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void processFunc() {
  const char* msg = "processFunc writes to standard output\n";
  write(1, msg, strlen(msg));
  close(1);
  open("fdProcess.txt", O_WRONLY | O_CREAT | O_TRUNC, 0744);
  msg = "processFunc writes to fdProcess.txt\n";
  write(1, msg, strlen(msg));
  sleep(2);
}

int main(void) {
  pid_t process1;

  if ((process1 = fork()) == 0) {
    processFunc();
  }
  sleep(1);

  const char* msg = "main write to standard output\n";
  write(1, msg, strlen(msg));

  waitpid(process1, NULL, 0);

  return 0;
}