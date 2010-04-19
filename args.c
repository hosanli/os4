#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i;
  for (i = 0; i < argc; i++) {
    printf(1, "Argument %d is %s\n", i, argv[i]);
  }
  exit();
}
