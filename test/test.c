#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handle_alarm() {
  puts("Shit has happened");
}

int main() {
  signal(SIGALRM, handle_alarm);

  alarm(2);
  char z = getchar();
  return 0;
}
