#include <stdio.h>
#include <stdlib.h>

extern int MALLOC_DEBUG;

int main()
{
  MALLOC_DEBUG = 1;

  int *a, *b;

  printf("\n# test 0 #\n");
  a = new int(0xCAFE);
  if (*a != 0xCAFE) return 1;
  delete a;

  printf("\n# test 0.5 #\n");
  a = new int[3];
  a[2] = 0;
  delete a;

  printf("\n# test 1 #\n");
  a = new int;
  b = new int;
  delete a;
  delete b;

  printf("\n# test 2 #\n");
  a = new int;
  b = new int;
  delete a;
  a = new int;
  printf("\n# check #\n");
  delete a;
  delete b;

  printf("\n# test 3 #\n");
  a = new int[2000];
  
  printf("\n# test 4 #\n");
  delete a;

  printf("\n# test 5 #\n");
  a = new int[2000];
  b = new int[10000];
  delete a;
  delete b;
  /**/  
  return 0;
}
