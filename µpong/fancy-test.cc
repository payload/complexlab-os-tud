#include <stdio.h>
#include <cstring>

bool DEBUG;

int main(int argc, char **argv) {
  DEBUG = argc > 1 && strcmp(argv[1], "DEBUG") == 0;
  printf("Yeah, let's face it!\n");
  return 0;
}
