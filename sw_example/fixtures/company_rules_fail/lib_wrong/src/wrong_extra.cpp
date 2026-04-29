#include "lib_wrong_extra.h"

int wrong_prefix_counter = 0;

int lib_wrong_extra(void)
{
  int localValue = wrong_prefix_counter;
  return localValue;
}
