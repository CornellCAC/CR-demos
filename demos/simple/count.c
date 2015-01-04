#include <stdio.h>
#include <unistd.h>

int main(void) {
  unsigned long i = 0;
  while (1) {
    printf("%lu ", i);
    i = i + 1;
    sleep(1);
    fflush(stdout);
  } 
} 
